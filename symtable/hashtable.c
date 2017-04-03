

#ifndef __LENYE__
#define __LENYE__

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLESIZE 1024

#define VAR
#define FUNC
#define PARAM

unsigned short level = 0;

typedef struct _reg {
    int dval;
    char* strval;
} reg;

reg* table[TABLESIZE];

int hash(char* symbol) {
    return strlen(symbol);
}

int store_symbol(char* symbol) {
    /* stores the string value and the first numeric character */
    int hv = hash(symbol);
    char *c;
    int alpha = -1;
    for(c = symbol; c != '\0'; c++) {
        if(isdigit(*c)) {
            alpha = *c;
            break;
        } 
    }
    reg *r = (struct reg*)malloc(sizeof(int)+sizeof(char*));
    r->dval = alpha;
    r->strval = symbol;

    table[hv] = r;
    return 0;
}



int main(int argc, char* argv[]) {
    char buffer[64];
    printf("Enter a symbol (a string with an alphanumeric character): ");
    gets(buffer);
    if(!store_symbol(buffer))
        printf("Something went wrongggg\n");
    return 0;
}

#endif
