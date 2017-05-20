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
#include "codegen.h"
#include "format.h"
#include "parser.h"
#include "symtable.h"

int line=1;
char msg_buff[256] = "";

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
    sprintf(msg_buff,__VA_ARGS__);\
    p_error(msg_buff);\
}

#define verbose(...) {\
    sprintf(msg_buff,__VA_ARGS__);\
    p_verbose(msg_buff);\
}


// oneliner for checking if token is type keyword
bool is_type(int token) {
    return 
        token == INT ||
        token == UINT ||
        token == CHAR ||
        token == STRING ||
        token == FLOAT  ||
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
    int next, i=0;
    while((next = yylex()) != '}') {
        error("** dropping \"%s\" token",yytext);
    } if(next != '}') {
        error("reached end of file: '{' is not closed");
        return PARSE_ERR;
    }
    if(i != 0)
        return PARSE_ERR;
    return PARSE_OK;
}

// finds a name in the symbol table and puts the relevant info in the pointers passed
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

// auxiliar, but almost part of the code: it
// settles between boolean and numeric expressions
int expression(int prev, int *ret_type) {
    if(is_num_oper(prev)
            || prev == FLO_N
            || prev == INT_N) {
        verbose("parsing numerical expression");
        return nexp(prev,ret_type);
    }
    if(is_logic_oper(prev)
            || prev == TRUE
            || prev == FALSE) {
        verbose("parsing logical expression");
        *ret_type = BOOL;
        return bexp(prev);
    }
    if(prev == ID) {
        SymbolRegister *reg = NULL;
        int func_or_var, type;
        if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) {
            error("%s was not previously declared",yylval.sval);
            return PARSE_ERR;
        }
        if(type == BOOL) {
            verbose("parsing logical expression");
            *ret_type = BOOL;
            return bexp(prev);
        } else if(type != VOID) {
            verbose("parsing numerical expression");
            return nexp(prev,ret_type);
        }
    }
    return PARSE_ERR;
}


/*==============================================*/
/** ** ** ** **   MAIN FUNCTION    ** ** ** ** **/
/*==============================================*/

int main(int argc, char *argv[]) {
    if(argc>1)
        if((yyin = fopen(argv[1],"r")) == NULL)
            return -1;
    // use q_line as temporary buffer to store filename
    sprintf(msg_buff,argv[1]);

    try {
        init_q_file(strcat(msg_buff,".q.c"));
    } catch(const char *msg) {
        fprintf(stderr,msg);
    }
    verbose("Entry point: object file initialized*");

    int ret = program();

    qgen("END\n");
    quit_codegen();

    fflush(stdout);
    if(ret == PARSE_OK)
        printf(BOLD GREEN "Holy shit, it compiled!\n" CLEAR_FORMAT);
    printf("return: %i\nState of the symbol table:\n",ret);
    table->print();
    return ret;
}

/*==============================================*/
/** ** ** ** **  RULE FUNCTIONS ** ** ** ** ** **/
/*==============================================*/


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
            next = peek; // in declaration we peeked +1 ahead
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
 * *peeked.
 * Lookahead: 2
 * Eq. rule:
 *      {type} ID ("<-" nexp)?
 */
int declaration(int prev, int *peeked) {
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
        verbose("declaration: no-assignment");
        *peeked = next;
        goto store_and_exit;
    }

    verbose("declaration: perceived arrow");
    // next is arrow, a.k.a. useless here
    next = yylex();
    verbose("declaration: token after arrow is %s",yytext);

    // a scan assignment
    if(next == SCAN) {
        verbose("declaration: oh, a scan call!");
        *peeked = yylex();
        goto store_and_exit; // done with it
    }

    // a literal string assingment
    if(next == STR) {
        verbose("declaration: a literal string in the assignment");
        if(expected_type == STRING) {
            verbose("declaration: correct string assignment");
            *peeked = yylex();
            goto store_and_exit;
        } else {
            error("trying to assign a string literal to a %i var/arg",
                    expected_type);
            return PARSE_ERR;
        }
    }

    if(nexp(next,&parsed_type) == PARSE_ERR) {
        return PARSE_ERR;
    }
    if(expected_type != parsed_type) {
        error("declaration: %s was declared as %i; tried to assign type %i",
                name,expected_type,parsed_type);
        free(name);
        return PARSE_ERR;
    }
    *peeked = yylex();

