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
    // For range extension thunks
    i16 thunk_idx;
    i16 thunk_sym_idx;
} SymbolExtras;
typedef struct {
    /* data */
    ObjectFile *file;
    uintptr_t origin;
    u64 value;
    char *nameptr;
    i32 namelen;

    u8 flags;
    SymbolExtras extra;
    // Index into the symbol table of the owner file.
    i32 sym_idx;
    i32 aux_idx;

    u16 ver_idx;
    bool is_weak : 1;
    bool write_to_symtab : 1;
    bool is_traced : 1;
    bool is_wrapped : 1;

    bool is_imported : 1;
    bool is_exported : 1;
    bool is_canonical : 1;

    bool has_copyrel : 1;
    bool is_copyrel_readonly : 1;
    bool referenced_by_regular_obj : 1;
    bool address_taken : 1;

    UT_hash_handle hh; 
} ELFSymbol;

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