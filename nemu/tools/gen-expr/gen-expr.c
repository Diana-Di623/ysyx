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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
static int pos=0;
static int choose(int num)
{
  return rand()%num;
}
static void gen_num()
{
  char num[32];
  uint32_t value=rand()%10000000;
  int len=snprintf(num,sizeof(num),"%uu",value);
  if(len+pos>=sizeof(buf)||len<0)return;
  memcpy(buf+pos, num, len);
  pos+=len;
  buf[pos]='\0';
}
static void gen(char s)
{
  if(pos+1<sizeof(buf))
  {
  buf[pos]=s;
  pos++;
  }
  buf[pos]='\0';
}
static void gen_rand_op()
{
  char op[4]={'+','-','/','*'};
  gen(op[rand()%4]);
}
static void gen_rand_expr(int deep) {
  if(deep>10)
  {gen_num();
    return;
  }
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr(deep+1); gen(')'); break;
    default: gen_rand_expr(deep+1); gen_rand_op(); gen_rand_expr(deep+1); break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf[0]='\0';
    pos=0;
    gen_rand_expr(0);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -fsanitize=undefined "
      "-fno-sanitize-recover=undefined "
      "/tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    int status=pclose(fp);
    if (ret != 1 ||!WIFEXITED(status)||WEXITSTATUS(status) != 0) {
         continue;
      }
    printf("%u %s\n", result, buf);
  }
  return 0;
}
