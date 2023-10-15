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
  TK_NOTYPE = 256, TK_EQ,TK_NEG,
  TK_NUM, // 10 & 16
  TK_HEX,
  TK_REG,
  TK_VAR,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"==", TK_EQ},        // equal
  {"\\(", '('},
  {"\\)", ')'},
  {"0x[A-Fa-f0-9]+", TK_HEX}, 
  {"[0-9]+", TK_NUM}, // TODO: non-capture notation (?:pattern) makes compilation failed
  {"\\$\\w+", TK_REG},
  {"[A-Za-z_]\\w*", TK_VAR},
};

#define NR_REGEX ARRLEN(rules)

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

int find_major(int p, int q) {
  int ret = -1, par = 0, op_type = 0;
  for (int i = p; i <= q; i++) {
      if (i != p && tokens[i].type == '-' && 
          !(tokens[i-1].type == TK_NUM || tokens[i-1].type == TK_HEX || 
            tokens[i-1].type == ')')) {
          tokens[i].type = TK_NEG;
      }
  }
  
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_NUM || tokens[i].type == TK_HEX) {
      continue;
    }
    if (tokens[i].type == '(') {
      par++;
    } else if (tokens[i].type == ')') {
      if (par == 0) {
        return -1;
      }
      par--;
    } else if (par > 0) {
      continue;
    } else {
      int tmp_type = 0;
      switch (tokens[i].type) {
      case '*': case '/': tmp_type = 1; break;
      case '+': case '-': tmp_type = 2; break;
      case TK_NEG: tmp_type = 3; break;
      default: 
    printf("Unexpected token type: %d\n",tokens[i].type);
    assert(0);
	break;
      }
      if (tmp_type >= op_type) {
        op_type = tmp_type;
        ret = i;
      }
    }
  }
  if (par != 0) return -1;
  return ret;
}


word_t eval(int p, int q, bool *ok) {
  *ok = true;
  if (p > q) {
    *ok = false;
    return 0;
  } else if (p == q) {
    switch (tokens[p].type) {
      case TK_NUM:
        return strtol(tokens[p].str, NULL, 10);   
      case TK_HEX:
        return strtol(tokens[p].str, NULL, 16);
      case TK_NEG:
        return -strtol(tokens[p].str+1, NULL, 10);  // When token is negative number
      default:
        *ok = false;
        return 0;
    }
    word_t ret = strtol(tokens[p].str, NULL, 10);
    return ret;
  } else if (check_parentheses(p, q)) {
    return eval(p+1, q-1, ok);
  } else {    
    int major = find_major(p, q);
    if (major < 0) {
      *ok = false;
      return 0;
    }

    word_t val1 = eval(p, major-1, ok);
    if (!*ok) return 0;
    word_t val2 = eval(major+1, q, ok);
    if (!*ok) return 0;
    
    switch(tokens[major].type) {
     case TK_NEG: return -eval(p, major, ok);
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': if (val2 == 0) {
        *ok = false;
        return 0;
      } 
      return (sword_t)val1 / (sword_t)val2; // e.g. -1/2, may not pass the expr test
      default: assert(0);
    }
  }
}




// 定义一个函数，输入参数是一个字符指针，返回一个布尔值
// 定义一个函数，输入参数是一个字符指针，返回一个布尔值
static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    // deal with negative number
    if(e[position] == '-') {
    tokens[nr_token].str[0] = e[position]; 
    tokens[nr_token].str[1] = '\0'; 
    if (e[position+1] == '-') { // check if immediate next character is also a minus
      tokens[nr_token].type = TK_NEG;
      position += 2;
    } else {
      tokens[nr_token].type = '-';
      position++;
    }
    nr_token++;
    continue;
  }
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      int reg_res = regexec(&re[i], e + position, 1, &pmatch, 0);
      if (reg_res == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
  
        // skip the whitespace
        if (rules[i].token_type != TK_NOTYPE) {
          // copy to tokens here
          tokens[nr_token].type = rules[i].token_type;
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
          nr_token++;
        }
        position += substr_len;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;  // tokens cannot be recognized
    }
  }

  return true;  // tokenize successfully
}








word_t expr(char *e, bool *success) {
  
  // 'make_token' 函数可能是用于将输入的表达式字符串 'e' 转换为一种内部表示（如标记序列）
  // 如果转换失败（例如，输入不是一个有效的表达式），'make_token' 将返回 false
  if (!make_token(e)) {
    // 如果'tokenise'返回false，则在'*success'中存放false（表示解析失败），并返回0作为函数结果
    *success = false;
    return 0;
  }

  return eval(0, nr_token-1, success);;
}

