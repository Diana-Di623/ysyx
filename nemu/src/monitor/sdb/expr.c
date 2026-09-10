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
#include <memory/paddr.h>
enum {
  TK_NOTYPE = 256, TK_EQ,TK_NEQ,TK_LE,TK_GE,TK_AND,TK_OR,TK_NUM,TK_REG,TK_NEG,TK_HNUM,TK_POINTER,TK_L,TK_G
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
  {"<",TK_L},           //less
  {">",TK_G},       //greater
  {">=",TK_GE},         //greater equal
  {"&&",TK_AND},        //logical and
 {"\\|\\|",TK_OR},         //logical or
 {"\\(",'('},            //left brace
 {"\\)",')'},            //right brace
 {"0[xX][0-9a-f]+",TK_HNUM},
 {"[0-9]+u?",TK_NUM},    //number
 {"\\$[a-zA-Z0-9]+",TK_REG}//register
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
        if(rules[i].token_type!=TK_NOTYPE)
        {         
          int pre_token_type=tokens[nr_token-1].type;
        switch (rules[i].token_type) {
          case '-':
            bool is_neg=false;
            if(nr_token==0)
              is_neg=true;
            else{
     
              if(pre_token_type=='('||pre_token_type=='*'||pre_token_type=='-'||pre_token_type=='/'||pre_token_type=='+'||pre_token_type==TK_EQ||pre_token_type==TK_AND||pre_token_type==TK_GE||pre_token_type==TK_LE||pre_token_type==TK_NEQ||pre_token_type==TK_OR||pre_token_type==TK_NEG||pre_token_type==TK_L||pre_token_type==TK_G)
              {
                is_neg=true;
              }
            }
             tokens[nr_token].type=(is_neg==false)?'-':TK_NEG;
              break;
          case'*':
              bool is_pointer=false;
            if(nr_token==0)
              is_pointer=true;
            else{
              if(pre_token_type=='('||pre_token_type=='*'||pre_token_type=='-'||pre_token_type=='/'||pre_token_type=='+'||pre_token_type==TK_EQ||pre_token_type==TK_AND||pre_token_type==TK_GE||pre_token_type==TK_LE||pre_token_type==TK_NEQ||pre_token_type==TK_OR||pre_token_type==TK_NEG||pre_token_type==TK_L||pre_token_type==TK_G)
              {
                is_pointer=true;
              }
            }
             tokens[nr_token].type=(is_pointer==false)?'*':TK_POINTER;
              break;
          default: 
          {
            tokens[nr_token].type=rules[i].token_type;
            break;
          }
        }
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len] = '\0';
          nr_token++;
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
 {  int priority=9;
    int pairs=0;
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
      if(tokens[i].type==TK_OR)
      {
        ret=i;
        priority=0;
      }
      if((tokens[i].type==TK_AND)&&priority>0)
      {
        ret=i;
        priority=1;
      }
      if((tokens[i].type==TK_EQ||tokens[i].type==TK_NEQ)&&priority>1)
      { 
        ret=i;
        priority=2;
      }
      if((tokens[i].type==TK_L||tokens[i].type==TK_G||tokens[i].type==TK_GE||tokens[i].type==TK_LE)&&priority>2)
      {
        ret=i;
        priority=3;
      }
      if((tokens[i].type=='+'||tokens[i].type=='-')&&priority>3)
     { 
      ret=i;
      priority=4;
     }
     if((tokens[i].type=='*'||tokens[i].type=='/')&&priority>4)
     {
      ret=i;
      priority=5;
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
     word_t value=strtol(tokens[p].str,NULL,10);
     return value;
    }
    if(tokens[p].type==TK_HNUM)
     {
     word_t value=strtol(tokens[p].str,NULL,16);
     return value;
    }
    if (tokens[p].type==TK_REG) {
     word_t value=isa_reg_str2val(tokens[p].str, success);
     return value;
    }

     *success=false;
      return 0;
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1,success);
  }
  else {
    word_t op = find_main_op(p,q);
        if (op == -1) {
      if (tokens[p].type == TK_NEG) {
        return -eval(p + 1, q, success);
      }

      if (tokens[p].type == TK_POINTER) {
        word_t addr = eval(p + 1, q, success);
        return paddr_read(addr, 4);
      }

      *success = false;
      return 0;
    }

    word_t val1 = eval(p, op - 1,success);
    word_t val2 = eval(op + 1, q,success);

    switch (tokens[op].type) {
      case TK_OR: return val1||val2;
      case TK_EQ: return val1==val2;
      case TK_NEQ:return val1!=val2;
      case TK_AND:return val1&&val2;
      case TK_L:return val1>val2;
      case TK_GE:return val1>=val2;
      case TK_LE:return val1<=val2;
      case TK_G:return val1<val2;
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
