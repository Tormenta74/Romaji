
#ifndef _DR_PARSER_H
#define _DR_PARSER_H

#define PARSE_OK 0
#define PARSE_ERR 1

int parse();

int program();
int declaration(int,int*);
int definition();
int mn();
int assignment();
int nexp(int,int*);
int bexp();
int signature();
int main_sig();
//int arguments(); // replaced by for loop
int argument(int);
int code(int);
int call();
//int parameters() // replaced by for loop;
int parameter();

void verbose(const char*);
void p_error(const char*);

#endif
