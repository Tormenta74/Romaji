
#ifndef _DR_PARSER_H
#define _DR_PARSER_H

#define PARSE_OK 0
#define PARSE_ERR 1
#define PARSE_WARN 2

int start();

int program();
int declaration(int,int*);
int definition();
int mn();
//int assignment(); // taken care of when needed
int nexp(int,int*);
int bexp(int);
//int signature(); // totally unnecesary
//int main_sig(); // totally unnecesary
//int arguments(); // replaced by for loop
int argument(int);
int code(int);
int call();
//int parameters() // replaced by for loop;
int parameter(int,int*);


#endif
