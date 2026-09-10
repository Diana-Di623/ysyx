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
#include "sdb.h"
#include <assert.h>
#include <stdbool.h>

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  char expr[128];
  word_t old_value;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

static WP* new_wp() {
  assert(free_);
  WP* ret = free_;
  free_ = free_->next;
  ret->next = head;
  head = ret;
  return ret;
}
void free_wp(WP* wp){
  if(wp==NULL){
    return;
  }
  wp->next = free_;
  free_ = wp;
}

void wp_display(){
  WP* temp=head;
  if(temp==NULL)
  {printf("No watchpoint available\n");
    return;
  }
  else{
    while(temp){
      printf("%d:%-8s\n",temp->NO,temp->expr);
      temp=temp->next;
    }
  }
}
void create_watchpoint(char *args, word_t ret){
  WP* new=new_wp();
  strcpy(new->expr,args);
  new->old_value=ret;
  printf("create watchpoint %d:%s\n",new->NO,new->expr);
}
void delete_watchpoint(uint32_t NO){
  assert(NO<NR_WP);
  WP* del=&wp_pool[NO];
  printf("delete watchpoint %d:%s\n",del->NO,del->expr);
  free_wp(del);
}
void watchpoint_diff(bool *change){
  WP* temp=head;
  bool success;
  word_t new_value;
  while(temp){
    new_value=expr(temp->expr, &success);
    if(success==false)assert(0);
    if(temp->old_value!=new_value)
    {
       printf("Watchpoint %d: %s\nOld value = %u,New value = %u\n",temp->NO, temp->expr, temp->old_value, new_value);
       temp->old_value=new_value;
       *change=true;
    }
    temp=temp->next;
  }
}