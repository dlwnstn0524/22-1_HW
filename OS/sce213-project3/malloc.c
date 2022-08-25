/**********************************************************************
 * Copyright (c) 2020
 *  Jinwoo Jeong <jjw8967@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdint.h>

#include "malloc.h"
#include "types.h"
#include "list_head.h"

#define ALIGNMENT 32
#define HDRSIZE sizeof(header_t)

static LIST_HEAD(free_list); // Don't modify this line
static algo_t g_algo;        // Don't modify this line
static void *bp;             // Don't modify this line

/***********************************************************************
 * extend_heap()
 *
 * DESCRIPTION
 *   allocate size of bytes of memory and returns a pointer to the
 *   allocated memory.
 *
 * RETURN VALUE
 *   Return a pointer to the allocated memory.
 */
void *my_malloc(size_t size){
  /* Implement this function */
  header_t *temp;
  header_t *entry;
  int a;
  int flag = 0;
  if(size % ALIGNMENT == 0){
    size = size;
  } else {
    size  = ((size / ALIGNMENT) + 1)* ALIGNMENT;
  }
  
  if(g_algo == FIRST_FIT){ // Use alloc algo to FIRST_FIT
    list_for_each_entry(temp, &free_list, list){ // Find empty entry for FIRST_FIT algo.
      if(temp->free == true && temp->size >= size){ // Find first entry that can be allocated.
        flag = 1;                                           // Set flag for find first fit space.
        break;
      }
    }
    if(flag == 0){ // 1. No space for entry   2. First entry to enter
      if(list_last_entry(&free_list, header_t, list)->free == true){
        //fprintf(stderr, "Got you!\n");
        entry = sbrk(size + HDRSIZE);
        entry->size = size;
        entry->free = false;
        temp = list_last_entry(&free_list, header_t, list);
        __list_add(&entry->list, temp->list.prev, &temp->list);
        return entry;
      } else {
        entry = sbrk(size + HDRSIZE);
        entry->size = size;
        entry->free = false;
        list_add_tail(&entry->list, &free_list);
        return entry;
      }      
    } else { //Find first fit space to allocate.
      if(temp->size == size){
        temp->free = false;
        return temp;
      }
      else {
        a = temp->size;
        temp->size = size;
        temp->free = false;
        entry = temp - size - 20;
        entry->free = true;
        entry->size = a - temp->size - HDRSIZE;
        list_add(&entry->list, &temp->list);
        return temp;
      }
    }
  }
  else if(g_algo == BEST_FIT){
    long unsigned int remain = SIZE_MAX;
    header_t *entry;
    list_for_each_entry(temp, &free_list, list){ // Find Best location to allocate
      if(temp->free == true && temp->size >= size && remain > temp->size - size){
        flag = 1;
        remain = temp->size - size;
      }
    }
    if(flag == 0){ // 1. No space for entry   2. First entry to enter
      entry = sbrk(size + HDRSIZE);
      entry->size = size;
      entry->free = false;
      if(list_last_entry(&free_list, header_t, list)->free == true){
        temp = list_last_entry(&free_list, header_t, list);
        __list_add(&entry->list, temp->list.prev, &temp->list);
        return entry;
      } else {
        list_add_tail(&entry->list, &free_list);
        return entry;
      }  
    }
    else { // Find Best fit space to allocate.
      
      list_for_each_entry(temp, &free_list, list){
        if(remain == temp->size - size && temp->free == true){
          break;
        }
      }
      if(temp->size == size){
        //list_replace(&temp->list, &entry->list);
        temp->free = false;
        return temp;
      }
      else {
        a = temp->size;
        temp->size = size;
        temp->free = false;
        entry = temp - size - 32;
        entry->free = true;
        entry->size = a - temp->size - HDRSIZE;
        list_add(&entry->list, &temp->list);
        return temp;
      }
    }
  }
  return NULL;
}

/***********************************************************************
 * my_realloc()
 *
 * DESCRIPTION
 *   tries to change the size of the allocation pointed to by ptr to
 *   size, and returns ptr. If there is not enough memory block,
 *   my_realloc() creates a new allocation, copies as much of the old
 *   data pointed to by ptr as will fit to the new allocation, frees
 *   the old allocation.
 *
 * RETURN VALUE
 *   Return a pointer to the reallocated memory
 */
void *my_realloc(void *ptr, size_t size)
{
  /* Implement this function */
  header_t *entry;
  header_t *exist = ptr;
  if(exist->size == size){ // 기존 데이터와 재할당하는 데이터 크기가 같을 경우
    entry = ptr;
    entry->size = size;
    entry->free = false;
    list_replace(&exist->list, &entry->list);
  }
  else if(exist->size < size) { // 기존 데이터보다 재할당하는 데이터의 크기가 더 클 경우
    entry = my_malloc(size);
    my_free(ptr);
  }
  return entry;
}

/***********************************************************************
 * my_realloc()
 *
 * DESCRIPTION
 *   deallocates the memory allocation pointed to by ptr.
 */
void my_free(void *ptr)
{
  /* Implement this function */
  header_t *target = ptr;
  
  target->free =true;
  if (list_next_entry(target, list)->free == true) {
      target->size = target->size + list_next_entry(target, list)->size + HDRSIZE;
      list_del(&list_next_entry(target, list)->list);
    }
    if (list_prev_entry(target, list)->free == true) {
      target->size = target->size + list_prev_entry(target, list)->size + HDRSIZE;
      list_del(&list_prev_entry(target, list)->list);
    }
  return;
}

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
/*          ****** EXCEPT TO mem_init() AND mem_deinit(). ******      */
void mem_init(const algo_t algo)
{
  g_algo = algo;
  bp = sbrk(0);
}

void mem_deinit()
{

  header_t *header;
  size_t size = 0;
  list_for_each_entry(header, &free_list, list) {
    size += HDRSIZE + header->size;
  }
  sbrk(-size);

  if (bp != sbrk(0)) {
   fprintf(stderr, "[Error] There is memory leak\n");  
  }
}

void print_memory_layout()
{

  header_t *header;
  int cnt = 0;

  printf("===========================\n");
  list_for_each_entry(header, &free_list, list) {
    cnt++;
    printf("%c %ld\n", (header->free) ? 'F' : 'M', header->size);
  }

  printf("Number of block: %d\n", cnt);
  printf("===========================\n");
  return;
}