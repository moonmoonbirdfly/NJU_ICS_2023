/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>  // 导入正则表达式库，用于对输入的表达式进行匹配和解析
#include <memory/vaddr.h>  // 导入虚拟地址库，对程序内存进行操作

// 这里定义了一些枚举常量，用于表示不同的标记类型
enum {
  TK_NOTYPE = 256,  // 当前没有标记类型
  
  TK_POS, TK_NEG, TK_DEREF,  // 正数、负数和解引用操作符
  TK_EQ, TK_NEQ, TK_GT, TK_LT, TK_GE, TK_LE,  // 等于、不等于、大于、小于、大于等于、小于等于
  TK_AND,  // 逻辑与
  TK_OR,  // 逻辑或

  TK_NUM, // 十进制和十六进制
  TK_REG,  // 寄存器变量
};

// 这个结构体定义了一个规则，包括一个正则表达式和它匹配的标记类型
static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  {" +", TK_NOTYPE},    // 空格，不作为标记

  {"\\(", '('}, {"\\)", ')'},  // 左括号和右括号
  {"\\*", '*'}, {"/", '/'},  // 乘法和除法
  {"\\+", '+'}, {"-", '-'},  // 加法和减法
  {"<", TK_LT}, {">", TK_GT}, {"<=", TK_LE}, {">=", TK_GE},  // 小于、大于、小于等于、大于等于
  {"==", TK_EQ}, {"!=", TK_NEQ},  // 等于、不等于
  {"&&", TK_AND},  // 逻辑与
  {"\\|\\|", TK_OR},  // 逻辑或

  {"(0x)?[0-9]+", TK_NUM},  // 十进制或者十六进制的数字
  {"\\$\\w+", TK_REG},  // $后跟随单词字符的寄存器变量
};


static bool oftypes(int type, int types[], int size) {
  for (int i = 0; i < size; i++) {
    if (type == types[i]) return true;
  }
  return false;
}
#define NR_REGEX ARRLEN(rules)
#define OFTYPES(type, types) oftypes(type, types, ARRLEN(types))
static int bound_types[] = {')',TK_NUM,TK_REG}; // boundary for binary operator
static int nop_types[] = {'(',')',TK_NUM,TK_REG}; // not operator type
static int op1_types[] = {TK_NEG, TK_POS, TK_DEREF}; // unary operator type


static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  // 遍历所有的正则表达式规则
  for (i = 0; i < NR_REGEX; i ++) {
    // 编译正则表达式，将文本形式的正则表达式转换为可以用于匹配的结构
    // 如果成功，ret为0，如果失败则为非零值
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    // 如果编译失败，将错误信息存入error_msg并报错
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}


// 定义一个词法分析单元结构体
typedef struct token {
  int type;         // 标记类型
  char str[32];     // 标记字符串
} Token;


static Token tokens[32] __attribute__((used)) = {};  // 存储标记的数组
static int nr_token __attribute__((used))  = 0;      // 记录标记数量的变量

// 寻找主运算符，该函数返回表示主要运算符位置的数组索引
// p和q是给定表达式的起始和结束的索引
static int find_major(int p, int q) {
  int ret = -1, par = 0, op_pre = 0;
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == '(') { // 如果当前标记是左括号，par++
      par++;
    } else if (tokens[i].type == ')') { // 如果当前标记是右括号且par为0，返回-1
      if (par == 0) {
        return -1;
      }
      par--;
    } else if (OFTYPES(tokens[i].type, nop_types)) { // 如果当前标记的类型是nop类型，继续下一个循环
      continue;
    } else if (par > 0) { // 如果par大于0，继续下一个循环
      continue;
    } else {
      int tmp_pre = 0;
      // 判断标记的类型，并附加运算优先级
      switch (tokens[i].type) {
      case TK_OR: tmp_pre++;
      case TK_AND: tmp_pre++;
      case TK_EQ: case TK_NEQ: tmp_pre++;
      case TK_LT: case TK_GT: case TK_GE: case TK_LE: tmp_pre++;
      case '+': case '-': tmp_pre++;
      case '*': case '/': tmp_pre++;
      case TK_NEG: case TK_DEREF: case TK_POS: tmp_pre++; break;
      default: return -1;
      }
      // 如果当前运算符的优先级高于之前保存的最高优先级，或者当前与上一个优先级相同且标记类型非op1_types
      // 更新保存的最高优先级，将主运算符的位置保存在ret中
      if (tmp_pre > op_pre || (tmp_pre == op_pre && !OFTYPES(tokens[i].type, op1_types))) {
        op_pre = tmp_pre;
        ret = i;
      }
    }
  }
  // 检查括弧是否匹配，如果不匹配返回-1
  if (par != 0) return -1;
  return ret;
}


