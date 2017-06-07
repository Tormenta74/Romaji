
#ifndef _Q_CODE_GEN_H
#define _Q_CODE_GEN_H

#include <stdio.h>
#include <stdint.h>

void init_q_file(char *filename);
void quit_codegen();
void codegen(char *qline);

/** ------------------------------------- **/
/**            TAGS AND GOTOS             **/
/** ------------------------------------- **/
int qgen_reserve_tag();
void qgen_write_reserved_tag(int label);
int qgen_tag(char *fname);
void qgen_jmp(char *fname);
void qgen_jmp(int label);

/** ------------------------------------- **/
/**            MEMORY RESERVE             **/
/** ------------------------------------- **/
void reset_local_vars();
unsigned int qgen_str_lit(char *string);
unsigned int qgen_declare_var(int type, int scope);

unsigned int qgen_declare_str_var(int size, int scope);
unsigned int qgen_declare_str_var(char *string, int scope, int size);
unsigned int qgen_declare_str_var_scan(int size, int scope);

/** ------------------------------------- **/
/**             STACK UTILS               **/
/** ------------------------------------- **/
void qgen_release_stack(int n);
void qgen_take_stack(int n);

int *qgen_push_32_regs();
int *qgen_push_64_regs();

void qgen_pop_32_regs(int *array);
void qgen_pop_64_regs(int *array);

// push params to the stack
void qgen_push_param(int type, int arg_or_var, unsigned int addr, int scope);

void qgen_push_result(int type);
void qgen_pop_result(int type, int *unavailable);

/** ------------------------------------- **/
/**                 I/O                   **/
/** ------------------------------------- **/
void qgen_scan(int type, unsigned int addr);
unsigned int qgen_str_stat(char *string);

/** ------------------------------------- **/
/**             ARITHMETICS               **/
/** ------------------------------------- **/
void qgen_get_str(unsigned int addr);
// arithmetics: registers
int get_32_reg();
int get_64_reg();
void free_32_reg(int reg);
void free_64_reg(int reg);
void free_reg(int type, int reg);
int result_reg(int type);

void qgen_get_vararg(int type, int arg_or_var, unsigned int addr, int scope);

void qgen_get_int_val(int val, int reg);
void qgen_get_flo_val(double val, int reg);

void qgen_un_op(int plus_or_minus, int type, int arg_or_var, unsigned int addr, int scope);
void qgen_bi_op(int oper, int type, int reg1, int reg2);
void qgen_log_comp_op(int oper, int type, int reg1, int reg2);

void qgen_assign(int type, int arg_or_var, unsigned int addr, int scope);

// useful helpers
unsigned short type_length(int type);

// shortcut
#define qgen(...) {\
    char q_line[256];\
    sprintf(q_line,__VA_ARGS__);\
    strcat(q_line,"\n");\
    codegen(q_line);\
}

#endif
