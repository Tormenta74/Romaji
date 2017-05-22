
#ifndef _Q_CODE_GEN_H
#define _Q_CODE_GEN_H

#include <stdio.h>
#include <stdint.h>

void init_q_file(char *filename);
void quit_codegen();
void codegen(char *qline);

// tags and gotos
void qgen_tag(char *fname);
void qgen_jmp(char *fname);

// reserve memory
unsigned int qgen_str(char *string);
unsigned int qgen_var(int type);
unsigned int qgen_str_var(int size);
unsigned int qgen_str_var(char *string);
unsigned int qgen_str_var(int size, char *string);
unsigned int qgen_str_scan(int size);

// fetch from the stack
void qgen_get_arg(int type, int offset);

// save registers


// push params to the stack
void qgen_put_par(int type, int offset, int val);

// useful helpers
unsigned short type_length(int type);

#define qgen(...) {\
    char q_line[256];\
    sprintf(q_line,__VA_ARGS__);\
    strcat(q_line,"\n");\
    codegen(q_line);\
}

#endif
