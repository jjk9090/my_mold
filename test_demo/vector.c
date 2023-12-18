/* loop--AC/DC
 * loop--AC/DC
 */
#include "vector.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

void VectorNew(vector* v,int elemSize,VectorFreeFunction freefn,VectorCopyFunction copyfn,VectorCompareFunction cmpfn) {
    v->elemSize = elemSize;
    v->allocatedSize = 4;
    v->logicalSize = 0;
    v->data = malloc(v->elemSize * v->allocatedSize);
    v->data = malloc(v->elemSize * v->allocatedSize);
    assert(v->data!=NULL);
    v->freefn = freefn;
    v->cmpfn = cmpfn;
    v->copyfn = copyfn;
}
void VectorNew_2(vector* v,int elemSize,VectorFreeFunction freefn,VectorCopyFunction copyfn,VectorCompareFunction cmpfn) {
    v->elemSize = elemSize;
    v->allocatedSize = 4;
    v->logicalSize = 0;
    v->ddata = malloc(v->elemSize * v->allocatedSize);
    assert(v->ddata!=NULL);
    v->freefn = freefn;
    v->cmpfn = cmpfn;
    v->copyfn = copyfn;
}

static void VectorGrow(vector* v) {
    v->data = realloc(v->data,2*v->allocatedSize*v->elemSize);
    assert(v->data!=NULL);
    v->allocatedSize = 2*v->allocatedSize;
}

static void VectorGrow_2(vector* v) {
    v->ddata = realloc(v->ddata,2*v->allocatedSize*v->elemSize);
    assert(v->ddata!=NULL);
    v->allocatedSize = 2*v->allocatedSize;
}

void Vectorpush_back(vector* v,const void* elemAddr) {
    if(v->logicalSize == v->allocatedSize)
        VectorGrow(v);
    void* destAddr = (char*)v->data + v->logicalSize * v->elemSize;
    /* memcpy(destAddr, elemAddr, v->elemSize); */
	if(v->copyfn != NULL)
		v->copyfn(destAddr, elemAddr);
    memcpy(destAddr,elemAddr,v->elemSize);
    v->logicalSize++;
}

void Vectorpush_back_2(vector* v,void** elemAddr) {
    if(v->logicalSize == v->allocatedSize)
        VectorGrow_2(v);
    void** destAddr = (void **)v->ddata + v->logicalSize * v->elemSize;
    /* memcpy(destAddr, elemAddr, v->elemSize); */
	if(v->copyfn != NULL)
		v->copyfn(destAddr, elemAddr);
    memcpy(destAddr,elemAddr,v->elemSize);
    v->logicalSize++;
}
void VectorDispose(vector* vec) {
    // 释放vec->data的内存空间
    //... (省略具体实现)
    free(vec->data);
}

void VectorDispose_2(vector* vec) {
    // 释放vec->data的内存空间
    //... (省略具体实现)
    free(vec->ddata);
}

// void VectorAdd(vector* arr, void *element,int elementSize) {
//     if (arr->allocatedSize < arr->capacity) {
//         arr->data[arr->allocatedSize++] = *element;
//     } else {
//         // 动态扩容
//         arr->capacity *= 2;
//         arr->data = (int*)realloc(arr->data, elementSize * arr->capacity);
//         arr->data[arr->allocatedSize++] = element;
//     }
// }

void VectorAdd_2(vector* arr, void *element,int elementSize) {
    if (arr->allocatedSize < arr->capacity) {
        arr->ddata[arr->allocatedSize++] = element;
    } else {
        // 动态扩容
        arr->capacity *= 2;
        arr->ddata = (int*)realloc(arr->ddata, elementSize * arr->capacity);
        arr->ddata[arr->allocatedSize++] = element;
    }
}