#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uthash.h"

typedef struct symbol {
    char name[50];
    char value[50];
    UT_hash_handle hh;
} Symbol;

Symbol* symbol_table = NULL;

void insert_symbol(const char* name, const char* value) {
    Symbol* symbol = malloc(sizeof(Symbol));
    strncpy(symbol->name, name, 49);
    strncpy(symbol->value, value, 49);
    HASH_ADD_STR(symbol_table, name, symbol);
}

const Symbol* get_symbol(const char* name) {
    Symbol* symbol = NULL;
    HASH_FIND_STR(symbol_table, name, symbol);
    return symbol;
}

void free_symbol_table() {
    Symbol* symbol, * tmp;
    HASH_ITER(hh, symbol_table, symbol, tmp) {
        HASH_DEL(symbol_table, symbol);
        free(symbol);
    }
}

int main() {
    insert_symbol("foo", "42");
    insert_symbol("bar", "123");

    const Symbol* foo_symbol = get_symbol("foo");
    if (foo_symbol) {
        printf("Foo symbol found: %s\n", foo_symbol->value);
    } else {
        printf("Foo symbol not found\n");
    }

    free_symbol_table();

    return 0;
}
