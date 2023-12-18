#include "vector.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "symbol.h"

int main() {
    vector v2;
    
    Symbol s1;
    strcpy(s1.name,"s1");
    Symbol s2;
    strcpy(s2.name,"s2");
    VectorNew(&v2,sizeof(Symbol),NULL,NULL,NULL);
    
    Vectorpush_back(&v2,&s1);
    Vectorpush_back(&v2,&s2);
    printf("%s\n", ((Symbol *)v2.data)->name);
    Symbol* secondSym = (Symbol*)v2.data + 1;

    printf("%s\n",secondSym->name);
    vector v3;
    Symbol *sym1 = (Symbol *)malloc(sizeof(Symbol));
    Symbol *sym2 = (Symbol *)malloc(sizeof(Symbol));
    strcpy(sym1->name, "sym1");
    strcpy(sym2->name, "sym2");
    VectorNew_2(&v3, sizeof(Symbol *), NULL, NULL, NULL);
    Vectorpush_back_2(&v3, &sym1);
    Vectorpush_back_2(&v3, &sym2);
    Vectorpush_back_2(&v3, &sym2);
    Vectorpush_back_2(&v3, &sym2);
    Vectorpush_back_2(&v3, &sym2);
    Vectorpush_back_2(&v3, &sym2);
    Vectorpush_back_2(&v3, &sym2);
    Vectorpush_back_2(&v3, &sym2);
    printf("%s\n", (*(Symbol **)(v3.ddata))->name);
    
    VectorDispose(&v2);
    VectorDispose_2(&v3);
    
    free(sym1);
    free(sym2);
}