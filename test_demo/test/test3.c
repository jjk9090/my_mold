#include <stdio.h>
#include <string.h>

int main(){
    printf("%d\n",strcmp(".test",".plt"));
    printf("%d\n",strcmp(".test",".plt.got"));
    printf("%d\n",strcmp(".plt.got",".plt"));
}