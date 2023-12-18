#ifndef COMMON_H  // 头文件保护，防止重复包含
#define COMMON_H
#include "mold.h"
static inline void push_back(Context* ctx, Symbol* value) {
    ctx->arg.undefined_count++;
    Symbol** temp = (Symbol**)malloc(ctx->arg.undefined_count * sizeof(Symbol *));
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

#endif
