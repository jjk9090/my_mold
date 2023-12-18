#ifndef HASHMAP_H
#define HASHMAP_H

#include "mold.h"
static inline Symbol* insert_symbol(Context *ctx, char* name, const char* value) {
    Symbol* symbol = (Symbol*)malloc(sizeof(Symbol));
    strncpy(symbol->name, name, 49);
    strncpy(symbol->value, value, 49);
    HASH_ADD_STR(ctx->symbol_map, name, symbol);
    return symbol;
}


static inline Symbol* get_symbol(Context *ctx,const char* name) {
    Symbol* symbol = NULL;
    HASH_FIND_STR(ctx->symbol_map, name, symbol);
    return symbol;
}

static inline void free_symbol_table(Context *ctx) {
    Symbol* symbol, * tmp;
    HASH_ITER(hh, ctx->symbol_map, symbol, tmp) {
        HASH_DEL(ctx->symbol_map, symbol);
        free(symbol);
    }
}


static inline void merger_sec_insert_element(my_hash_element *map,char *key, void *value/*u64 hash*/) {
    my_hash_element *element = (my_hash_element *)malloc(sizeof(my_hash_element));
    element->key = strdup(key);
    element->value = value;
    // element->hh.hashv = hash; 
    HASH_ADD(hh, map, key, sizeof(char *), element); // 添加元素
}

static inline void out_sec_insert_element(OutputSection_element *map,OutputSectionKey *key, void *value/*u64 hash*/) {
    // OutputSection_element *element = (OutputSection_element *)malloc(sizeof(OutputSection_element));
    // element->key = (OutputSectionKey *)malloc(sizeof(OutputSectionKey));
    // element->key = key;
    // element->value = value;
    // // element->hh.hashv = hash; 
    // HASH_ADD(hh, map, key, sizeof(OutputSectionKey *), element); // 添加元素
    OutputSection_element *element = (OutputSection_element *)malloc(sizeof(OutputSection_element));
    memset(element, 0, sizeof(OutputSection_element));
    element->key = (OutputSectionKey *)malloc(sizeof(OutputSectionKey));
    element->key = key;
    element->value = value;
    HASH_ADD(out, map, key, sizeof(OutputSectionKey *), element); // 添加元素
}

static inline my_hash_element *merger_sec_find_element(my_hash_element *map,char *key) {
    my_hash_element *element;
    HASH_FIND(hh,map, &key,sizeof(char *), element); // 使用 HASH_FIND_INT64 宏查找元素
    return element;
}

static inline OutputSection_element *out_sec_find_element(OutputSection_element *map,OutputSectionKey *key) {
    OutputSection_element *element;
    HASH_FIND(out,map, &key,sizeof(OutputSectionKey *), element); // 使用 HASH_FIND_INT64 宏查找元素
    return element;
}
// 从哈希表中删除指定的元素
static inline void merger_sec_delete_element(MergedSection *ms,my_hash_element *element) {
    HASH_DEL(ms->map, element); // 从哈希表中删除指定的元素
    free(element); // 释放内存
}
#endif