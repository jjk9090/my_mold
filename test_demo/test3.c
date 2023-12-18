#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"

// 定义一个自定义结构体作为哈希表中的元素
typedef struct {
    int64_t key;  // 键是 i64 类型
    void *value;  // 值是 void* 类型
    UT_hash_handle hh;
} my_hash_element;

my_hash_element *hash_table = NULL;

// 向哈希表中插入一个元素
void insert_element(int64_t key, void *value) {
    my_hash_element *element = (my_hash_element *)malloc(sizeof(my_hash_element));
    element->key = key;
    element->value = value;
    HASH_ADD_INT(hash_table, key, element); // 使用 HASH_ADD_INT64 宏添加元素
}

// 根据键值查找哈希表中的元素
my_hash_element *find_element(int64_t key) {
    my_hash_element *element;
    HASH_FIND_INT(hash_table, &key, element); // 使用 HASH_FIND_INT64 宏查找元素
    return element;
}

// 从哈希表中删除指定的元素
void delete_element(my_hash_element *element) {
    HASH_DEL(hash_table, element); // 从哈希表中删除指定的元素
    free(element); // 释放内存
}

int main(int argc, char *argv[]) {
    // 插入一些元素
    int64_t keys[] = {1, 2, 3};
    int values[] = {100, 200, 300};
    for (int i = 0; i < 3; i++) {
        insert_element(keys[i], &values[i]);
    }

    // 查找并输出元素
    my_hash_element *element;
    int64_t keys_to_find[] = {1, 3, 5};
    for (int i = 0; i < 3; i++) {
        element = find_element(keys_to_find[i]);
        if (element != NULL) {
            printf("Key: %ld" ", Value: %d\n", element->key, *(int *)element->value);
        } else {
            printf("Key %ld"  " not found\n", keys_to_find[i]);
        }
    }

    // 删除哈希表中的一个元素
    element = find_element(2);
    if (element != NULL) {
        delete_element(element);
    }

    // 释放哈希表
    while (hash_table != NULL) {
        element = hash_table;
        HASH_DEL(hash_table, element);
        free(element);
    }

    return 0;
}
