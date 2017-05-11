/*
 * Some clarifications:
 * - "definition" means a function is declared and
 *   its code defined.
 * - "declaration" means a variable is declared and
 *   possibly assigned a value
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bison-bridge.h"
#include "format.h"
#include "parser.h"
#include "symtable.h"

int line=1;
char err_buff[256] = "";

int func_counter = 0;

extern YYSTYPE yylval;
extern char *yytext;

SymbolTable *table = new SymbolTable();

/** auxiliar **/

void verbose(const char* msg) {
    char buffer[256] = "";
    sprintf(buffer,"parser@%i: %s\n", line, msg);
    fprintf(stdout,buffer);
}

void p_error(const char* msg) {
    fprintf(stderr,"parser@%i: syntax error \"" RED " %s\"\n" CLEAR_FORMAT,line,msg);
}


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

bool is_num_oper(int token) {
    return 
        token == '+' ||
        token == '-' ||
        token == '*' ||
        token == '/' ||
        token == '%';
}

bool is_bool_oper(int token) {
    return 
        token == '=' ||
        token == '<' ||
        token == '>' ||
        token == '!';
}

// use when there is no room for ambiguity
int expect(int expected, bool type=false) {
    int next = yylex();
    if(type) { // ignore expected
        if(!is_type(next)) {
            sprintf(err_buff,"unexpected token %s (code %i); expected a type",
                    yytext,next);
            p_error(err_buff);
            if(yytext) free(yytext);
            return PARSE_ERR;
        }
        return next; // type
    }
    else if(next != expected) {
        sprintf(err_buff,"unexpected token %s (code %i); expected %i",
                yytext,next,expected);
        p_error(err_buff);
        if(yytext) free(yytext);
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
            if(declaration(next,&peek) == PARSE_ERR)
                return PARSE_ERR;
            next = peek; // in declaration we looked +1 ahead
        }
        /* verify definition correct */
        else if(next == FUNC) {
            if(definition() == PARSE_ERR)
                return PARSE_ERR;
            next = yylex();
        }
        /* not main and not the previous */
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
 * -Stores next token in *looked
 * */
int declaration(int prev, int *looked) {
    int next, type;
    if(expect(ID) == PARSE_ERR) return PARSE_ERR;

    table->store_symbol(prev, VAR_T, 0, yylval.sval);

    next = yylex();
    if(next != ARROW) {
        *looked = next;
        return PARSE_OK;
    }
    return nexp(next,&type);
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
    if(expect(ID) == PARSE_ERR) //identifier
        return PARSE_ERR;
    /* save the name */
    char *name = strdup(yylval.sval);

    if(expect(':') == PARSE_ERR) //:
        return PARSE_ERR;

    if((ret = expect(0,true)) == PARSE_ERR) //type
        return PARSE_ERR;

    if(expect(ARROW) == PARSE_ERR) //<-
        return PARSE_ERR;

    /* ambiguity! */
    next = yylex();
    while(next != '{') { /* still not the code */
        if(argument(next) == PARSE_ERR)
            return PARSE_ERR;
        next = yylex();
    }
    table->store_symbol(ret, FUNC_T, 0, name); //TODO: FIX
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
    if(expect(ARROW) == PARSE_ERR)
        return PARSE_ERR; //<-
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

/*
 * -Previous token: {ID | numb | call | operator}
 * -Receives: 
 * -Lookahead: 1
 * -Description:
 *  */
int nexp(int prev,int *ret_type) {
    switch(prev) {
        case INT_N:
        case FLO_N:
            return PARSE_OK; // not quite... or yes?
            /* explanation: returns OK so that the parser
             * will complain at the next error, if any    */
        case ID:
            SymbolRegister *got = table->get_symbol(yylval.sval);
            if(!got) { /* not even in the table */
                sprintf(err_buff,"%s was not declared",yylval.sval);
                p_error(err_buff);
                return PARSE_ERR;
            } else { /* in the table: let's check type */
                int type = got->get_type();
                /* -- take value of identifier -- */
                if(type == VAR_T || type == ARG_T) { /* var/arg? */
                    unsigned int level = got->get_level();
                    if(level != table->get_scope() && level != 0) { /* is it from this scope?
                                                                     * scope 0 means global  */
                        sprintf(err_buff,"%s was not declared in this scope",yylval.sval);
                        p_error(err_buff);
                        return PARSE_ERR;
                    } /* var/arg in the scope */
                    return PARSE_OK;
                }
                /* -- take return from call -- */
                else if(type == FUNC_T) { /* found a function */
                    /* check the call */
                    if(call() == PARSE_ERR)
                        return PARSE_ERR;
                }
            }
            return PARSE_OK; // same reasoning as with constants
        default:
            if(!is_num_oper(prev)) {

                sprintf(err_buff,"unexpected token %s",yytext);
                p_error(err_buff);
                return PARSE_ERR;
            }
    }
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

/*
 * -Previous token: ID
 * -Receives: return type for exit/return checks
 * -Lookahead: 1
 * -Description: 
 * '{' | | | | '}'
 * */
int argument(int prev) {
    /* lex has been called already! */
    /* save the name */
    int type;
    char *name = strdup(yylval.sval);

    if(expect(':') == PARSE_ERR) {
        free(name);
        return PARSE_ERR;
    }

    if((type = expect(0,true)) == PARSE_ERR) {
        free(name);
        return PARSE_ERR;
    }

    table->store_symbol(ARG_T,type,0,name); //TODO: FIX
    free(name);
    return PARSE_OK;
}

/*
 * -Previous token: ID (but no need to use it)
 * -Lookahead: 1
 * -Description: not only checks syntax, but consults
 *  the symbol table to check if the number of
 *  parameters passed is correct
 * ID '(' parameter* ')'
 * */
int call() {
    if(expect('(') == PARSE_ERR)
        return PARSE_ERR;
    // ambiguity! in the number of arguments
    // so count them
    int param_count = 0, of_type, next = yylex();
    while(next != ')') {
        if(nexp(next,&of_type) == PARSE_ERR)
            return PARSE_ERR;
        param_count++;
    }
    return PARSE_ERR;
}

int parameter() {
    return PARSE_ERR;
}

