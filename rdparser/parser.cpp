/*
 * Some clarifications:
 * - "definition" means a function is declared and
 *   its code defined.
 * - "declaration" means a variable is declared and
 *   possibly assigned a value
 *
 * */

#include <stdio.h>
#include <string.h>
#include "format.h"
#include "parser.h"
#include "bison-bridge.h"

int line=1;
char err_buff[256] = "";

extern YYSTYPE yylval;
extern char *yytext;

/** auxiliar **/

bool is_type(int token) {
    return 
        token == INT ||
        token == LONG ||
        token == UINT ||
        token == CHAR ||
        token == STRING ||
        token == FLOAT  ||
        token == DOUBLE;
}

// use when there is no room for ambiguity
int expect(int expected, bool type=false) {
    int next = yylex();
    if(type) { // ignore expected
        if(!is_type(next)) {
            sprintf(err_buff,"unexpected token %s (code %i); expected a type",
                    yytext,next);
            p_error(err_buff);
            return PARSE_ERR;
        }
        return next; // type
    }
    else if(next != expected) {
        sprintf(err_buff,"unexpected token %s (code %i); expected %i",
                yytext,next,expected);
        p_error(err_buff);
        return PARSE_ERR;
    }
    return next;
}

/** main **/
int main(int argc, char *argv[]) {
    return parse();
}

/** implementations **/

int parse() {
    /* TODO: code generation in this line */
    return program();
    /* and this one */
}

/*
 * Previous token: none
 * Lookahead: 1
 * Description: Program is a series of declarations
 * | definitions plus the main function. The first
 * terminals then ought to be always a match of
 * ({type} | "kansu")* "omo"
 * */
int program() {
    int next = yylex(), peek;
    while(next != MAIN) {
        /* verify declaration correct */
        if(is_type(next)) {
            if(declaration(next,&peek) == PARSE_ERR) //ALERT: declaration looks +1 ahead for arrows
                return PARSE_ERR;
        }
        else if(next == FUNC) {
            if(definition() == PARSE_ERR)
                return PARSE_ERR;
        }
        else {
            sprintf(err_buff,"unexpected token %s (code %i)",
                    yytext,next);
            p_error(err_buff);
            return PARSE_ERR;
        }
    }
    return mn();
}

/*
 * -Previous token: {type}
 * -Lookahead: 2
 * -Description: consists of a type keyword, an
 * identifier and optionally an asignment. So
 * {type} ID ("<-" nexp)?
 * -Stores next token in *next
 * */
int declaration(int prev, int *looked) {
    int next;
    expect(ID);
    // store in symbol table
    // table->store(yytext, blah, prev, whatever);
    next = expect(ARROW);
    if(next != ARROW) {
        *looked = next;
        return PARSE_OK;
    }
    return nexp(next);
}

/*
 * -Previous token: "kansu" (no need to use it)
 * -Lookahead: 1
 * -Description: 
 * "kansu" ID ':' {type} "<-" (argument)*
 * '{' code '}'
 * */
int definition() {
    int ret,next;
    expect(ID); //identifier
    char *name = strdup(yytext);
    expect(':'); //:
    ret = expect(0,true); //type
    // table->store(yytext,blah,next,blah);
    expect(ARROW); //<-
    // ambiguity
    next = yylex();
    if(next != '{') {
        do {
            if(argument(next) == PARSE_ERR)
                return PARSE_ERR;
            next = yylex();
        } while(next != '{');
    }
    //if(code() == PARSE_ERR) // code should parse until '}' and consume it
        //return PARSE_ERR;

    return code(ret);
}

/*
 * -Previous token: "omo" (no need to use it)
 * -Lookahead: 1
 * -Description: 
 * "omo" ':' {type} "<-" (argument)*
 * '{' code '}'
 * */
int mn() {
    int ret, next;
    expect(':'); //:
    ret = expect(0,true); //type
    expect(ARROW); //<-
    // ambiguity
    next = yylex();
    if(next != '{') {
        do {
            if(argument(next) == PARSE_ERR)
                return PARSE_ERR;
            next = yylex();
        } while(next != '{');
    }
    return code(ret);
}

/*
 * -Previous token: '{' (no need to use it)
 * -Receives: return type for exit/return checks
 * -Lookahead: 1
 * -Description: 
 * '{' | | | | '}'
 * */
int code(int ret) {
    return PARSE_ERR;
}

int assignment() {
    return PARSE_ERR;
}

int nexp(int prev) {
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

int argument(int prev) {
    return PARSE_ERR;
}

int call() {
    return PARSE_ERR;
}

int parameter() {
    return PARSE_ERR;
}

void verbose(const char* msg) {
    char buffer[256] = "";
    sprintf(buffer,"parser@%i: %s\n", line, msg);
    fprintf(stdout,buffer);
}

void p_error(const char* msg) {
    fprintf(stderr,"parser@%i: syntax error \"" RED " %s\"\n" CLEAR_FORMAT,line,msg);
}

