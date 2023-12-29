#ifndef NEW_VECTOR_H
#define NEW_VECTOR_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    // 定义动态数组结构体
    void** data;
    int size;
    int capacity;
} vector;

static inline void VectorNew(vector* arr, int num_elements) {
    arr->data = (void**)malloc(sizeof(void*) * num_elements);
    arr->size = 0;
    arr->capacity = num_elements;
}

static inline void VectorAdd(vector* arr, void* element, size_t element_size) {
    if (arr->size < arr->capacity) {
        // 分配内存空间并复制元素的值
        arr->data[arr->size] = malloc(element_size);
        // memcpy(arr->data[arr->size], element, element_size);
        arr->data[arr->size] = element;
        arr->size++;
    } else {
        // 动态扩容
        arr->capacity *= 2;
        arr->data = (void**)realloc(arr->data, sizeof(void*) * arr->capacity);

        // 分配内存空间并复制元素的值
        arr->data[arr->size] = malloc(element_size);
        arr->data[arr->size] = element;
        // void *element1 = element;
        // void *element2 = arr->data[arr->size];
        arr->size++;
    }
}

static inline void VectorFree(vector* arr) {
    // 释放每个元素的内存空间
    for (int i = 0; i < arr->size; i++) {
        free(arr->data[i]);
    }

    // 释放数组本身的内存空间
    free(arr->data);
}


#endif