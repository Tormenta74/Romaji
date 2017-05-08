
#include <stdio.h>
#include "format.h"
#include "parser.h"

int line=1;
extern YYSTYPE yylval;

/*int yylex() {
    return FUNC;
}*/

/** auxiliar **/

bool is_decl_type(int token) {
    return 
        token == INT ||
        token == LONG ||
        token == UINT ||
        token == CHAR ||
        token == STRING ||
        token == FLOAT  ||
        token == DOUBLE;
}

// saves the value of the previous
int peek(YYSTYPE *save) {
    return false;
}

/** implementations **/

int parse() {
    /* TODO: code generation in this line */
    return program();
    /* and this one */
}

int program() {
    int next;
    while(is_decl_type(next = yylex()) || next == FUNC)
        if(!declaration() && !definition())
            return PARSE_ERR;
    return mn();
}

int declaration() {
    //int next = yylex();
    return PARSE_ERR;
}

int definition() {
    return PARSE_ERR;
}

int mn() {
    return PARSE_ERR;
}

int assignment() {
    return PARSE_ERR;
}

int nexp() {
    return PARSE_ERR;
}

int bexp() {
    return PARSE_ERR;
}

int signature() {
    return PARSE_ERR;
}

int main_sig() {
    return PARSE_ERR;
}

int argument() {
    return PARSE_ERR;
}

int code() {
    return PARSE_ERR;
}

int call() {
    return PARSE_ERR;
}

int parameter() {
    return PARSE_ERR;
}

void verbose(char* msg) {
    char buffer[256] = "";
    sprintf(buffer,"parser@%i: %s\n", line, msg);
    fprintf(stdout,buffer);
}

void p_error(char* msg) {
    fprintf(stderr,"parser@%i:" RED " %s\n" CLEAR_FORMAT,line,msg);
}

