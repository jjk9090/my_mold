#include <stdbool.h>        //为了在C语言文件中使用bool变量,C99规定,在本文件中并未用到bool类型
#ifndef __VECTOR_H_
#define __VECTOR_H_

typedef int (*VectorCompareFunction)(const void* elemAddr1,const void* elemAddr2);
typedef void (*VectorCopyFunction)(void* destAddr,const void* srcAddr);
typedef void (*VectorFreeFunction)(void* elemAddr);

typedef struct {
    void* data;                       //指向数据元素的指针
    void** ddata;
    int elemSize;                     //每个元素占内存的大小
    int allocatedSize;                //已分配内存元素的大小
    int logicalSize;                  //实际储存元素的个数
    int capacity;
    VectorFreeFunction freefn;      //设置了一个free函数，根据数据地址释放不定长部分的内存空间
    VectorCopyFunction copyfn;      //提供源数据和目的数据的地址，为目的数据中不定长部分申请内存空间，将源数据中不定长部分拷贝到目的数据中不定长部分
    VectorCompareFunction cmpfn;    //根据参数地址比较两数据的大小
} vector;
void VectorNew(vector* v,int elemSize,VectorFreeFunction freefn,VectorCopyFunction copyfn,VectorCompareFunction cmpfn);
void VectorNew_2(vector* v,int elemSize,VectorFreeFunction freefn,VectorCopyFunction copyfn,VectorCompareFunction cmpfn);
void VectorDispose(vector* v);
void VectorDispose_2(vector* v);
void* VectorNth(const vector* v,int position);
void VectorDelete(vector* v,int position);
void Vectorpush_back(vector* v,const void* elemAddr);
void Vectorpush_back_2(vector* v,void** elemAddr);
int VectorSize(const vector* v);
int VectorSearch(vector* v,const void* key,int startIndex);

#endif
