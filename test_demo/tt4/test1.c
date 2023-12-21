#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义一个结构体表示Chunk
struct Chunk {
    char name[50];
    int sh_type;
    int sh_flags;
};

// 定义一个比较函数用于在qsort中进行自定义排序
int compareChunks(const void *a, const void *b) {
    struct Chunk *chunkA = (struct Chunk *)a;
    struct Chunk *chunkB = (struct Chunk *)b;

    // 按照逐个成员变量进行比较
    int nameComparison = strcmp(chunkA->name, chunkB->name);
    if (nameComparison != 0) {
        return nameComparison;
    }

    if (chunkA->sh_type != chunkB->sh_type) {
        return chunkA->sh_type - chunkB->sh_type;
    }

    return chunkA->sh_flags - chunkB->sh_flags;
}

int main() {
    struct Chunk chunks[] = {
        {"def", 1, 3},
        {"abc", 1, 2},
        {"ghi", 2, 1}
    };
    size_t numChunks = sizeof(chunks) / sizeof(chunks[0]);

    // 使用qsort对chunks数组进行排序
    qsort(chunks, numChunks, sizeof(struct Chunk), compareChunks);

    // 输出排序后的结果
    for (size_t i = 0; i < numChunks; i++) {
        printf("Name: %s, sh_type: %d, sh_flags: %d\n", chunks[i].name, chunks[i].sh_type, chunks[i].sh_flags);
    }

    return 0;
}
