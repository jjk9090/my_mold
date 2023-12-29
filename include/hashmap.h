#ifndef HASHMAP_H
#define HASHMAP_H
#include "mold.h"
static inline ELFSymbol * insert_symbol(Context *ctx,char *nameptr, char *value/*u64 hash*/) {
    // element->hh.hashv = hash; 
    ELFSymbol *sym = (ELFSymbol *)malloc(sizeof(ELFSymbol));
    sym->nameptr = value;
    sym->namelen = strlen(value);
    HASH_ADD(hh, ctx->symbol_map, nameptr, sizeof(char *), sym); // 添加元素
    return sym;
}
static inline ELFSymbol* get_symbol(Context *ctx,const char* name) {
    ELFSymbol* symbol = NULL;
    HASH_FIND_STR(ctx->symbol_map, name, symbol);
    return symbol;
}

static inline void free_symbol_table(Context *ctx) {
    ELFSymbol *symbol, * tmp;
    HASH_ITER(hh, ctx->symbol_map, symbol, tmp) {
        HASH_DEL(ctx->symbol_map, symbol);
        free(symbol);
    }
}


static inline void merger_sec_insert_element(MergedSection *sec,char *key, void *value/*u64 hash*/) {
    my_hash_element *element = (my_hash_element *)malloc(sizeof(my_hash_element));
    element->key = strdup(key);
    element->value = value;
    // element->hh.hashv = hash; 
    HASH_ADD(hh, sec->map, key, sizeof(char *), element); // 添加元素
}

static inline SectionFragment *merger_sec_find_element(my_hash_element *map,char *key) {
    my_hash_element *element = NULL;
    HASH_FIND(hh,map, &key,sizeof(char *), element); // 使用 HASH_FIND_INT64 宏查找元素
    return element->value;
}

// 从哈希表中删除指定的元素
static inline void merger_sec_delete_element(MergedSection *ms,my_hash_element *element) {
    HASH_DEL(ms->map, element); // 从哈希表中删除指定的元素
    free(element); // 释放内存
}
#endif