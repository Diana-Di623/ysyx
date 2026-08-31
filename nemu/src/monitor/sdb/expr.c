/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
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

#include "common.h"
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdbool.h>
#include <string.h>

enum {
  TK_NOTYPE = 256, TK_EQ,TK_NEQ,TK_LE,TK_GE,TK_AND,TK_OR,TK_NUM,TK_REG,TK_NEG

  /* TODO: Add more token types */

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
  {"-",'-'},             //sub
  {"\\*",'*'},          //multi
  {"/",'/'},            //div
  {"==", TK_EQ},        // equal
  {"!=",TK_NEQ},        //not equal
  {"<=",TK_LE},         //less equal
  {">=",TK_GE},         //greater equal
  {"&&",TK_AND},        //logical and
 {"\\|\\|",TK_OR},         //logical or
 {"\\(",'('},            //left brace
 {"\\)",')'},            //right brace
 {"[0-9]+u?",TK_NUM},    //number
 {"\\$[a-zA-Z0-9]",TK_REG}//register
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

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[512] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
         case TK_NOTYPE:
            break;
          case '-':
            bool is_neg=false;
            if(nr_token==0)
              is_neg=true;
            else{
              int pre_token_type=tokens[nr_token-1].type;
              if(pre_token_type=='('||pre_token_type=='*'||pre_token_type=='-'||pre_token_type=='/'||pre_token_type=='+'||pre_token_type==TK_EQ||pre_token_type==TK_AND||pre_token_type==TK_GE||pre_token_type==TK_LE||pre_token_type==TK_NEQ||pre_token_type==TK_OR||pre_token_type==TK_NEG)
              {
                is_neg=true;
              }
            }
             tokens[nr_token].type=(is_neg==false)?'-':TK_NEG;
             strncpy(tokens[nr_token].str, substr_start, substr_len);
             tokens[nr_token].str[substr_len] = '\0';
             nr_token++;
              break;
          default: 
          {
            tokens[nr_token].type=rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          }
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
  /* TODO: Insert codes to evaluate the expression. */
int check_parentheses(int p,int q)
 {if(tokens[p].type=='('&&tokens[q].type==')')
  {
    int pair=0;
    for(int i=p;i<=q;i++)
    {
        if(tokens[i].type=='(') pair--;
        else if (tokens[i].type==')') pair++;
        if (pair==0)return i==q;
    }
  }
  return false;
 }
 int find_main_op(int p,int q)
 {
    int pairs=0;
    int priority=0;
    int ret=-1;
  for(int i=p;i<=q;i++)
  {
    if(tokens[i].type==TK_NUM || tokens[i].type==TK_REG)
    {
      continue;
    }
    if(tokens[i].type=='(')pairs++;
    else if(tokens[i].type==')')
    {
        if(pairs==0)return -1;
        pairs--;
    }
    else if(pairs>0)continue;
    else{
      if(tokens[i].type=='+'||tokens[i].type=='-')
     { priority=1;
      ret=i;
     }
     if(tokens[i].type=='*'||tokens[i].type=='/')
     {
      if(priority!=1)ret=i;
     }
    }
  }
   if(pairs!=0)return -1;
   return ret;
 }
word_t eval(int p, int q, bool *success) {
  if (p > q) {
    /* Bad expression */
    *success=false;
    return 0;
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if(tokens[p].type==TK_NUM)
    {
     word_t value=strtol(tokens[p].str,0,10);
     return value;
    }
    if (tokens[p].type==TK_REG) {
     word_t value=isa_reg_str2val(tokens[p].str, success);
     return value;
    }

     *success=false;
      return 0;
  }
  else if(tokens[p].type==TK_NEG){
    word_t value=eval(p+1,q, success);
    return -value;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1,success);
  }
  else {
    word_t op = find_main_op(p,q);
    word_t val1 = eval(p, op - 1,success);
    word_t val2 = eval(op + 1, q,success);

    switch (tokens[op].type) {
      case '+': return val1 + val2;
      case '-': return val1-val2;
      case '*': return val1*val2;
      case '/': 
      if(val2==0)
      {
        *success=false;
        return 0;
      }
      return val1/val2;
      default: assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  *success=true;
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  return eval(0,nr_token-1,success);
}
