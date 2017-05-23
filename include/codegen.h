
#ifndef _Q_CODE_GEN_H
#define _Q_CODE_GEN_H

#include <stdio.h>
#include <stdint.h>

void init_q_file(char *filename);
void quit_codegen();
void codegen(char *qline);

// tags and gotos
int qgen_reserve_tag();
void qgen_write_reserved_tag(int label);
int qgen_tag(char *fname);
void qgen_jmp(char *fname);
void qgen_jmp(int label);

// save registers
void qgen_push_regs();
void qgen_pop_regs();

// reserve memory
unsigned int qgen_str(char *string);
unsigned int qgen_var(int type);
unsigned int qgen_str_var(int size);
unsigned int qgen_str_var(char *string);
unsigned int qgen_str_var(int size, char *string);
unsigned int qgen_str_var_scan(int size);

//stack

void qgen_raise_stack();
void qgen_lower_stack();

// fetch from the stack
void qgen_get_arg(int type, int offset);

// push params to the stack
void qgen_param_reg(unsigned int addr);
void qgen_push_par(int type, int offset, int reg);

// i/o
void qgen_scan(int type, unsigned int addr);

// arithmetics

// arithmetics: registers
int get_32_reg();
int get_64_reg();
void free_32_reg(int reg);
void free_64_reg(int reg);
void free_reg(int type, int reg);
int result_reg(int type);

void qgen_get_var(int type, int reg, unsigned int addr);
void qgen_get_int_val(int val, int reg);
void qgen_get_flo_val(double val, int reg);
void qgen_un_op(int plus_or_minus, int type, unsigned int addr);
void qgen_bi_op(int oper, int type, int reg1, int reg2);
void qgen_log_op(int oper, int reg1, int reg2);
void qgen_log_comp_op(int oper, int type, int reg1, int reg2);
void qgen_assign(int type, unsigned int addr);

// useful helpers
unsigned short type_length(int type);

#define qgen(...) {\
    char q_line[256];\
    sprintf(q_line,__VA_ARGS__);\
    strcat(q_line,"\n");\
    codegen(q_line);\
}

#endif