static word_t eval_operand(int i, bool *ok) {
  switch (tokens[i].type) {
  case TK_NUM:
    if (strncmp("0x", tokens[i].str, 2) == 0) return strtol(tokens[i].str, NULL, 16); 
    else return strtol(tokens[i].str, NULL, 10);
  case TK_REG:
    return isa_reg_str2val(tokens[i].str, ok);
  default:
    *ok = false;
    return 0;
  }
}

// unary operator
static word_t calc1(int op, word_t val, bool *ok) {
  switch (op)
  {
  case TK_NEG: return -val;
  case TK_POS: return val;
  case TK_DEREF: return vaddr_read(val, 8);
  default: *ok = false;
  }
  return 0;
}

// binary operator
static word_t calc2(word_t val1, int op, word_t val2, bool *ok) {
  switch(op) {
  case '+': return val1 + val2;
  case '-': return val1 - val2;
  case '*': return val1 * val2;
  case '/': if (val2 == 0) {
    *ok = false;
    return 0;
  } 
  return (sword_t)val1 / (sword_t)val2; // e.g. -1/2, may not pass the expr test
  case TK_AND: return val1 && val2;
  case TK_OR: return val1 || val2;
  case TK_EQ: return val1 == val2;
  case TK_NEQ: return val1 != val2;
  case TK_GT: return val1 > val2;
  case TK_LT: return val1 < val2;
  case TK_GE: return val1 >= val2;
  case TK_LE: return val1 <= val2;
  default: *ok = false; return 0;
  }
}

bool check_parentheses(int p, int q) {
  if (tokens[p].type=='(' && tokens[q].type==')') {
    int par = 0;
    for (int i = p; i <= q; i++) {
      if (tokens[i].type=='(') par++;
      else if (tokens[i].type==')') par--;

      if (par == 0) return i==q; // the leftest parenthese is matched
    }
  }
  return false;
}


static word_t eval(int p, int q, bool *ok) {
  *ok = true;
  if (p > q) {
    *ok = false;
    return 0;
  } else if (p == q) {
    return eval_operand(p, ok);
  } else if (check_parentheses(p, q)) {
    return eval(p+1, q-1, ok);
  } else {    
    int major = find_major(p, q);
    if (major < 0) {
      *ok = false;
      return 0;
    }

    bool ok1, ok2;
    word_t val1 = eval(p, major-1, &ok1);
    word_t val2 = eval(major+1, q, &ok2);

    if (!ok2) {
      *ok = false;
      return 0;
    }
    if (ok1) {
      word_t ret = calc2(val1, tokens[major].type, val2, ok);
      return ret;
    } else {
      word_t ret =  calc1(tokens[major].type, val2, ok);
      return ret;
    }
  }
}


static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      int reg_res = regexec(&re[i], e + position, 1, &pmatch, 0);
      if (reg_res == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);
        
        position += substr_len;
        
        if (rules[i].token_type == TK_NOTYPE) break;

        tokens[nr_token].type = rules[i].token_type;
        nr_token++;
switch (rules[i].token_type) {
  case TK_NUM:
  case TK_REG:
    // todo: handle overflow (token exceeding size of 32B)
    strncpy(tokens[nr_token].str, substr_start, substr_len);
    tokens[nr_token].str[substr_len] = '\0';
    break;
  case '*': case '-': case '+':
    if (nr_token==0 || !OFTYPES(tokens[nr_token-1].type, bound_types)) {
      switch (rules[i].token_type)
      {
        case '-': tokens[nr_token].type = TK_NEG; break;
        case '+': tokens[nr_token].type = TK_POS; break;
        case '*': tokens[nr_token].type = TK_DEREF; break;
      }
    }
    break;
}

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}





 





word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  
  return eval(0, nr_token-1, success);;
}
