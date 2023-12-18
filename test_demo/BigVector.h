#include "../elf.h"
#include "malloc.h"
#include "stdbool.h"
#include "integers.h"
typedef struct {
    u8* vec;
    u32 size;
} BitVector;

void BitVectorNew(BitVector* bv, u32 size) {
    bv->size = (size + 7) / 8;
    bv->vec = (u8*)malloc(bv->size * sizeof(u8));
    for (u32 i = 0; i < bv->size; i++) {
        bv->vec[i] = 0;
    }
}

void BitVectorResize(BitVector* bv, u32 size) {
    u32 newSize = (size + 7) / 8;
    bv->vec = (u8*)realloc(bv->vec, newSize * sizeof(u8));
    if (newSize > bv->size) {
        for (u32 i = bv->size; i < newSize; i++) {
            bv->vec[i] = 0;
        }
    }
    bv->size = newSize;
}

bool BitVectorGet(const BitVector* bv, u32 idx) {
    return bv->vec[idx / 8] & (1 << (idx % 8));
}

void BitVectorSet(BitVector* bv, u32 idx) {
    bv->vec[idx / 8] |= 1 << (idx % 8);
}

void BitVectorDispose(BitVector* bv) {
    free(bv->vec);
    bv->vec = NULL;
    bv->size = 0;
}