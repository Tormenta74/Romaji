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
extern FILE *yyin;

SymbolTable *table = new SymbolTable();

/*==============================================*/
/** ** **        HELPER FUNCTIONS        ** ** **/
/*==============================================*/

// useful for debugging
void verbose(const char* msg) {
    char buffer[256] = "";
    sprintf(buffer,"parser@%i: " BLUE "%s" CLEAR_FORMAT "\n", line, msg);
    fprintf(stdout,buffer);
    fflush(stdout);
}

// formats error messages
void p_error(const char* msg) {
    fprintf(stderr,"parser@%i: syntax error " RED "%s" CLEAR_FORMAT "\n" ,line,msg);
}

// oneliner for checking if token is type keyword
bool is_type(int token) {
    return 
        token == INT ||
        token == LONG ||
        token == UINT ||
        token == CHAR ||
        token == STRING ||
        token == FLOAT  ||
        token == DOUBLE  ||
        token == BOOL  ||
        token == VOID;
}

// oneliner for checking if token is a numeric operator
bool is_num_oper(int token) {
    return 
        token == '+' ||
        token == '-' ||
        token == '*' ||
        token == '/' ||
        token == '%';
}

// oneliner for checking if token is a logic operator
bool is_logic_oper(int token) {
    return 
        token == '=' ||
        token == '<' ||
        token == '>' ||
        token == '&' ||
        token == '|' ||
        token == '!';
}

