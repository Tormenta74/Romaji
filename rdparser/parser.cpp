/*
 * Some clarifications:
 * - "definition" means a function is declared and
 *   its code defined.
 * - "declaration" means a variable is declared and
 *   possibly assigned a value.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bison-bridge.h"
#include "format.h"
#include "parser.h"
#include "symtable.h"

int line=1;
char err_buff[256] = "";

// internal counter that actually defines
// a sort of scope: it increments each on
// definition (not in code blocks). what
// this means is that variables are actually
// available to the rest of the function
// after their declaration, no matter where
// that is. it also means that we can have
// name collision, so:
// TODO: fix symbol table to accomodate for
// this
int func_counter = 0;

extern YYSTYPE yylval;
extern char *yytext;

SymbolTable *table = new SymbolTable();

/*==============================================*/
/** ** **        HELPER FUNCTIONS        ** ** **/
/*==============================================*/

void verbose(const char* msg) {
    char buffer[256] = "";
    sprintf(buffer,"parser@%i: %s\n", line, msg);
    fprintf(stdout,buffer);
}

void p_error(const char* msg) {
    fprintf(stderr,"parser@%i: syntax error \"" RED "%s\"" CLEAR_FORMAT "\n" ,line,msg);
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

// discards token like a maniac until it finds '}'
// or dies in the attempt
int drop_until_brace() {
    int next;
    while((next = yylex()) != '}') {
        sprintf(err_buff,"** dropping \"%s\" token",yytext);
        p_error(err_buff);
    } if(next != '}') {
        sprintf(err_buff,"reached end of file: '{' is not closed");
        p_error(err_buff);
        return PARSE_ERR;
    }
    return PARSE_ERR;
}

// auxiliar, but almost part of the code:
// it performs divergence between boolean
// and numeric expressions
int expression(int previous) {
    //TODO
    return PARSE_ERR;
}

/*==============================================*/
/** ** **         MAIN FUNCTION          ** ** **/
/*==============================================*/

int main(int argc, char *argv[]) {
    //TODO
    return start(argv[1]);
}

/*==============================================*/
/** ** **        RULE FUNCTIONS          ** ** **/
/*==============================================*/

/*
 * Initializes object file
 * Receives: source code filename 
 */
int start(char* filename) {
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
/*
 * Previous token: 
 * Description:
 * Lookahead:
 * Eq. rule:
 *
 */
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
/*
 * Previous token: 
 * Description:
 * Lookahead:
 * Eq. rule:
 *
 */
int declaration(int prev, int *looked) {
    int next, type;
    if(expect(ID) == PARSE_ERR)
        return PARSE_ERR;

    //TODO: fix
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
/*
 * Previous token: 
 * Description:
 * Lookahead:
 * Eq. rule:
 *
 */
int definition() {
    int ret,next,num_args=0;
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
        num_args++;
        next = yylex();
    }
    // everything ok up to here, so we register the function
    func_counter++;
    table->store_symbol(ret, FUNC_T, num_args, name);

    return code(ret);
}

/*
 * -Previous token: "omo" (no need to use it)
 * -Lookahead: 1
 * -Description: 
 * "omo" ':' {type} "<-" (argument)*
 * '{' code '}'
 * */
/*
 * Previous token: 
 * Description:
 * Args: 
 * Lookahead:
 * Eq. rule:
 *
 */
int mn() {
    int ret, next;
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
    return code(ret);
}

/*
 * -Previous token: '{' (no need to use it)
 * -Receives: return type for exit/return checks
 * -Lookahead: 1
 * -Description: 
 * '{' uffff... '}'
 * */
/*
 * Previous token: 
 * Description:
 * Args: 
 * Lookahead:
 * Eq. rule:
 *
 */
int code(int ret) {
    //TODO
    int first = yylex();
    switch(first) {
        case '+':
            //TODO
            /* the incrementor and decrementor operators */
            // we have to repeat the logic, because
            // we can't invoke nexpr() ... or can we?
        case '-':
            //TODO
            //int type;
            //return nexp(first,&type);
            return PARSE_ERR;
        case ID:
            //TODO
            /* a call or an assignment */
            return PARSE_ERR;
        case IF:
            //TODO
            /* include here bexpr, the '{' code '}'  *
             * logic and the check for ELSE          */
            return PARSE_ERR;
        case WHILE:
            //TODO
            /* include here bexpr, the '{' code '}' */
            return PARSE_ERR;
        case PRINT:
            //TODO
            /* parse '(' parameter* ')' */
            return PARSE_ERR;
        case RET:
            /* several things here:
             * -check for parameter, and for type correctness *
             * -also, throw error if func is not VOID and no  *
             *  parameters are found                          *
             * -check for '}': ret is final to a code block   *
             *    -if not found, print error and iterate      *
             *    through tokens until one is found (doing    *
             *    nothing with them) and see what happens     */
            //TODO
            //if(no params) {
            if(ret != VOID) {
                sprintf(err_buff,"missing return value on non void function!");
                p_error(err_buff);
                return PARSE_ERR;
            }
            //}
            if(drop_until_brace() == PARSE_ERR)
                return PARSE_ERR;
            return PARSE_OK;
        case EXIT:
            /* drop_until_brace takes care of things and effectively
             * consumes the closing bracket (if found)*/
            if(drop_until_brace() == PARSE_ERR)
                return PARSE_ERR;
            return PARSE_OK;
        default: /* epsilon case is actually taken care of beforehand */
            sprintf(err_buff,"unexpected token %s (code %i)",
                    yytext,first);
            p_error(err_buff);
            return PARSE_ERR;

    }
    return PARSE_ERR;
}

/*
 * -Previous token: {ID | numb | call | operator}
 * -Receives: 
 * -Lookahead: 1
 * -Description:
 *  */
/*
 * Previous token: 
 * Description:
 * Args: 
 * Lookahead:
 * Eq. rule:
 *
 */
int nexp(int prev,int *ret_type) {
    //TODO: compute return type (well shitttt)
    switch(prev) {
        case INT_N:
        case FLO_N:
            return PARSE_OK; // not quite... or yes?
            /* explanation: returns OK so that the parser
             * will complain at the next error, if any    */
        case ID:
            {
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
                    return PARSE_OK; // same reasoning as with constants
                }
                return PARSE_ERR;
            }
        default:
            /* very ugly solution to use the default case, but  *
             * it simplifies a lot the checking of the token    */
            if(!is_num_oper(prev)) {
                sprintf(err_buff,"unexpected token %s; expected operator",yytext);
                p_error(err_buff);
                return PARSE_ERR;
            }
            /* back at it */
            switch(prev) {
                //int inmediate = -1;
                /* the 'lonely' operators -> the 'coupled' operands */
                case '*':
                case '/':
                case '%':
                    // parse two expressions
                    /* look ahead for twin */
                case '-':
                    // if found, look for identifier
                    // else parse two expressions
                case '+':
                default: return PARSE_ERR;
            }
            /* we got nothing? */
            return PARSE_OK;
    }
    return PARSE_ERR;
}

/*
 * -Previous token: {ID | numb | call | operator}
 * -Receives: 
 * -Lookahead: 1
 * -Description:
 *  */
/*
 * Previous token: 
 * Description:
 * Args: 
 * Lookahead:
 * Eq. rule:
 *
 */
int bexp(int prev) {
    switch(prev) {
        case -1:
            // previous is the control structure calling
            break; // or do I want to fallthrough?
            // probably need to place it somewhere else
        case TRUE:
        case FALSE:
            return PARSE_OK;
        case ID: /* it must be a call then: there are no */
            {        /* boolean variables in the language    */
                SymbolRegister *got = table->get_symbol(yylval.sval);
                if(!got) { /* not even in the table */
                    sprintf(err_buff,"%s was not declared",yylval.sval);
                    p_error(err_buff);
                    return PARSE_ERR;
                } else { /* in the table: let's check type */
                    int type = got->get_type();
                    /* -- invalid -- */
                    if(type == VAR_T || type == ARG_T)
                        return PARSE_ERR;
                    /* -- take return from call -- */
                    else if(type == FUNC_T) { /* found a function */
                        /* check the call */
                        if(call() == PARSE_ERR)
                            return PARSE_ERR;
                    }
                    return PARSE_OK;
                }
                return PARSE_ERR;
            }
            /* the easy ones (they only apply to boolean expressions)*/
        case '&':
        case '|':
            // parse 2 b expressions
            return PARSE_ERR;
        case '=':
            // NOTE: delegating aaaaall of this logic to expression()
            // holy fuck here we go
            // we need to peek to determine whether we deal with a nexpr or a bexpr
            // next triggers for b:
            //      - is_bool_oper
            //      - call returning bool
            //      - bool literal
            // next triggers for n:
            //      - is_num_oper
            //      - call returning num_type
            //      - num literal
            //      - identifier of var / arg
            return PARSE_ERR;
        case '>':
            // check for '='
            return PARSE_ERR;
        case '<':
            // check for '='
            return PARSE_ERR;
    }
    return PARSE_ERR;
}

/*
 * -Previous token: {type}
 * -Receives: the type of the argument
 * -Lookahead: 1
 * -Description: 
 * {type} ':' ID
 * */
/*
 * Previous token: 
 * Description:
 * Args: 
 * Lookahead:
 * Eq. rule:
 *
 */
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
 * Previous token: ID (already verified as function)
 * Description: checks for passed params and checks
 * each param's type against correspondant function
 * argument
 * Args:
 * Lookahead: 1
 * Eq. rule:
 *      [ID] '(' parameter* ')'
 * Special return: function return type
 */
int call() {
    //TODO: calls must return something, right?
    if(expect('(') == PARSE_ERR)
        return PARSE_ERR;
    // ambiguity! in the number of arguments so count them
    int param_count = 0, of_type, next = yylex();
    while(next != ')') {
        /* just let the parameter() function take 
         * care of it, then check param type */
        if(parameter(&of_type) == PARSE_ERR)
            return PARSE_ERR;
        param_count++;
    }
    return PARSE_OK;
}

/*
 * Previous token: '(' (no need to use it)
 * Description: very simple match, using the
 * same distinguish logic between b and n exp
 * Args:
 *  - *type : use to store the type of the param
 * Lookahead: 1
 * Eq. rule:
 *      STR | nexp | bexp
 */
int parameter(int *type) {
    int next = yylex();
    /* note: this block is actually significant to the 
     * language spec, since we are basically saying that
     * no operations can be performed on string literals,
     * which makes them pretty useless. but it keeps this
     * particular function extremely simple */
    if(next == STR) {
        *type = STR;
        return PARSE_OK;
    } else
        return expression(next);
}

