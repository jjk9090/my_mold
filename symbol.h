#ifndef SYMBOL_H  // 头文件保护，防止重复包含
#define SYMBOL_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mold.h"

typedef struct  {
    /* data */
    char name[50];
    char value[50];
    size_t namelen;
    UT_hash_handle hh; 
} Symbol;

typedef struct {
    char *key;  // 键是 i64 类型
    void *value;  // 值是 void* 类型
    u64 hash;
    UT_hash_handle hh;
} my_hash_element;

typedef struct {
    OutputSectionKey *key;  
    void *value;  // 值是 void* 类型
    UT_hash_handle out;
} OutputSection_element;
#endif