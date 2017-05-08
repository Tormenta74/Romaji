
#include "rjiparser.h"

#ifndef _DR_PARSER_H
#define _DR_PARSER_H

#define PARSE_OK 0
#define PARSE_ERR 1

int parse();

int program();
int declaration();
int definition();
int mn();
int assignment();
int nexp();
int bexp();
int signature();
int main_sig();
int arguments();
int argument();
int code();
int call();
int parameters();
int parameter();

void verbose(const char*);
void p_error(const char*);

#endif
