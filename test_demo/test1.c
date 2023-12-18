#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint8_t val[2];
} ExampleStruct;

int main() {
    ExampleStruct ehdr;
    ehdr.val[0] = 0xB0;
    ehdr.val[1] = 0x01;

    uint16_t value = (ehdr.val[1] << 8) | ehdr.val[0];

    // 将 value 格式化为十六进制字节
    printf("Value in hexadecimal: 0x%04X\n", value);

    // 分别输出每个字节的十六进制表示
    printf("Byte 1 in hexadecimal: 0x%02X\n", ehdr.val[0]);
    printf("Byte 2 in hexadecimal: 0x%02X\n", ehdr.val[1]);

    return 0;
}
