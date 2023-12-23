#include <stdio.h>

int main() {
    int ttext = 0x72600;
    int data = 0x72a90;
    int bss = 0x72f20;
    int comment = 0x70410;

    int ttext_dec = ttext;
    int data_dec = data;
    int bss_dec = bss;
    int comment_dec = comment;

    int ttext_data_diff = data_dec - ttext_dec;
    int data_bss_diff = bss_dec - data_dec;
    int bss_comment_diff = comment_dec - bss_dec;

    printf(".ttext to .data difference: %d\n", ttext_data_diff);
    printf(".data to .bss difference: %d\n", data_bss_diff);
    printf(".bss to .comment difference: %d\n", bss_comment_diff);

    return 0;
}