// use when there is no room for ambiguity:
// prints error and returns error code
int expect(int expected, bool type=false) {
    int next = yylex();
    if(type) { // ignore expected
        if(!is_type(next)) {
            sprintf(err_buff,"(%i)unexpected token %s (code %i); expected a type",
                    __LINE__,yytext,next);
            p_error(err_buff);
            if(yytext) free(yytext);
            return PARSE_ERR;
        }
        return next; // type
    }
    else if(next != expected) {
        sprintf(err_buff,"(%i)unexpected token %s (code %i); expected %i",
                __LINE__,yytext,next,expected);
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

// auxiliar, but almost part of the code: it
// settles between boolean and numeric expressions
int expression(int prev) {
    //TODO
    return PARSE_ERR;
}

/*==============================================*/
/** ** **         MAIN FUNCTION          ** ** **/
/*==============================================*/

int main(int argc, char *argv[]) {
    if(argc>1)
        if((yyin = fopen(argv[1],"r")) == NULL)
            return -1;
    verbose("diving into the parse tree");
    int ret = start();

    fflush(stdout);
    printf("return: %i\nState of the symbol table:\n",ret);
    table->print();
    return start();
}

/*==============================================*/
/** ** **        RULE FUNCTIONS          ** ** **/
/*==============================================*/

/*
 * Initializes object file
 * Receives: source code filename 
 */
int start() {
    /* TODO: code generation in this line */
    return program();
    /* and this one */
}


/*
 * Previous token: none
 * Description: Program is a series of declarations
 * | definitions plus the main function.
 * Lookahead: 1
 * Eq. rule:
 *      (declaration | definition)* main
 */
int program() {
    int next = yylex(), peek;
    while(next != MAIN) {
        /* verify declaration correct */
        verbose("still not main");
        if(is_type(next)) {
            verbose("perceived type keyword");
            if(declaration(next,&peek) == PARSE_ERR)
                return PARSE_ERR;
            next = peek; // in declaration we looked +1 ahead
            verbose("declarations++");
        }
        /* verify definition correct */
        else if(next == FUNC) {
            verbose("perceived function keyword");
            if(definition() == PARSE_ERR)
                return PARSE_ERR;
            next = yylex();
            verbose("definitions++");
        }
        /* not main and not the previous */
        else {
            sprintf(err_buff,"(%i)unexpected token %s (code %i)",
                    __LINE__,yytext,next);
            p_error(err_buff);
            return PARSE_ERR;
        }
    }
    return mn();
}

/*
 * -Previous token: {type}
 * Description: consists of a type keyword, an
 * identifier and optionally an asignment. in
 * order to figure out if an assignment is next,
 * it consumes one extra token, which is stored in
 * *looked.
 * Lookahead: 2
 * Eq. rule:
 *      {type} ID ("<-" nexp)?
 */
int declaration(int prev, int *looked) {
    int next, type;
    if(expect(ID) == PARSE_ERR)
        return PARSE_ERR;

    //TODO: fix
    table->store_symbol(VAR_T, prev, 0, yylval.sval);

    next = yylex();
    if(next != ARROW) {
        *looked = next;
        return PARSE_OK;
    }
    return nexp(next,&type);
}

/*
 * Previous token: "kansu" (no need to use it)
 * Lookahead: 1
 * Description: process tokens until '}', meaning
 * the function body is complete. very clean
 * Eq. rule:
 *      "kansu" ID ':' {type} "<-" (argument)*
 *      '{' code '}'
 */
int definition() {
    int ret,next,num_args=0;

    if(expect(ID) == PARSE_ERR) //identifier
        return PARSE_ERR;
    verbose("processed: identifier");
    /* save the name */
    char *name = strdup(yylval.sval);
    verbose(name);

    if(expect(':') == PARSE_ERR) //:
        return PARSE_ERR;
    verbose("processed: ':'");

    if((ret = expect(0,true)) == PARSE_ERR) //type
        return PARSE_ERR;
    verbose("processed: return type");

    if(expect(ARROW) == PARSE_ERR) //<-
        return PARSE_ERR;

    /* ambiguity! */
    next = yylex();
    while(next != '{') { /* still not the code */
        verbose("processed: argument");
        if(argument(next) == PARSE_ERR)
            return PARSE_ERR;
        num_args++;
        next = yylex();
    }
    // everything ok up to here, so we register the function
    table->set_scope(++func_counter);
    //TODO: set the correct scope (or number) of the function
    table->store_symbol(FUNC_T, ret, num_args, name);

    return code(ret);
}

/*
 * Previous token: "omo" (no need to use it)
 * Description: very similar to the function parsing
 * Args: none
 * Lookahead: 1
 * Eq. rule:
 *      "omo" ':' {type} "<-" (argument)*
 *      '{' code '}'
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
 * Previous token: '{' (no need to use it)
 * Description: checks for code integrity and
 * for return type.
 * Args:
 *   - ret: the expected return type
 * Lookahead: 2
 * Eq. rule:
 *      '{' {better see the grammar for this rule} '}'
 */
int code(int ret) {
    int first = yylex();
    int plus_or_minus = first; // if we save the + / - we can avoid repeating their code twice
    SymbolRegister *found = table->get_symbol(yylval.sval); // let's get this out of the way
    // TODO: lacking a while, are we?
    // TODO: also, lacking variable declarations!
    // TODO: also, lacking closing '}'
    // or maybe recursion, but that could be bad
    while(first != '}') {
    sprintf(err_buff,"next statement: starts with %s",yytext);
    verbose(err_buff);
        switch(first) {
            /* the incrementor and decrementor operators:  *
             * we have to check for the "twin", and if not *
             * found the expression makes no sense         */
            case '+':
            case '-': 
                first = yylex();
                if(first == plus_or_minus) {
                    first = yylex();
                    if(first != ID) {
                        sprintf(err_buff,"(%i)the increment operator only works on variables",
                                __LINE__);
                        p_error(err_buff);
                        return PARSE_ERR;
                    } else if(!(found = table->get_symbol(yylval.sval))) {
                        sprintf(err_buff,"(%i)%s was not previously declared",
                                __LINE__,yylval.sval);
                        p_error(err_buff);
                        return PARSE_ERR;
                    } else if(found->get_type() != INT) {
                        sprintf(err_buff,"(%i)the decrement/increment operator only works on variables of integer type",
                                __LINE__);
                        p_error(err_buff);
                        return PARSE_ERR;
                    } else {
                        // do your increment magic here
                    }
                } else {
                    sprintf(err_buff,"(%i)the only allowed statement that starts with '%c' is a decrement of a variable",
                            __LINE__,plus_or_minus);
                    p_error(err_buff);
                    return PARSE_ERR;
                }
            case ID:
                /* a call or an assignment */
                int type;
                if(!found) {
                    sprintf(err_buff,"(%i)%s was not previously declared",
                            __LINE__,yylval.sval);
                    p_error(err_buff);
                    return PARSE_ERR;
                } else if ((type = found->get_type())== FUNC_T) {
                    //in this instance we don't need to check the return type... nobody is expecting it!
                    if(call() == PARSE_ERR)
                        return PARSE_ERR;
                    break; // will this break out of the switch, or the while?
                } else if (type == VAR_T || type == ARG_T) { 
                    // look for asignment
                    if(expect(ARROW) == PARSE_ERR)
                        return PARSE_ERR;
                    //TODO: missing the SCAN possibility... ought to peek one more
                    int expected_ret = found->get_return(),
                        parsed_ret;
                    if(nexp(-1,&parsed_ret) == PARSE_ERR)
                        return PARSE_ERR;
                    if(expected_ret != parsed_ret) {
                        sprintf(err_buff,"(%i)trying to assign a %i value to %s, which is of type %i",
                                __LINE__,parsed_ret,found->get_name(),expected_ret);
                        p_error(err_buff);
                        return PARSE_ERR;
                    }
                    break;
                }
                return PARSE_ERR;
            case IF:
                if(bexp(-1) == PARSE_ERR) // check for the condition
                    return PARSE_ERR;
                if(expect('{') == PARSE_ERR) {
                    p_error("sorry, but conditional statements must be surrounded by curly brackets");
                }
                if(code(-1) == PARSE_ERR) // consumes the closing '}'
                    return PARSE_ERR;
                first = yylex(); // look ahead for ELSE
                if(first == ELSE) {
                    if(expect('{') == PARSE_ERR) {
                        p_error("sorry, but conditional statements must be surrounded by curly brackets");
                    }
                    if(code(-1) == PARSE_ERR) // consumes the closing '}'
                        return PARSE_ERR;
                }
                break; // next token already fetched
            case WHILE:
                /* include here bexpr, the '{' code '}' */
                if(bexp(-1) == PARSE_ERR) // check for the condition
                    return PARSE_ERR;
                if(expect('{') == PARSE_ERR) {
                    p_error("sorry, but conditional statements must be surrounded by curly brackets");
                }
                if(code(-1) == PARSE_ERR) // consumes the closing '}'
                    return PARSE_ERR;
                first = yylex(); // done with this statement
                break; // next token already fetched
            case PRINT:
                /* parse '(' parameter* ')' */
                /* or... could I just use call()? as long as it processes the arguments correctly...
                 * if it becomes hellish to put the arguments into the object, just copy the code */
                if(call() == PARSE_ERR)
                    return PARSE_ERR;
                first = yylex(); // done with this statement
            case RET:
                /* several things here:
                 * -check for parameter, and for type correctness *
                 * -also, throw error if func is not VOID and no  *
                 *  parameters are found                          *
                 * -check for '}': ret is final to a code block   *
                 *    -if not found, print error and iterate      *
                 *    through tokens until one is found (doing    *
                 *    nothing with them) and see what happens     */
                if(ret == VOID) {
                    if(expect('}') == PARSE_ERR) {
                        sprintf(err_buff,"(%i)returning something in a void function!",__LINE__);
                        p_error(err_buff);
                        return PARSE_ERR;
                    }
                    return PARSE_OK;
                } else {
                    first = yylex();
                    if(first == ID) { // identifier or call?
                        found = table->get_symbol(yylval.sval);
                        if(!found) {
                            sprintf(err_buff,"(%i)%s was not previously declared",
                                    __LINE__,yylval.sval);
                            p_error(err_buff);
                            return PARSE_ERR;
                        } else if(found->get_return() != ret) { // return type coherence check
                            sprintf(err_buff,"(%i)expected %i return type: %s is of %i type",
                                    __LINE__,ret,found->get_name(),found->get_return());
                            p_error(err_buff);
                            return PARSE_ERR;
                        } else if(found->get_type()== FUNC_T) {
                            if(call() == PARSE_ERR)
                                return PARSE_ERR;
                            break;
                        } else { // ARG_T / VAR_T, but we don't quite care
                            break;
                        }
                    } else if(expression(first) == PARSE_ERR) // also, gotta check return type
                        return PARSE_ERR;
                    return PARSE_OK;
                }
            case EXIT:
                /* drop_until_brace takes care of things and effectively
                 * consumes the closing bracket (if found)*/
                if(drop_until_brace() == PARSE_ERR)
                    return PARSE_ERR;
                return PARSE_OK;
            default: /* check for variable declarations, and if not, well, wtf */
                if(!is_type(first)) {
                    sprintf(err_buff,"(%i)unexpected token %s (code %i)",
                            __LINE__,yytext,first);
                    p_error(err_buff);
                    return PARSE_ERR;
                }
                if(declaration(first,&first) == PARSE_ERR)
                    return PARSE_ERR;
                sprintf(err_buff,"(%i)token after declaration: %s",
                        __LINE__,yytext);
                verbose(err_buff);
        }
    }
    return PARSE_ERR;
}

/*
 * Previous token: ?
 * Description: parses an expression and
 * computes return type
 * Args: 
 *   - *ret_type: stores the return type
 *   of the expression
 * Lookahead: ?
 * Eq. rule:
 *      *check the grammar file
 */
int nexp(int prev, int *ret_type) {
    //TODO: compute return type (well shitttt)
    int func_or_var;
    SymbolRegister *got = NULL;
    switch(prev) {
        case ARROW:
            return nexp(yylex(),ret_type);
        case INT_N:
            *ret_type = INT;
            return PARSE_OK;
        case FLO_N:
            *ret_type = FLOAT;
            return PARSE_OK;
            /* explanation: returns OK so that the parser
             * will complain at the next error, if any    */
        case ID:
            got = table->get_symbol(yylval.sval);
            if(!got) { /* not even in the table */
                sprintf(err_buff,"(%i)%s was not declared",
                        __LINE__,yylval.sval);
                p_error(err_buff);
                return PARSE_ERR;
            } else { /* in the table: let's check type */
                *ret_type = got->get_return(); // we can get this now
                func_or_var = got->get_type();
                /* -- take value of identifier -- */
                if(func_or_var == VAR_T || func_or_var == ARG_T) { /* var/arg? */
                    // do womething about the type
                    return PARSE_OK;
                }
                /* -- take return from call -- */
                else if(func_or_var == FUNC_T) { /* found a function */
                    /* check the call */
                    if(call() == PARSE_ERR)
                        return PARSE_ERR;
                    // do something with the type
                }
                return PARSE_OK;
            }
        default:
            /* very ugly solution to use the default case, but  *
             * it allows for a more decent error message        */
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
 * Previous token: ?
 * Description: parses an expression and
 * computes return type
 * Args: 
 *   - *ret_type: stores the return type
 *   of the expression
 * Lookahead: ?
 * Eq. rule:
 *      *check the grammar file
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
            //      - is_logic_oper
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
 * Previous token: {type}
 * Description: fairly trivial
 * Args:
 *   - prev: the argument type
 * Lookahead: 1
 * Eq. rule:
 *      {type} ':' ID
 */
int argument(int prev) {
    /* lex has been called already! */
    int type = prev;

    if(expect(':') == PARSE_ERR)
        return PARSE_ERR;

    if(expect(ID) == PARSE_ERR)
        return PARSE_ERR;
    char *name = strdup(yylval.sval);

    table->store_symbol(ARG_T,type,0,name); //TODO: FIX
    free(name);
    return PARSE_OK;
}

/*
 * Previous token: ID (already verified as function)
 * Description: checks for passed params and checks
 * each param's type against correspondant function
 * argument
 * Args: none
 * Lookahead: 1
 * Eq. rule:
 *      [ID] '(' parameter* ')'
 * Special return: function return type (rly? better just int*)
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

