#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uthash.h"

typedef struct my_hash_element {
    char *key;
    void *value;
    UT_hash_handle hh; // 哈希表链接字段
} my_hash_element;

int main() {
    my_hash_element *map = NULL;
    my_hash_element *element;
    char *key1 = "key1";
    char *key2 = "key2";
    char *key3 = "key3";
    int value1 = 1;
    int value2 = 2;
    int value3 = 3;
    
    // 添加元素到哈希表中
    element = (my_hash_element *)malloc(sizeof(my_hash_element));
    element->key = strdup(key1);
    element->value = &value1;
    HASH_ADD_KEYPTR(hh, map, element->key, strlen(element->key), element);
    
    element = (my_hash_element *)malloc(sizeof(my_hash_element));
    element->key = strdup(key2);
    element->value = &value2;
    HASH_ADD_KEYPTR(hh, map, element->key, strlen(element->key), element);
    
    element = (my_hash_element *)malloc(sizeof(my_hash_element));
    element->key = strdup(key3);
    element->value = &value3;
    HASH_ADD_KEYPTR(hh, map, element->key, strlen(element->key), element);
    
    // 遍历哈希表中的所有元素
    my_hash_element *tmp;
    my_hash_element *ele;
    HASH_ITER(hh, map, ele, tmp) {
        printf("Key: %s, Value: %d\n", ele->key, *((int *)ele->value));
    }
    
    // 释放哈希表中的所有元素
    HASH_ITER(hh, map, element, tmp) {
        HASH_DEL(map, element);
        free(element->key);
        free(element);
    }
    
    return 0;
}
