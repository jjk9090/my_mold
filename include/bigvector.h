#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned int u32;

typedef struct {
    u8* vec;
    u32 size;
} BitVector;

static inline void resize(BitVector* bv, u32 size) {
    u32 new_size = (size + 7) / 8;
    u8* new_vec = (u8*)realloc(bv->vec, new_size);
    if (new_vec != NULL) {
        bv->vec = new_vec;
        bv->size = size;
    }
}
