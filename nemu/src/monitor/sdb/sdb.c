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

#include <assert.h>
#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sdb.h"
#include "common.h"
#include "debug.h"
#include "memory/paddr.h"
#include "utils.h"

static int is_batch_mode = false;

void init_regex();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state=NEMU_QUIT;
  exit(0);
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","Single step ",cmd_si},
  {"info","Display some infomation",cmd_info},
  {"x","Scan memory",cmd_x},
  {"p","Evaluate the expression",cmd_p},
  {"w","Watchpoint",cmd_w},
  {"d","Delete watchpoint",cmd_d}
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_si(char *args)
{
  char *arg = strtok(NULL, " ");//只有一个参数第二次读返回null
  if(arg==NULL)cpu_exec(1);
  else{
    cpu_exec(atoi(arg));
  }
  return 0;
}

static int cmd_info(char *args)
{
  args=strtok(NULL, " ");
  if(args==NULL){printf("Usage:info w/r");
  assert(0);
  }
  if(*args=='r')
  {
     isa_reg_display();
  }
  if(*args=='w')
  {
    wp_display();
  }
  return 0;
}

static int cmd_x(char *args)
{ char *len=strtok(NULL, " ");
  args=strtok(NULL, " ");
  uint32_t addr=strtol(args,NULL,16);
  uint32_t N=strtol(len,NULL,10);
   for(int i=0;i<N;i++)
  {
    word_t mem=paddr_read(addr+i*4, 4);
    printf("0x%08x\t",mem);
    if((i+1)%4==0){
    printf("\n");
  }
}
  printf("\n");
  return 0;
}
static int cmd_p(char *args)
{ if(args==NULL){printf("Usage:p <expression>\n");
  assert(0);
  }
  bool success;
  word_t ret=expr(args,&success);
  if(success==false)printf("expr fail!\n");
  printf("0x%x\n",ret);
  return 0;
}
static int cmd_w(char *args)
{ bool success;
  if(args==NULL){printf("Usage:w <expression>");
  assert(0);}
  word_t ret=expr(args, &success);
  if(success==false)printf("expr fail!");
  else{
    create_watchpoint(args,ret);
  }
  return 0;
}
static int cmd_d(char *args){
   if(args==NULL){printf("Usage:d <expression>");
  assert(0);}
  uint32_t NO=strtol(args,NULL,10);
  delete_watchpoint(NO);
  return 0;
}
static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");//char *strtok(char *str, const char *delim);delim:分隔符，第一次调用str传字符传，后面NULL
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
void test_expr()
{
  FILE* fp=fopen("/home/kirin/Desktop/linux/ysyx-workbench/nemu/tools/gen-expr/input","r");
  if(fp==NULL)perror("can't load file");
  word_t right_res;
  char *e=NULL;
  size_t len=0;
  bool success=false;
  while(1){
  if(fscanf(fp,"%u",&right_res)==-1)break;
  int read=getline(&e,&len,fp);
  e[read-1]='\0';
  word_t res=expr(e,&success);
  assert(success);
  if(res!=right_res)
  {
    printf("expect:%u,get:%u",right_res,res);
    assert(0);
  }
  }
  printf("pass the expr test!\n");
}
void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();
  //test_expr();
  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
