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
// this variable is used every time a function
// definition is parsed: it is incremented so
// that each argument can be stored along with it's
// position. it must be reset once the function is
// parsed
int local_num_args = 0;

extern YYSTYPE yylval;
extern char *yytext;
extern FILE *yyin;

SymbolTable *table = new SymbolTable();

/*==============================================*/
/** ** **        HELPER FUNCTIONS        ** ** **/
/*==============================================*/

// useful for debugging
void p_verbose(const char* msg) {
    char buffer[256] = "";
    sprintf(buffer,"parser@%i: " BLUE "%s" CLEAR_FORMAT "\n", line, msg);
    fprintf(stdout,buffer);
    fflush(stdout);
}

// formats error messages
void p_error(const char* msg) {
    fprintf(stderr,"parser@%i: syntax error " RED "%s" CLEAR_FORMAT "\n" ,line,msg);
}

#define error(...) {\
    sprintf(err_buff,__VA_ARGS__);\
    p_error(err_buff);\
}

#define verbose(...) {\
    sprintf(err_buff,__VA_ARGS__);\
    p_verbose(err_buff);\
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
            error("unexpected token %s (code %i); expected a type",
                    yytext,next);
            if(yytext) free(yytext);
            return PARSE_ERR;
        }
        return next; // type
    }
    else if(next != expected) {
        error("unexpected token %s (code %i); expected %i",
                yytext,next,expected);
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
        error("** dropping \"%s\" token",yytext);
    } if(next != '}') {
        error("reached end of file: '{' is not closed");
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

bool find_in_table(char *name, SymbolRegister *r, int *func_or_var, int *type) {
    r = table->get_symbol(name,table->get_scope()); // if not, search in the global scope
    if(!r)
        r = table->get_symbol(name,0);
    if(!r)
        return false;
    *func_or_var = r->get_type();
    *type = r->get_return();
    return true;
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
    return 0;
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
        if(is_type(next)) {
            verbose("program: perceived type keyword");
            if(declaration(next,&peek) == PARSE_ERR)
                return PARSE_ERR;
            next = peek; // in declaration we looked +1 ahead
        }

        /* verify definition correct */
        else if(next == FUNC) {
            verbose("program: perceived function keyword");
            if(definition() == PARSE_ERR)
                return PARSE_ERR;
            next = yylex();
        }
        /* not main and not the previous */
        else {
            error("program: unexpected token %s (code %i)",
                    yytext,next);
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
    int next, expected_type = prev, parsed_type;
    char *name;

    verbose("declaration: type keyword detected: %s",yytext);

    if(expect(ID) == PARSE_ERR)
        return PARSE_ERR;
    name = strdup(yylval.sval);
    verbose("declaration: identifier taken");

    next = yylex();
    verbose("declaration: token after id is %s",yytext);
    if(next != ARROW) {
        *looked = next;
        try {
            table->store_symbol(VAR_T, expected_type, 0, name);
        } catch(const char* msg) {
            p_error(msg);
            return PARSE_ERR;
        }

        verbose("no-assignment: symbol %s successfully stored",name);
        return PARSE_OK;
    }

    /* next is arrow, a.k.a. useless here */
    next = yylex();
    verbose("declaration: token after arrow is %s",yytext);
    if(nexp(next,&parsed_type) == PARSE_ERR)
        return PARSE_ERR;

    if(expected_type != parsed_type) {
        error("declaration: %s was declared as %i; tried to assign type %i",
                name,expected_type,parsed_type);
        free(name);
        return PARSE_ERR;
    }

    try {
        table->store_symbol(VAR_T, expected_type, 0, name);
    } catch(const char* msg) {
        p_error(msg);
        return PARSE_ERR;
    }
    verbose("assignment: symbol %s successfully stored",name);
    free(name);
    return PARSE_OK;
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
    int ret,next;
    local_num_args = 0;

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
    func_counter++;
    table->set_scope(func_counter);
    while(next != '{') { /* still not the code */
        verbose("processed: argument");
        if(argument(next) == PARSE_ERR)
            return PARSE_ERR;
        local_num_args++;
        next = yylex();
    }
    // everything ok up to here, so we register the function
    table->set_scope(0);
    try {
        table->store_symbol(FUNC_T, ret, local_num_args, name);
    } catch(const char* msg) {
        p_error(msg);
        return PARSE_ERR;
    }
    table->set_scope(func_counter);
    local_num_args = 0;

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
    /* we are DONE with function declarations: we can now only access
     * global variables (scope: 0) and those declared */
    table->set_scope(0);
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
    int first = yylex(), func_or_var, type;
    int plus_or_minus = first; // if we save the + / - we can avoid repeating their code twice
    SymbolRegister *reg = NULL; // let's get this out of the way
    // TODO: also, lacking closing '}'
    // or maybe recursion, but that could be bad
    while(first != '}') {
        verbose("code: next statement starts with %s",yytext);
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
                        error("code: the increment operator only works on variables");
                        return PARSE_ERR;
                    } else if(!(reg = table->get_symbol(yylval.sval))) {
                        error("code: %s was not previously declared",yylval.sval);
                        return PARSE_ERR;
                    } else if(reg->get_type() != INT) {
                        error("code: the decrement/increment operator only works on variables of integer type");
                        return PARSE_ERR;
                    } else {
                        // do your increment magic here
                    }
                } else {
                    error("code: the only allowed statement that starts with '%c' is a decrement of a variable", plus_or_minus);
                    return PARSE_ERR;
                }
            case ID:
                /* a call or an assignment */
                if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) {
                    error("code: %s was not previously declared",
                            yylval.sval);
                    return PARSE_ERR;
                } else if (type == FUNC_T) {
                    //in this instance we don't need to check the return type... nobody is expecting it!
                    if(call() == PARSE_ERR)
                        return PARSE_ERR;
                    break; // will this break out of the switch, or the while?
                } else if (type == VAR_T || type == ARG_T) { 
                    // look for asignment
                    if(expect(ARROW) == PARSE_ERR)
                        return PARSE_ERR;
                    //TODO: missing the SCAN possibility... ought to peek one more
                    int expected_ret = type,
                        parsed_ret;
                    if(nexp(-1,&parsed_ret) == PARSE_ERR)
                        return PARSE_ERR;
                    if(expected_ret != parsed_ret) {
                        error("code: trying to assign a %i value to %s, which is of type %i",
                                parsed_ret,reg->get_name(),expected_ret);
                        return PARSE_ERR;
                    }
                    break;
                }
                return PARSE_ERR;
            case IF:
                if(bexp(-1) == PARSE_ERR) // check for the condition
                    return PARSE_ERR;
                if(expect('{') == PARSE_ERR) {
                    error("code: sorry, but conditional statements must be surrounded by curly brackets");
                }
                if(code(-1) == PARSE_ERR) // consumes the closing '}'
                    return PARSE_ERR;
                first = yylex(); // look ahead for ELSE
                if(first == ELSE) {
                    if(expect('{') == PARSE_ERR) {
                        error("code: sorry, but conditional statements must be surrounded by curly brackets");
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
                    error("code: sorry, but conditional statements must be surrounded by curly brackets");
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
                        p_error("code: returning something in a void function!");
                        return PARSE_ERR;
                    }
                    return PARSE_OK;
                } else {
                    first = yylex();
                    if(first == ID) { // identifier or call?
                        if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) {
                            error("code: %s was not previously declared",
                                    yylval.sval);
                            return PARSE_ERR;
                        } else if(type != ret) { // return type coherence check
                            error("code: expected %i return type: %s is of %i type",
                                    ret,reg->get_name(),type);
                            return PARSE_ERR;
                        } else if(func_or_var == FUNC_T) {
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
                 * consumes the closing bracket (if found) */
                if(drop_until_brace() == PARSE_ERR)
                    return PARSE_ERR;
                return PARSE_OK;
            default: /* check for variable declarations, and if not, well, wtf */
                if(!is_type(first)) {
                    error("code: unexpected token %s (code %i)",
                            yytext,first);
                    return PARSE_ERR;
                }
                /* it's a type keyword, so we expect a declaration */
                if(declaration(first,&first) == PARSE_ERR)
                    return PARSE_ERR;
                verbose("code: token after declaration: %s",
                        yytext);
        }
    }
    return PARSE_ERR;
}