store_and_exit:
    try {
        table->store_symbol(VAR_T, expected_type, 0, name);
    } catch(const char* msg) {
        p_error(msg);
        return PARSE_ERR;
    }
    verbose("declaration: symbol %s successfully stored",name);
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
    qgen_tag(name);

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
 *      "omo" ':' {kyo | seisu}
 *      '{' code '}'
 */
int mn() {
    int ret;
    /* we are DONE with function declarations: we can now only access
     * global variables (scope: 0) and those declared */
    table->set_scope(0);
    if(expect(':') == PARSE_ERR) //:
        return PARSE_ERR;

    if((ret = expect(0,true)) == PARSE_ERR) //type
        return PARSE_ERR;
    if(ret != VOID && ret != INT) {
        error("main function cannot be of a type \
                other than \"kyo\" or \"seisu\"");
        return PARSE_ERR;
    }

    if(expect('{') == PARSE_ERR) // {
        return PARSE_ERR;

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
    int next = yylex(), func_or_var, type;
    int plus_or_minus; // if we save the + / - we can avoid repeating their code twice
    int actual_ret = ret; // relevant in the RET case
    SymbolRegister *reg = NULL;
    while(next != '}') {
        verbose("code: next statement starts with %s",yytext);
        switch(next) {
            /* the incrementor and decrementor operators:  *
             * we have to check for the "twin", and if not *
             * found the expression makes no sense         */
            case '-':
            case '+':
                // look ahead for twin
                // if found, look for identifier
                // else parse two expressions
                plus_or_minus = next;
                next = yylex();
                if(next == plus_or_minus) { // the perceived operand, + or -
                    next = yylex();
                    if(next != ID) {
                        error("code: expected an integer variable, but got %s",
                                yytext);
                        return PARSE_ERR;
                    } else if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) { // reuse the variables
                        return PARSE_ERR;
                    } else if(func_or_var == FUNC_T) {
                        error("code: can't apply a unary operator on a function call");
                        return PARSE_ERR;
                    } else if(type != INT && type != UINT) {
                        error("code: can't apply a unary operator on a non integer variable (type is %i)",
                                type);
                        return PARSE_ERR;
                    }
                    // increment magic!
                    verbose("code: incre/decrementing variable %s",yylval.sval);
                    next = yylex();
                    break;
                } else {
                    // just making my life easier
                    error("code: expressions in the wild make no sense");
                    return PARSE_ERR;
                }
            case ID:
                /* a call or an assignment */
                verbose("code: checking the symbol table");
                if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) {
                    error("code: %s was not previously declared",
                            yylval.sval);
                    return PARSE_ERR;
                } else {
                    if (func_or_var == FUNC_T) {
                        verbose("code: symbol %s is a function",yylval.sval);
                        //in this instance we don't need to check the return type... nobody is expecting it!
                        if(call() == PARSE_ERR)
                            return PARSE_ERR;
                        next = yylex();
                        verbose("code: token going into the next iteration: %s", yytext);
                        break; // will this break out of the switch, or the while?
                    } else if (func_or_var == VAR_T || func_or_var == ARG_T) { 
                        verbose("code: symbol %s is a var / arg",yylval.sval);
                        // look for asignment
                        if(expect(ARROW) == PARSE_ERR)
                            return PARSE_ERR;

                        int expected_ret = type,
                            parsed_ret;
                        // next is arrow, so we go ahead
                        next = yylex();
                        verbose("code: token after assignment arrow is %s",yytext);
                        if(next == STR) {
                            if(type == STRING) {
                                verbose("code: correct string assignment");
                                next = yylex();
                                break;
                            } else {
                                error("code: trying to assign a string literal to a %i var/arg",
                                        type);
                                return PARSE_ERR;
                            }
                        }
                        if(next == SCAN) {
                            verbose("code: oh, a scan call!");
                            next = yylex();
                            break;
                        }
                        if(nexp(next,&parsed_ret) == PARSE_ERR)
                            return PARSE_ERR;
                        if(expected_ret != parsed_ret) {
                            error("code: trying to assign a %i value to %s, which is of type %i",
                                    parsed_ret,reg->get_name(),expected_ret);
                            return PARSE_ERR;
                        }
                        // done with the assignment
                        next = yylex();
                        break;
                    }
                }
            case IF:
                next = yylex();
                if(bexp(next) == PARSE_ERR) // check for the condition
                    return PARSE_ERR;
                if(expect('{') == PARSE_ERR) {
                    error("code: sorry, but conditional statements must be surrounded by curly brackets");
                }
                // this is a complication... VOID type? what if we perceive a kisu?
                if(code(ret*1000) == PARSE_ERR) // consumes the closing '}'
                    return PARSE_ERR;
                next = yylex(); // look ahead for ELSE
                if(next == ELSE) {
                    verbose("code: perceived ELSE after IF");
                    if(expect('{') == PARSE_ERR) {
                        error("code: sorry, but conditional statements must be surrounded by curly brackets");
                    }
                    if(code(ret*1000) == PARSE_ERR) // consumes the closing '}' // does it??
                        return PARSE_ERR;
                    next = yylex();
                }
                break; // next token already fetched
            case WHILE:
                /* include here bexpr, the '{' code '}' */
                next = yylex();
                if(bexp(next) == PARSE_ERR) // check for the condition
                    return PARSE_ERR;
                if(expect('{') == PARSE_ERR) {
                    error("code: sorry, but conditional statements must be surrounded by curly brackets");
                }
                if(code(ret*1000) == PARSE_ERR) // consumes the closing '}'
                    return PARSE_ERR;
                next = yylex(); // done with this statement
                break; // next token already fetched
            case PRINT:
                /* parse '(' parameter* ')' */
                /* or... could I just use call()? as long as it processes the arguments correctly...
                 * if it becomes hellish to put the arguments into the object, just copy the code */
                if(call() == PARSE_ERR)
                    return PARSE_ERR;
                next = yylex(); // done with this statement
                break;
            case RET:
                /* several things here:
                 * -check for parameter, and for type correctness *
                 * -also, throw error if func is not VOID and no  *
                 *  parameters are found                          *
                 * -check for '}': ret is final to a code block   *
                 *    -if not found, print error and iterate      *
                 *    through tokens until one is found (doing    *
                 *    nothing with them) and see what happens     */
                int parsed_ret;
                while(actual_ret > 1000) {// we are actually inside a conditional code block
                    verbose("we are actually inside a conditional guarded code block (ret = %i > 1000)",ret);
                    actual_ret = actual_ret / 1000;
                }

                if(actual_ret == VOID) {
                    if(expect('}') == PARSE_ERR) {
                        p_error("code: returning something in a void function!");
                        return PARSE_ERR;
                    }
                    return PARSE_OK;
                } else {
                    next = yylex();
                    /* var/arg or call */
                    if(next == ID) {
                        if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) {
                            error("code: %s was not previously declared",
                                    yylval.sval);
                            return PARSE_ERR;
                        } else if(type != actual_ret) { // return type coherence check
                            error("code: expected %i return type: %s is of %i type",
                                    actual_ret,reg->get_name(),type);
                            return PARSE_ERR;
                        } else if(func_or_var == FUNC_T) {
                            verbose("code: returning a function call");
                            if(call() == PARSE_ERR)
                                return PARSE_ERR;
                            next = yylex();
                            break;
                        } else { // ARG_T / VAR_T, but we don't quite care
                            verbose("code: returning a var / arg");
                            next = yylex();
                            verbose("code: next after return is '%s'",yytext);
                            break;
                        }
                    }
                    /* expression */
                    else if(expression(next,&parsed_ret) == PARSE_ERR) {
                        return PARSE_ERR;
                    } else if(parsed_ret != actual_ret) {
                        error("expression returns type %i, when function returns type %i",
                                parsed_ret,actual_ret);
                        return PARSE_ERR;
                    }
                    verbose("code: returning an expression!");
                    next = yylex();
                    break;
                }
            case EXIT:
                /* drop_until_brace takes care of things and effectively
                 * consumes the closing bracket (if found) */
                if(drop_until_brace() == PARSE_ERR)
                    return PARSE_ERR;
                return PARSE_OK;
            default: /* check for variable declarations, and if not, well, wtf */
                if(!is_type(next)) {
                    error("code: unexpected token %s (code %i)",
                            yytext,next);
                    return PARSE_ERR;
                }
                /* it's a type keyword, so we expect a declaration */
                if(declaration(next,&next) == PARSE_ERR)
                    return PARSE_ERR;
                verbose("code: token after declaration: %s",
                        yytext);
        }
    }
    return PARSE_OK;
}

