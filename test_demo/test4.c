#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"

// 定义一个自定义结构体作为哈希表中的元素
typedef struct {
    char *key;  // 键是 i64 类型
    void *value;  // 值是 void* 类型
    uint32_t hash; // 存储哈希值的成员
    UT_hash_handle hh;
} my_hash_element;

my_hash_element *hash_table = NULL;

// 向哈希表中插入一个元素
void insert_element(char *key, void *value,int  hash) {
    my_hash_element *element = (my_hash_element *)malloc(sizeof(my_hash_element));
    element->key = key;
    element->value = value;
    // HASH_MAKE_KEY(hash, element->key); // 生成哈希值
    element->hash = hash; // 存储哈希值
    HASH_ADD(hh, hash_table, key, sizeof(int64_t), element); // 添加元素
}

// 根据键值查找哈希表中的元素
my_hash_element *find_element(int64_t key) {
    my_hash_element *element;
    HASH_FIND(hh, hash_table, &key, sizeof(int64_t), element); // 查找元素
    return element;
}

// 从哈希表中删除指定的元素
void delete_element(my_hash_element *element) {
    HASH_DELETE(hh, hash_table, element); // 删除元素
    free(element); // 释放内存
}

my_hash_element *find_element(int64_t key, uint32_t hash) {
    my_hash_element *element;
    HASH_FIND(hh, hash_table, &key, sizeof(int64_t), element); // 首先根据键值查找元素
    while (element != NULL && element->hash != hash) {  // 如果找到的元素哈希值不匹配，则继续查找下一个
        element = (my_hash_element*)(element->hh.next);
    }
    return element;
}

// 根据键值和哈希值删除哈希表中的元素
// void delete_element_by_key_and_hash(int64_t key, uint32_t hash) {
//     my_hash_element *element = find_element(key, hash);  // 先根据键值和哈希值查找元素
//     if (element != NULL) {
//         HASH_DELETE(hh, hash_table, element); // 删除元素
//         free(element); // 释放内存
//     }
// }
int main(int argc, char *argv[]) {
    // ... （其他部分保持不变）

    return 0;
}