/*
 * Previous token: ? (but it is what yylex() has returned)
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
    int func_or_var, type;
    SymbolRegister *reg = NULL;

    verbose("nexp: token %s", yytext);

    switch(prev) {
        case INT_N:
            *ret_type = INT;
            return PARSE_OK;
        case FLO_N:
            *ret_type = FLOAT;
            return PARSE_OK;
            /* explanation: returns OK so that the parser
             * will complain at the next error, if any    */
        case ID:
            if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) { /* not even in the table */
                error("nexp: %s was not declared",
                        yylval.sval);
                return PARSE_ERR;
            } else if(func_or_var == VAR_T || func_or_var == ARG_T) { /* var/arg? */
                return PARSE_OK;
            } else if(func_or_var == FUNC_T) { /* found a function */
                /* check the call */
                if(call() == PARSE_ERR)
                    return PARSE_ERR;
                // do something with the type
            }
            return PARSE_OK;
        default:
            /* very ugly solution to use the default case, but  *
             * it allows for a more decent error message        */
            if(!is_num_oper(prev)) {
                error("nexp: unexpected token %s; expected operator",yytext);
                return PARSE_ERR;
            }
            /* back at it */
            int operand = prev, operation_type, peek;
            switch(prev) {
                //int inmediate = -1;
                /* the 'lonely' operators -> the 'coupled' operands */
                case '*':
                case '/':
                case '%':
                    // parse two expressions
                    if(nexp(yylex(),&operation_type) == PARSE_ERR) {
                        error("nexp: unable to parse a first expression after '%c'",operand);
                        return PARSE_ERR;
                    }
                    if(nexp(yylex(),ret_type) == PARSE_ERR) {
                        error("nexp: unable to parse a second expression after '%c'",operand);
                        return PARSE_ERR;
                    }
                    if(operation_type != *ret_type) {
                        error("nexp: the types of the two operands are not the same!: %i and %i",
                                operation_type,*ret_type);
                        return PARSE_ERR;
                    }
                    return PARSE_OK;
                case '-':
                    // look ahead for twin
                    // if found, look for identifier
                    // else parse two expressions
                case '+':
                    peek = yylex();
                    if(peek == operand) { // the perceived operand, + or -
                        peek = yylex();
                        if(peek != ID) {
                            error("nexp: expected an integer variable, but got %s",
                                    yytext);
                            return PARSE_ERR;
                        } else if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) { // reuse the variables
                            return PARSE_ERR;
                        } else if(func_or_var == FUNC_T) {
                            error("nexp: can't apply a unary operator on a function call");
                            return PARSE_ERR;
                        } else if(type != INT) {
                            error("nexp: can't apply a unary operator on a non integer variable");
                            return PARSE_ERR;
                        }
                        return PARSE_OK;
                    }
                default: return PARSE_ERR;
            }
    }
    /* we got nothing? */
    return PARSE_OK;
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
    SymbolRegister *reg = NULL;
    int func_or_var, type;
    switch(prev) {
        case -1:
            // previous is the control structure calling
            break; // or do I want to fallthrough?
            // probably need to place it somewhere else
        case TRUE:
        case FALSE:
            return PARSE_OK;
        case ID: /* it must be a call then: there are no */
                    /* boolean variables in the language    */
            if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) { /* not even in the table */
                error("nexp: %s was not declared",
                        yylval.sval);
                return PARSE_ERR;
            } else if(func_or_var == VAR_T || func_or_var == ARG_T) { /* var/arg? either way, no such thing is allowed */
                return PARSE_ERR;
            } else if(func_or_var == FUNC_T) { /* found a function */
                /* check the call */
                if(call() == PARSE_ERR)
                    return PARSE_ERR;
            }
            return PARSE_OK;
            
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

    if(expect(':') == PARSE_ERR) {
        error("expected ':', got %s",yytext);
        return PARSE_ERR;
    }

    if(expect(ID) == PARSE_ERR) {
        error("expected identifier, got %s",yytext);
        return PARSE_ERR;
    }
    char *name = strdup(yylval.sval);

    try {
        table->store_symbol(ARG_T,type,local_num_args,name);
    } catch(const char* msg) {
        p_error(msg);
        return PARSE_ERR;
    }
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

