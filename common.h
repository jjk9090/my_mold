#ifndef COMMON_H  // 头文件保护，防止重复包含
#define COMMON_H
#include "mold.h"
static inline void push_back(Context* ctx, ELFSymbol *value) {
    ctx->arg.undefined_count++;
    ELFSymbol **temp = (ELFSymbol **)malloc(ctx->arg.undefined_count * sizeof(ELFSymbol *));
    if (temp == NULL) {
        // 处理内存分配失败的情况
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    } else {
        for (int i = 0; i < ctx->arg.undefined_count - 1; i++) {
            temp[i] = ctx->arg.undefined[i];
        }
        temp[ctx->arg.undefined_count - 1] = value;

        // 释放旧的内存块
        // free(ctx->arg.undefined);

        ctx->arg.undefined = temp;
    }
}


static inline bool has_single_bit(unsigned long long val) {
    // 使用 Brian Kernighan's Algorithm 检查val中位为1的个数是否为1
    return val && !(val & (val - 1));
}

static inline u64 align_to(u64 val, u64 align) {
    if (align == 0)
        return val;
    assert(has_single_bit(align));
    return (val + align - 1) & ~(align - 1);
}
#endif
