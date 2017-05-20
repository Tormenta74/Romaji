
#ifndef _Q_CODE_GEN_H
#define _Q_CODE_GEN_H

#include <stdio.h>

void init_q_file(char *filename);
void quit_codegen();
void codegen(char *qline);
void qgen_tag(char *fname);
void qgen_jmp(char *fname);
void qgen_str(char *string);

#define qgen(...) {\
    char q_line[256];\
    sprintf(q_line,__VA_ARGS__);\
    strcat(q_line,"\n");\
    codegen(q_line);\
}

#endif
