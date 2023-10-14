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


// 一个函数，检查一个特定的整型"类型"是否在一个特定的整型数组"类型"中
// type：待检查的整型值
// types[]：整型值数组
// size：数组的大小
static bool oftypes(int type, int types[], int size) {
  for (int i = 0; i < size; i++) {
    // 如果待检查的整型值在数组中，函数返回true
    if (type == types[i]) return true;
  }
  // 如果待检查的整型值不在数组中，函数返回false
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
    } else if (OFTYPES(tokens[i].type, nop_types)) { // 如果当前标记的类型是正负解引用类型，继续下一个循环
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


// 定义一个名为eval_operand的函数。这个函数接收两个参数：一个整数i和一个布尔指针ok。
// 函数返回一个名为word_t的数据类型的值，该数据类型未在代码中定义，可能已在其他地方定义。
static word_t eval_operand(int i, bool *ok) { 

  // 根据第i个令牌的类型执行相应的操作
  switch (tokens[i].type) { 

  // 如果令牌类型为TK_NUM（可能表示这是一个数字）
  case TK_NUM:

    // 判断字符串的前两个字符是否为 '0x'，这通常表示一个十六进制数
    if (strncmp("0x", tokens[i].str, 2) == 0) return strtol(tokens[i].str, NULL, 16); 

    // 否则，将字符串转换为十进制数
    else return strtol(tokens[i].str, NULL, 10);

  // 如果令牌类型为TK_REG（可能表示这是一个寄存器）
  case TK_REG:

    // 将字符串值转换为寄存器对应的数值
    return isa_reg_str2val(tokens[i].str, ok);

  // 对于不满足以上几种情况的令牌
  default:

    // 将ok设置为false，表示失败
    *ok = false;
    
    // 返回0
    return 0;
  }
}

// 定义一个处理一元操作的函数。函数接收一个操作符op、一个值val以及一个布尔指针ok
static word_t calc1(int op, word_t val, bool *ok) {
  
  // 根据操作符对值执行对应的操作
  switch (op) {

  // 如果是取负操作（TK_NEG可能代表取负），返回值的负数
  case TK_NEG: return -val;

  // 如果是取正操作（TK_POS可能代表取正），返回原值
  case TK_POS: return val;

  // 如果是解引用操作（TK_DEREF可能代表解引用），读取地址为val，大小为8字节的内存值
  case TK_DEREF: return vaddr_read(val, 4);

  // 对于其他未列出操作，将ok设置为false，表示失败
  default: *ok = false;
  }

  // 默认返回0
  return 0;
}

// 定义一个处理二元操作的函数。函数接收两个值val1和val2、一个操作符op以及一个布尔指针ok
static word_t calc2(word_t val1, int op, word_t val2, bool *ok) {
  
  switch(op) {
    
  case '+': return val1 + val2;   // 加法操作
  case '-': return val1 - val2;   // 减法操作
  case '*': return val1 * val2;   // 乘法操作
  case '/':                       // 除法操作
    if (val2 == 0) {              // 检查除数是否为0，如果为0则会导致除法错误
      *ok = false;                // 将ok设置为false，表示失败
      return 0;                   // 返回0
    } 
    return (sword_t)val1 / (sword_t)val2; // 如果除数不为0，进行除法操作
                                          // 注意强制性的类型转换，可能是为了保证结果精度或者进行符号扩展
  case TK_AND: return val1 && val2;                    // 逻辑与操作
  case TK_OR: return val1 || val2;                     // 逻辑或操作
  case TK_EQ: return val1 == val2;                     // 等于操作
  case TK_NEQ: return val1 != val2;                    // 不等于操作
  case TK_GT: return val1 > val2;                      // 大于操作
  case TK_LT: return val1 < val2;                      // 小于操作
  case TK_GE: return val1 >= val2;                     // 大于等于操作
  case TK_LE: return val1 <= val2;                     // 小于等于操作
  default: *ok = false; return 0;                      // 对于其他未列出操作，将ok设置为false，表示失败
                                                        // 并返回0
  }
}


bool check_parentheses(int p, int q){
	int a;
	int j = 0, k = 0;
	if (tokens[p].type == '(' || tokens[q].type == ')'){
		for (a = p; a <= q; a++){
			if (tokens[a].type == '('){
				j++;
			}
			if (tokens[a].type == ')'){
				k++;
			}
			if (a != q && j == k){
				return false;
			}
		}
		if (j == k){
				return true;
			} else {
				return false;
			}
	}
	return false;
}


static word_t eval(int p, int q, bool *ok) {
  *ok = true;
  if (p > q) {
    // 如果子表达式无效（比如空或开始索引大于结束索引），返回错误
    *ok = false;
    return 0;
  } else if (p == q) {
    // 如果子表达式只有一个字符，直接对其求值
    return eval_operand(p, ok); 
  } else if (check_parentheses(p, q)) {
    // 如果子表达式是一个带有配对括号的有效表达式，去掉括号后继续求值
    return eval(p+1, q-1, ok);
  } else {    
    // 寻找子表达式中运算优先级最低的运算符（如果存在的话）
    int major = find_major(p, q);
    if (major < 0) {
      // 如果子表达式中没有合法的运算符，返回错误
      *ok = false;
      return 0;
    }

    // 对运算符左边的子表达式进行求值
    bool ok1, ok2;
    word_t val1 = eval(p, major-1, &ok1);
    // 对运算符右边的子表达式进行求值
    word_t val2 = eval(major+1, q, &ok2);

    if (!ok2) {
      // 如果右边的子表达式不能正确求值，返回错误
      *ok = false;
      return 0;
    }
    if (ok1) {
      // 如果左边的子表达式也能正确求值，对两个子表达式进行相应的二元运算，返回结果
      word_t ret = calc2(val1, tokens[major].type, val2, ok);
      return ret;
    } else {
      // 如果左边的子表达式不能正确求值，对右边的子表达式进行一元运算，返回结果
      word_t ret =  calc1(tokens[major].type, val2, ok);
      return ret;
    }
  }
}



// 定义一个函数，输入参数是一个字符指针，返回一个布尔值
static bool make_token(char *e) {
  // 定义并初始化位置变量
  int position = 0;
  int i;
  regmatch_t pmatch;

  // 初始化词法标记数量为0
  nr_token = 0;
  // 当遇到字符串结束符（'\0'）时停止循环
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    // 尝试所有的规则
    for (i = 0; i < NR_REGEX; i ++) {
      // 使用正则表达式库函数regexec判断当前字符是否符合规则
      int reg_res = regexec(&re[i], e + position, 1, &pmatch, 0);
      // 如果匹配且匹配的起始位置为0，执行以下代码
      if (reg_res == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // 采用注释的这段代码用于打印一些调试信息
        
        // 当前位置前进匹配的字符串长度
        position += substr_len;
        
        // 如果当前规则的token_type是TK_NOTYPE，则跳过此次循环
        if (rules[i].token_type == TK_NOTYPE) break;

        // 取出还没被赋值的token，将其类型设置为匹配到的类型
        tokens[nr_token].type = rules[i].token_type;
        // 匹配到的token数目加一
        nr_token++;

        // 根据不同的token_type做不同的处理
        switch (rules[i].token_type) {
          case TK_NUM:
          case TK_REG:
            // 如果token类型是TK_NUM或TK_REG，将匹配到的字符串复制到token.str中
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            break;
          case '*': case '-': case '+':
            // 如果当前token是第一个token或之前的token的类型是bound_types，则将这些token的类型做更改
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

    // 如果遍历所有的规则后都没有匹配的，打印错误信息并返回false
    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  // 如果处理完所有字符，返回true
  return true;
}





// 函数 'expr' 接收一个字符指针 'e' 和一个 bool 指针 'success' 作为输入
// 'e' 可能是一个运算表达式的字符串形式，例如 "2+3*4"
// 'success' 用于让该函数可以将运算成功与否的信息传回调用它的函数
word_t expr(char *e, bool *success) {
  
  // 'make_token' 函数可能是用于将输入的表达式字符串 'e' 转换为一种内部表示（如标记序列）
  // 如果转换失败（例如，输入不是一个有效的表达式），'make_token' 将返回 false
  if (!make_token(e)) {
    // 如果'tokenise'返回false，则在'*success'中存放false（表示解析失败），并返回0作为函数结果
    *success = false;
    return 0;
  }

  return eval(0, nr_token-1, success);
}