/*
 * Previous token: ? (but it is what yylex() has returned)
 * Description: parses an expression and
 * computes return type
 * Args: 
 *   - *ret_type: stores the return type
 *   of the expression
 * Lookahead: 1
 * Eq. rule:
 *      *check the grammar file
 */
int nexp(int prev, int *ret_type) {
    //TODO: compute return type in the integers case (well shitttt)
    int func_or_var, type, operand_type;
    SymbolRegister *reg = NULL;

    verbose("nexp: token %s", yytext);

    switch(prev) {
        case INT_N:
            verbose("nexp: it's an integer number");
            *ret_type = INT;
            return PARSE_OK;
        case FLO_N:
            verbose("nexp: it's a floating point number");
            *ret_type = FLOAT;
            return PARSE_OK;
            /* explanation: returns OK so that the parser
             * will complain at the next error, if any    */
        case ID:
            if(!find_in_table(yylval.sval,reg,&func_or_var,&type)) { /* not even in the table */
                error("nexp: %s was not declared",
                        yylval.sval);
                return PARSE_ERR;
            } else  {
                *ret_type = type;
                if(func_or_var == VAR_T || func_or_var == ARG_T) { /* var/arg? */
                    return PARSE_OK;
                } else if(func_or_var == FUNC_T) { /* found a function */
                    /* check the call */
                    if(call() == PARSE_ERR)
                        return PARSE_ERR;
                }
                return PARSE_OK;
            }
        case '*':
        case '/':
        case '%':
        case '-':
        case '+':
            // NO look ahead for twin
            // sorry, but I just can't afford that added complexity

            // parse two expressions
            if(nexp(yylex(),&operand_type) == PARSE_ERR) {
                error("nexp: unable to parse a first expression after '%c'",prev);
                return PARSE_ERR;
            }
            verbose("nexp: first operand of '%c' parsed",prev);
            *ret_type = operand_type; // type is constrained to the first operand type
            if(nexp(yylex(),&operand_type) == PARSE_ERR) {
                error("nexp: unable to parse a second expression after '%c'",prev);
                return PARSE_ERR;
            }
            if(operand_type != *ret_type) {
                verbose("nexp: the types of the two operands are not the same!: %i and %i",
                        operand_type,*ret_type);
            }
            verbose("nexp: second operand of '%c' parsed",prev);
            return PARSE_OK;
        default:
            /* very ugly solution to use the default case, but  *
             * it allows for a more decent error message        */
            error("nexp: unexpected token %s; expected operator",yytext);
            return PARSE_ERR;
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
    SymbolRegister *reg = NULL;
    int func_or_var, type, operand_type, peek;
    switch(prev) {
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

        case '!':
            peek = yylex();
            if(bexp(peek) == PARSE_ERR) {
                error("bexp: unable to parse logical expression after '!'");
                return PARSE_ERR;
            }
            return PARSE_OK;
            /* the easy ones (they only apply to boolean expressions)*/
        case '&':
        case '|':
            // parse 2 b expressions
            peek = yylex();
            if(bexp(peek) == PARSE_ERR) {
                error("bexp: unable to parse first operand of logical '%c'",prev);
                return PARSE_ERR;
            }
            if(bexp(yylex()) == PARSE_ERR) {
                error("bexp: unable to parse second operand of logical '%c'",prev);
                return PARSE_ERR;
            }
            return PARSE_OK;
        case '=':
            //peek = yylex();
            if(nexp(yylex(),&operand_type) == PARSE_ERR) {
                error("bexp: unable to parse first operand of comparison");
                return PARSE_ERR;
            }
            verbose("bexp: first operand of comparison parsed");
            type = operand_type;
            if(nexp(yylex(),&operand_type) == PARSE_ERR) {
                error("bexp: unable to parse second operand of comparison");
                return PARSE_ERR;
            }
            if(operand_type != type) {
                verbose("bexp: comparison operands do not match in type: %i and %i",
                        type, operand_type);
            }
            if(operand_type == STR || type == STR) {
                error("bexp: operations involving strings are not allowed");
                return PARSE_ERR;
            }
            return PARSE_OK;
        case '>':
        case '<':
            // check for '='
            peek = yylex();
            if(peek == '=') {
                verbose("bexp: '%c%c'",prev,peek);
                peek = yylex();
                if(nexp(peek,&operand_type) == PARSE_ERR) {
                    error("bexp: unable to parse first operand of comparison");
                    return PARSE_ERR;
                }
                verbose("bexp: first operand of comparison parsed");
                type = operand_type;
                if(nexp(yylex(),&operand_type) == PARSE_ERR) {
                    error("bexp: unable to parse second operand of comparison");
                    return PARSE_ERR;
                }
                if(operand_type != type) {
                    verbose("bexp: comparison operands do not match in type: %i and %i",
                            type, operand_type);
                }
                if(operand_type == STR || type == STR) {
                    error("bexp: operations involving strings are not allowed");
                    return PARSE_ERR;
                }
                verbose("bexp: second operand of comparison parsed");
                return PARSE_OK;
            } else {
                verbose("bexp: '%c'",prev);
                if(nexp(peek,&operand_type) == PARSE_ERR) {
                    error("bexp: unable to parse first operand of comparison");
                    return PARSE_ERR;
                }
                verbose("bexp: first operand of comparison parsed");
                type = operand_type;
                if(nexp(yylex(),&operand_type) == PARSE_ERR) {
                    error("bexp: unable to parse second operand of comparison");
                    return PARSE_ERR;
                }
                verbose("bexp: second operand of comparison parsed");
                if(operand_type != type) {
                    verbose("bexp: comparison operands do not match in type: %i and %i",
                            type, operand_type);
                }
                if(operand_type == STR || type == STR) {
                    error("bexp: operations involving strings are not allowed");
                    return PARSE_ERR;
                }
                return PARSE_OK;
            }
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
 * Previous token: ID|tsutaeru (already verified as function)
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
    if(expect('(') == PARSE_ERR)
        return PARSE_ERR;
    // ambiguity! in the number of arguments so count them
    int param_count = 0, of_type, next;
    next = yylex();
    while(next != ')') {
        /* just let the parameter() function take 
         * care of it, then check param type */
        if(parameter(next,&of_type) == PARSE_ERR)
            return PARSE_ERR;
        verbose("call: parsed a %i type parameter",of_type);
        param_count++;
        next = yylex();
    }
    verbose("call: parsed %i parameters",param_count);
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
int parameter(int prev, int *type) {
    //int next = yylex();
    /* note: this block is actually significant to the 
     * language spec, since we are basically saying that
     * no operations can be performed on string literals,
     * which makes them pretty useless. but it keeps this
     * particular function extremely simple */
    if(prev == STR) {
        *type = STR;
        return PARSE_OK;
    } else
        return expression(prev,type);
}

