#include <stdio.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint32_t u32;

// 定义一个union类型，用于方便访问数组
typedef union {
    u8 val[4];
} Word;

int main() {
    // 定义一个Word类型的变量sh_size
    Word sh_size;

    // 给数组赋值
    sh_size.val[0] = 32;
    sh_size.val[1] = 1;
    // if (sh_size == 0) {
    //     int k = 1;
    // }
    u32 *sh = &sh_size;
    // 打印整体值
    printf("%u\n", sh_size);
    printf("%u\n", *sh);
    return 0;
}
