#include <stdlib.h>

// 定义结构体，表示 ObjectFile 类
typedef struct ObjectFile {
    // 成员变量
    int a;
    char b[10];
} ObjectFile;

// 定义结构体，表示智能指针
typedef struct SmartPointer {
    // 成员变量
    ObjectFile *ptr;
    int ref_count;
} SmartPointer;

// 定义函数，用于创建一个 ObjectFile 对象，并返回指针
ObjectFile *create_object_file() {
    ObjectFile *obj = (ObjectFile *)malloc(sizeof(ObjectFile));
    obj->a = 1;
    obj->b[0] = 'a';
    obj->b[1] = '\0';
    return obj;
}

// 定义函数，用于创建一个智能指针，并初始化引用计数为 1
SmartPointer *create_smart_pointer(ObjectFile *obj) {
    SmartPointer *sp = (SmartPointer *)malloc(sizeof(SmartPointer));
    sp->ptr = obj;
    sp->ref_count = 1;
    return sp;
}

// 定义函数，用于增加智能指针的引用计数
void increase_ref_count(SmartPointer *sp) {
    sp->ref_count++;
}

// 定义函数，用于释放智能指针所指向的对象
void release_object(SmartPointer *sp) {
    if (--sp->ref_count == 0) {
        free(sp->ptr);
        free(sp);
    }
}

// 定义函数，类似于 C++ 中的 emplace_back() 函数，用于向对象池中添加一个对象
void add_object_to_pool(ObjectFile **pool, int *size, ObjectFile *obj) {
    // 创建一个智能指针，将引用计数初始化为 1
    SmartPointer *sp = create_smart_pointer(obj);

    // 将新的智能指针加入到数组中
    pool[*size] = obj;
    (*size)++;

    // 增加智能指针的引用计数
    increase_ref_count(sp);
}

// 测试函数
int main() {
    ObjectFile **pool = (ObjectFile **)malloc(sizeof(ObjectFile *) * 10);
    int size = 0;

    ObjectFile *obj1 = create_object_file();
    add_object_to_pool(pool, &size, obj1);

    ObjectFile *obj2 = create_object_file();
    add_object_to_pool(pool, &size, obj2);

    // 释放对象池中所有对象
    for (int i = 0; i < size; i++) {
        release_object(pool[i]);
    }

    free(pool);

    return 0;
}
