#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bison-bridge.h"
#include "codegen.h"
#include "format.h"
#include "parser.h"
#include "symtable.h"

#define error(...) {\
    sprintf(msg_buff,__VA_ARGS__);\
    p_error(msg_buff);\
}

#define verbose(...) {\
    sprintf(msg_buff,__VA_ARGS__);\
    p_verbose(msg_buff);\
}

typedef struct {
    YYSTYPE lexed;
    char *text;
    int code;
} token;

token next;

extern YYSTYPE yylval;
extern char *yytext;
extern FILE *yyin;

int line=1;
char msg_buff[256] = "";

SymbolTable *table = new SymbolTable();

// internal counter that actually defines
// a sort of scope: it increments each on
// definition (not in code blocks). what
// this means is that variables are actually
// available to the rest of the function
// after their declaration, no matter where
// that is.
int func_counter = 0;

// this variable is used every time a function
// definition is parsed: it is incremented so
// that each argument can be stored along with it's
// position. it must be reset once the function is
// parsed
int local_num_args = 0;

// return type of a code block: to be set when
// the return type is found, and reset after function
// code is finished (in case of main, it doesn't really
// matter)
int return_type = 0;

unsigned int int_fmt_str_addr = 0;
unsigned int char_fmt_str_addr = 0;

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

// NOTE: to be consistent across the code, when a rule is
// complete, the next token must already be lexed
//
// consumes a token
void shift() {
    int code = yylex();     // invokes yylex

    if(next.text)
        free(next.text);
    next.code = code;       // stores new info
    next.lexed = yylval;
    next.text = strdup(yytext);

    //verbose("shift: (%i,%s)",code,next.text);
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

// finds a name in the symbol table and puts the relevant info in the pointers passed
bool find_in_table(char *name, SymbolRegister *r, int *func_or_var, int *type, unsigned int *info=NULL) {
    r = table->get_symbol(name,table->get_scope()); // if not, search in the global scope
    if(!r)
        r = table->get_symbol(name,0);
    if(!r)
        return false;
    *func_or_var = r->get_type();
    *type = r->get_return();
    if(info) *info = r->get_info();
    return true;
}

// auxiliar, but almost part of the code: it
// settles between boolean and numeric expressions
// !! only called once
int expression() {
    if(is_num_oper(next.code)
            || next.code == FLO_N
            || next.code == INT_N) {
        return nexp();
    }
    if(is_logic_oper(next.code)
            || next.code == TRUE
            || next.code == FALSE) {
        return bexp(true);
    }
    if(next.code == ID) {
        SymbolRegister *reg = NULL;
        int func_or_var, type;
        if(!find_in_table(next.text,reg,&func_or_var,&type)) {
            error("expression: %s was not previously declared",next.text);
            return PARSE_ERR;
        }
        if(type == BOOL) {
            return bexp(true);
        } else if(type != VOID) {
            return nexp();
        }
    }
    return PARSE_ERR;
}

/*==============================================*/
/** ** ** ** **   MAIN FUNCTION    ** ** ** ** **/
/*==============================================*/

int main(int argc, char *argv[]) {
    int ret;


    if(argc == 2) {
        if((yyin = fopen(argv[1],"r")) == NULL)
            return -1;
        sprintf(msg_buff,argv[1]);
        try {
            init_q_file(strcat(msg_buff,".q.c"));
        } catch(const char *msg) {
            fprintf(stderr,msg);
        }
        verbose("Entry point: object file initialized*");

        ret = program();
        qgen("END\n");
        quit_codegen();
    } else {
        ret = program();
    }
    if(ret == PARSE_OK)
        printf(BOLD GREEN "Compilation OK\n" CLEAR_FORMAT);
    printf("Returned: %i\nState of the symbol table:\n",ret);
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
 * Eq. rule:
 *      (declaration | definition)* main
 */
int program() {

    char *fmt_str;
    fmt_str = strdup("%%li");
    int_fmt_str_addr = qgen_str_stat(fmt_str);
    free(fmt_str);
    fmt_str = strdup("%%c");
    char_fmt_str_addr = qgen_str_stat(fmt_str);
    free(fmt_str);

    shift();
    while(next.code != MAIN) {
        /* verify declaration correct */
        if(is_type(next.code)) {
            verbose("program: perceived type keyword");
            if(declaration() == PARSE_ERR)
                return PARSE_ERR;
        }

        /* verify definition correct */
        else if(next.code == FUNC) {
            verbose("program: perceived function keyword");
            if(definition() == PARSE_ERR)
                return PARSE_ERR;
        }
        /* not main and not the previous */
        else {
            error("program: unexpected token %s (code %i)",
                    next.text,next.code);
            return PARSE_ERR;
        }

        // already shifted in either declaration or definition
    } 

    return mn();
}

/*
 * -Previous token: {type}
 * Description: consists of a type keyword, an
 * identifier and optionally an asignment. in
 * order to figure out if an assignment is next,
 * it consumes one extra token
 * Eq. rule:
 *      {type} ID ("<-" nexp)?
 */
int declaration() {
    // TODO: draw a nice grammar graph for this
    int expected_type, parsed_type, current_scope = table->get_scope();
    unsigned int addr = 0, string_size = 0;
    char *name;

    expected_type = next.code; // save the type

    shift();

    if(next.code != ID) {
        error("declaration: expected identifier, got %s",next.text);
        return PARSE_ERR;
    }
    name = strdup(next.text);
    verbose("declaration: identifier %s taken", name);

    shift();

    // store the variable (other than string)
    if(expected_type != STRING) {
        addr = qgen_declare_var(expected_type,current_scope);
    }

    if(next.code == '[') {
        if(expected_type != STRING) {
            error("declaration: brackets are only meant for string variables");
            free(name);
            return PARSE_ERR;
        } 

        // handle the possible number and closing bracket

        shift();

        if(next.code == INT_N) {
            if(next.lexed.dval < 1) {
                error("declaration: can't declare a negative sized string var");
                free(name);
                return PARSE_ERR;
            }
            string_size = next.lexed.dval;
            shift();
        }

        if(next.code != ']') {
            error("declaration: brackets not closed!");
            free(name);
            return PARSE_ERR;
        }

        shift();
    } // case '[' dealt with

    if(next.code == ARROW) {
        // next is arrow, so go ahead

        shift();

        if(next.code == STR) {
            if(expected_type != STRING) { // literal string
                error("declaration: trying to assign string literal to non string variable");
                free(name);
                return PARSE_ERR;
            }

            // TODO: qgen - str literal assignment

            shift();

        } else if(next.code == SCAN) { // uketoru
            if(expected_type == STRING) {
                if(string_size == 0) {
                    error("declaration: can't declare string without explicit size when initializing by scanning");
                    free(name);
                    return PARSE_ERR;
                }
                addr = qgen_declare_str_var_scan(STRING,current_scope);
            }

            qgen_scan(expected_type,addr);

            shift();

        } else { // not a string literal, not uketoru => nexp
            if((parsed_type = nexp()) == PARSE_ERR) {
                free(name);
                return PARSE_ERR;
            }
            if(expected_type != parsed_type) {
                error("declaration: %s was declared as %i; tried to assign type %i",
                        name,expected_type,parsed_type);
                free(name);
                return PARSE_ERR;
            }

            qgen_assign(expected_type,VAR_T,addr,current_scope);

            // already shifted in nexp()
        }
    } // case "<-" dealt with

    try {
        table->store_symbol(VAR_T, expected_type, addr, name);
    } catch(const char* msg) {
        p_error(msg);
        return PARSE_ERR;
    }


    free(name);

    return PARSE_OK;
}

/*
 * Previous token: "kansu" (no need to use it)
 * Description: process tokens until '}', meaning
 * the function body is complete. very clean
 * Eq. rule:
 *      "kansu" ID ':' {type} "<-" (argument)*
 *      '{' code '}'
 */
int definition() {
    local_num_args = 0;

    shift();

    if(next.code != ID) {
        error("definition: expected identifier, got %s",next.text);
        return PARSE_ERR;
    }

    char *name = strdup(yylval.sval);
    verbose("definition: function identifier %s taken",name);

    // tag for future calls
    qgen_tag(name);

    shift();

    if(next.code != ':') {
        error("definition: expected ':', got %s",next.text);
        return PARSE_ERR;
    }

    shift();

    if(!is_type(next.code)) {
        error("definition: expected type, got %s",next.text);
        return PARSE_ERR;
    }
    return_type = next.code;
    verbose("definition: function %s has return type %i",name,return_type);

    shift();

    if(next.code != ARROW) {
        error("definition: expected arrow, got %s",next.text);
        return PARSE_ERR;
    }

    shift();

    table->set_scope(++func_counter);

    while(next.code != '{') { /* still not the code */
        verbose("definition@%i: next.code = %i",__LINE__,next.code);
        if(argument() == PARSE_ERR)
            return PARSE_ERR;

        local_num_args++;

        // already shifted in argument()
    }

    verbose("definition: defined a total of %i arguments",local_num_args);

    //register the function on the global scope
    table->set_scope(0);
    try {
        table->store_symbol(FUNC_T, return_type, local_num_args, name);
    } catch(const char* msg) {
        p_error(msg);
        return PARSE_ERR;
    }
    table->set_scope(func_counter);
    local_num_args = 0;

    // next is '{', so go ahead
    shift();

    reset_local_vars();

    if(code() == PARSE_ERR)
        return PARSE_ERR;

    // code is parsed: reset return_type and scope
    return_type = 0;
    table->set_scope(0);
    return PARSE_OK;
}


/*
 * Previous token: "omo" (no need to use it)
 * Description: very similar to the function parsing
 * Eq. rule:
 *      "omo" ':' {kyo | seisu}
 *      '{' code '}'
 */
int mn() {

    table->set_scope(++func_counter); // last time this is called

    // next is "omo", so go ahead
    shift();

    if(next.code != ':') {
        error("main: expected ':', got %s",next.text);
        return PARSE_ERR;
    }

    shift();

    if(!is_type(next.code)) {
        error("main: expected return type, got %s",next.text);
        return PARSE_ERR;
    } else {
        return_type = next.code;
    }
    if(return_type != VOID && return_type != INT) {
        error("main function cannot be of a type \
                other than \"kyo\" or \"seisu\"");
        return PARSE_ERR;
    }

    shift();

    if(next.code != '{') {
        error("main: expected '{', got %s",next.text);
        return PARSE_ERR;
    }

    qgen("L 0:\t\t// entry point");

    // next is '{', so go ahead
    shift();

    reset_local_vars();

    if(code() == PARSE_ERR)
        return PARSE_ERR;

    // code is parsed: reset return_type
    return_type = 0;
    return PARSE_OK;
}

/*
 * no previous token
 * description: checks for code integrity and
 * for return type.
 * eq. rule:
 *      '{' {better see the grammar for this rule} '}'
 */
int code() {

    int func_or_var, type;
    int plus_or_minus; // if we save the + / - we can avoid repeating their code twice

    //int param_count = 0, offset = 0, of_type;
    unsigned int address, label1, label2, label3;
    SymbolRegister *reg = NULL;

    if(next.code == '}') {
        verbose("code: first token of code is '}'");
        shift();
        return PARSE_WARN;
    }

    while(next.code != '}') {
        verbose("code: next statement starts with %s",next.text);

        switch(next.code) {
        case '-':
        case '+':
            // look ahead for twin
            // if found, look for identifier
            // else parse two expressions
            plus_or_minus = next.code;

            shift();

            if(next.code == plus_or_minus) { // the perceived operand, + or -

                shift();

                if(next.code != ID) { // not a symbol
                    error("code: expected an integer variable, but got %s",
                            next.text);
                    return PARSE_ERR;
                } else if(!find_in_table(next.text, reg, &func_or_var, &type, &address)) { // not a declared symbol
                    return PARSE_ERR;
                } else if(func_or_var == FUNC_T) { // not a variable
                    error("code: can't apply a unary operator on a function call");
                    return PARSE_ERR;
                } else if(type != INT && type != UINT) { // not an int
                    error("code: can't apply a unary operator on a non integer variable (type is %i)",
                            type);
                    return PARSE_ERR;
                }

                // increment magic!
                verbose("code: increment magic");
                qgen_un_op(plus_or_minus,type,func_or_var,address,table->get_scope());

                shift();

                break;
            }
            else {
                // just making my life easier
                error("code: expressions in the wild make no sense");
                return PARSE_ERR;
            }
        case ID:
            /* a call or an assignment */
            if(!find_in_table(next.text,reg,&func_or_var,&type,&address)) {
                error("code: %s was not previously declared",
                        next.text);
                return PARSE_ERR;
            } else {
                if (func_or_var == FUNC_T) {
                    //in this instance we don't need to check the return type... nobody is expecting it!
                    if(call() == PARSE_ERR)
                        return PARSE_ERR;

                    // already shifted in call();

                    break;
                } else if (func_or_var == VAR_T || func_or_var == ARG_T) { 

                    // look for asignment
                    shift();

                    if(next.code != ARROW) {
                        error("code: expected assignment arrow, got %s",
                                next.text);
                        return PARSE_ERR;
                    }

                    int expected_ret = type,
                        parsed_ret;

                    // next is arrow, so we go ahead
                    shift();

                    if(next.code == STR) {
                        // reserve mem for string literal
                        unsigned int str_addr = qgen_str_lit(next.text);

                        if(type == STRING) {
                            qgen("\tS(0x%x) = 0x%x;",address,str_addr);

                            shift();
                            break;
                        } else {
                            error("code: trying to assign a string literal to a %i var/arg",
                                    type);
                            return PARSE_ERR;
                        }
                    }
                    if(next.code == SCAN) {
                        verbose("code: oh, a scan call!");

                        // TODO: qgen_scan(type,address);

                        shift();
                        break;
                    }
                    if((parsed_ret = nexp()) == PARSE_ERR)
                        return PARSE_ERR;
                    if(expected_ret != parsed_ret) {
                        error("code: trying to assign a %i value to %s, which is of type %i",
                                parsed_ret,reg->get_name(),expected_ret);
                        return PARSE_ERR;
                    }
                    // already shifted in nexp()

                    qgen_assign(type,func_or_var,address,table->get_scope());

                    break;
                }
            }
        case IF:
            // we reserve 3 labels:
            // one for the first code block, another for the 
            // second (or the end of such code block) and another
            // for the end of the second block, if there is any
            label1 = qgen_reserve_tag();
            label2 = qgen_reserve_tag();
            label3 = qgen_reserve_tag();

            // bexp writes the necessary operations
            shift();
            if(bexp(true) == PARSE_ERR) // check for the condition
                return PARSE_ERR;

            // after condition, jump to reserved label
            qgen_jmp(label1);
            // inmediately afterwards, jump to the end of that codeblock
            // this will only happen if the condition is not met
            qgen_jmp(label2);

            // alreade shifted in bexp()

            if(next.code != '{') {
                error("code: conditional statements must be surrounded by curly brackets");
                return PARSE_ERR;
            }

            // put the reserved label before first block
            qgen_write_reserved_tag(label1);

            shift();

            if(code() == PARSE_ERR)
                return PARSE_ERR;

            // already shifted in code()

            if(next.code == ELSE) {

                // before we start with the second block, we jump ahead:
                // this happens at the end of the first guarded block
                qgen_jmp(label3);

                // and now we pin point the second code block
                qgen_write_reserved_tag(label2);

                shift();

                if(next.code != '{') {
                    error("code: conditional statements must be surrounded by curly brackets");
                    return PARSE_ERR;
                }

                shift();

                if(code() == PARSE_ERR)
                    return PARSE_ERR;

                // already shifted in code()
            }

            // put the reserved label for the end of the first (or second,
            // in case of an else statement) block
            qgen_write_reserved_tag(label3);

            break; // next token already fetched
        case WHILE:
            // reserve three tags:
            // - one for before the condition check
            // - one for the beginning of the block
            // - one for the end of the block
            label1 = qgen_reserve_tag();
            label2 = qgen_reserve_tag();
            label3 = qgen_reserve_tag();

            shift();

            // before the condition computation
            qgen_write_reserved_tag(label1);
            if(bexp(true) == PARSE_ERR) // check for the condition
                return PARSE_ERR;
            // if condition met, jump to the block
            qgen_jmp(label2);
            // else, jump to the end
            qgen_jmp(label3);

            // already shifted in bexp()

            if(next.code != '{') {
                error("code: conditional statements must be surrounded by curly brackets");
                return PARSE_ERR;
            }

            // beginning of the block
            qgen_write_reserved_tag(label2);

            shift();

            if(code() == PARSE_ERR) // consumes the closing '}'
                return PARSE_ERR;

            // jump to the condition computation
            qgen_jmp(label1);
            // end of the block (previous jump is included in the block)
            qgen_write_reserved_tag(label3);

            // already shifted in code()

            break;
        case PRINT:

            verbose("code: print");
            shift();

            if(next.code != '(') {
                error("code: expected '(', got %s",next.text);
                return PARSE_ERR;
            }

            shift();

            char stringify[64];
            int aux_reg;
            label1 = qgen_reserve_tag();

            while(next.code != ')') {

                // TODO: push params

                if(next.code == ID) {
                    if(!find_in_table(next.text,reg,&func_or_var,&type,&address)) {
                        error("code: %s was not previously declared",
                                next.text);
                        return PARSE_ERR;
                    } else if (func_or_var == VAR_T || func_or_var == ARG_T) { 
                        if(type == STRING) {
                            // get value inside
                            qgen_get_vararg(STRING,func_or_var,address,table->get_scope());
                            aux_reg = result_reg(STRING);

                            qgen("\tR1 = R%d;",aux_reg);
                            free_reg(STRING,aux_reg);

                            goto print_jump;
                        } else if(type == INT || type == UINT) {
                            qgen_get_vararg(type,func_or_var,address,table->get_scope());
                            aux_reg = result_reg(type);

                            // put number in
                            qgen("\tR2 = R%d;",aux_reg);
                            free_reg(STRING,aux_reg);

                            // global format string
                            qgen("\tR1 = 0x%x;\t//int fmt string",int_fmt_str_addr);

                            goto print_jump;
                        } else if(type == CHAR) {
                            qgen_get_vararg(type,func_or_var,address,table->get_scope());
                            aux_reg = result_reg(type);

                            // put number in
                            qgen("\tR2 = R%d;",aux_reg);
                            free_reg(STRING,aux_reg);

                            // global format string
                            qgen("\tR1 = 0x%x;\t//char fmt string",char_fmt_str_addr);

                            goto print_jump;
                        }
                    }
                } else if(next.code == STR) {
                    address = qgen_str_lit(next.text);
                } else if(next.code == FLO_N) {
                    sprintf(stringify,"%f",next.lexed.fval);
                    address = qgen_str_stat(stringify);
                } else if(next.code == INT_N) {
                    sprintf(stringify,"%li",next.lexed.dval);
                    address = qgen_str_stat(stringify);
                } else if(next.code == TRUE) {
                    sprintf(stringify,"true");
                    address = qgen_str_stat(stringify);
                } else if(next.code == FALSE) {
                    sprintf(stringify,"false");
                    address = qgen_str_stat(stringify);
                }

                // dirección de la ristra
                qgen("\tR1 = 0x%x;",address);
                // etiqueta de retorno
print_jump:     qgen("\tR0 = %d;\t\t//return label",label1);
                // salto a impresión
                qgen("\tGT(putf_);");
                // siguiente instrucción
                qgen_write_reserved_tag(label1);

                shift();

                // TODO: pop params
            }

            shift(); // done with this statement
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

            if(return_type == VOID) {
                shift();

                if(next.code != '}') {
                    error("code: returning something in a void function!");
                    return PARSE_ERR;
                }

                // get rid of all the stack taken previously
                qgen("\tR7 = R6;");

                // pop numargs * 4 and release all that stack
                qgen_release_stack(4);
                qgen("\tR6 = I(R7);");
                qgen("\tR7 = R6 + 4;");

                // pop ret addr
                qgen("\tR6 = I(R7);");
                qgen("\tGT(R6);");

                return PARSE_OK;
            } else {
                shift();

                /* string literal */
                if(next.code == STR) {
                    if(return_type != STRING) {
                        error("code: return type is not string!");
                        return PARSE_ERR;
                    }
                    address = qgen_str_lit(next.text);
                    aux_reg = get_32_reg();
                    parsed_ret = STRING;
                    qgen("\tR%d = 0x%x;",aux_reg,address);
                } else {
                    if((parsed_ret = expression()) == PARSE_ERR) {
                        return PARSE_ERR;
                    } else if(parsed_ret != return_type) {
                        error("code: expression returns type %i, when function returns type %i",
                                parsed_ret,return_type);
                        return PARSE_ERR;
                    }
                    aux_reg = result_reg(parsed_ret);
                }
                /* expression */
                verbose("code: returning an expression!");

                // TODO: qgen - return something
                qgen_push_result(parsed_ret);

                if(next.code != '}') {
                    error("code: return statement must be at the end of block");
                    return PARSE_ERR;
                }

                shift();

                // get rid of all the stack taken previously
                qgen("\tR7 = R6;");

                // pop numargs * 4 and release all that stack
                qgen_release_stack(4);
                qgen("\tR6 = I(R7);");
                qgen("\tR7 = R6 + 4;");

                // pop ret addr
                qgen("\tR6 = I(R7);");
                qgen("\tGT(R6);");


                return PARSE_OK;
            }
        case EXIT:

            shift();

            if(next.code != '}') {
                error("code: exit statement must be at the end of block");
                return PARSE_ERR;
            }

            qgen("\tR0=0;\t// no return");
            qgen("\tGT(-2);\t// exit");

            return PARSE_OK;
        default: /* check for variable declarations, and if not, well, wtf */
            if(!is_type(next.code)) {
                error("code: unexpected token %s (code %i)",
                        next.text,next.code);
                return PARSE_ERR;
            }
            /* it's a type keyword, so we expect a declaration */
            if(declaration() == PARSE_ERR)
                return PARSE_ERR;

            // already shifted in declaration()
        }
    }

    verbose("code: block (presumably) correctly parsed");

    shift();

    return PARSE_OK;
}

/*
 * Previous token: several
 * Description: parses an expression and
 * computes return type
 * Eq. rule:
 *      *check the grammar file
 * Return: type
 */
int nexp() {
    //TODO: compute return type in the integers case (well shitttt)
    int func_or_var, type = 0, operand_type_1, operand_type_2, opertor;
    int reg1, reg2;
    unsigned int address;
    SymbolRegister *reg = NULL;

    verbose("nexp: starts with '%s'",next.text);

    switch(next.code) {
    case INT_N:
        // Rx = dval
        reg1 = get_32_reg();
        qgen_get_int_val(next.lexed.dval,reg1);

        shift();
        verbose("nexp: returning %i",INT);
        return INT;
    case FLO_N:
        // Rx = fval
        reg1 = get_64_reg();
        qgen_get_flo_val(yylval.fval,reg1);

        shift();
        verbose("nexp: returning %i",FLOAT);
        return FLOAT;
    case ID:
        if(!find_in_table(next.text,reg,&func_or_var,&type,&address)) { // not even in the table
            error("nexp: %s was not declared",
                    next.text);
            return PARSE_ERR;
        } else {
            char *name = strdup(next.text);
            if(func_or_var == VAR_T || func_or_var == ARG_T) {  // var/arg?

                qgen_get_vararg(type,func_or_var,address,table->get_scope());

                shift();

                verbose("nexp: returning %i",type);
                return type;
            } else { // found a function

                // check the call
                if(call() == PARSE_ERR) {
                    free(name);
                    return PARSE_ERR;
                }

                // already shifted in call()

                free(name);
                verbose("nexp: returning %i",type);
                return type;
            }
        }
    case '*':
    case '/':
    case '%':
    case '-':
    case '+':
        // NO look ahead for twin
        // just can't afford the added complexity

        // get previous operand
        opertor = next.code;
        shift();

        // parse two expressions
        if((operand_type_1 = nexp()) == PARSE_ERR) {
            error("nexp: unable to parse a first expression after '%c'",opertor);
            return PARSE_ERR;
        }
        reg1 = result_reg(operand_type_1);
        if((operand_type_2 = nexp()) == PARSE_ERR) {
            error("nexp: unable to parse a second expression after '%c'",opertor);
            return PARSE_ERR;
        }
        if(operand_type_1 != operand_type_2) {
            verbose("nexp: the types of the two operands are not the same!: %i and %i",
                    operand_type_1,operand_type_2);
        }
        reg2 = result_reg(operand_type_2);
        qgen_bi_op(opertor,operand_type_1,reg1,reg2);
        verbose("nexp: returning %i",operand_type_1);
        return operand_type_1;
    default:
        /* very ugly solution to use the default case, but  *
         * it allows for a more decent error message        */
        error("nexp: unexpected token %s; expected operator",next.text);
        return PARSE_ERR;
    }
    return PARSE_ERR;
}

/*
 * Previous token: several
 * Description: parses a logic expression
 * Eq. rule:
 *      *check the grammar file
 */
int bexp(bool first_call) {
    SymbolRegister *reg = NULL;
    int func_or_var, type, operand_type_1, operand_type_2, oper;
    int reg1, reg2;
    bool or_equal = false;

    switch(next.code) {
    case TRUE:
    case FALSE:
        // save either 1 or 0 to a reg
        reg1 = get_32_reg();
        qgen_get_int_val(next.lexed.bval,reg1);

        // if this is the first time, we know (because we parse
        // operators beforehand) that this is a lonely shin/nise
        if(first_call)
            qgen("\tIF(R%d)",result_reg(BOOL));

        shift();

        return BOOL;
    case ID:
        // it must be a call then: there are no
        // boolean variables in the language
        if(!find_in_table(next.text,reg,&func_or_var,&type)) { /* not even in the table */
            error("nexp: %s was not declared",
                    next.text);
            return PARSE_ERR;
        } else if(func_or_var == VAR_T || func_or_var == ARG_T) { /* var/arg? either way, no such thing is allowed */
            error("bexp: (this shouldn't have happened) vars or arguments of type boolean are not allowed");
            return PARSE_ERR;
        } else if(func_or_var == FUNC_T) { /* found a function */

            /* check the call */
            if(call() == PARSE_ERR)
                return PARSE_ERR;
        }

        // thorugh here means correct call parsing,
        // and that means that result must be in R0
        // TODO: revisit this thought
        if(first_call)
            qgen("\tIF(R%d)",result_reg(BOOL));

        // already shifted in call()

        return BOOL;

    case '!':

        shift();

        if(bexp(false) == PARSE_ERR) {
            error("bexp: unable to parse logical expression after '!'");
            return PARSE_ERR;
        }

        reg2 = result_reg(BOOL);
        reg1 = get_32_reg();
        qgen("\tR%d = ! R%d;",reg1,reg2);

        if(first_call)
            qgen("\tIF(R%d)",reg1);

        // already shifted in bexp()

        return BOOL;
    case '&':
    case '|':
        // parse 2 b expressions

        oper = next.code;
        shift();

        if(bexp(false) == PARSE_ERR) {
            error("bexp: unable to parse first operand of logical '%c'",oper);
            return PARSE_ERR;
        }
        reg1 = result_reg(BOOL);

        // already shifted in bexp()

        if(bexp(false) == PARSE_ERR) {
            error("bexp: unable to parse second operand of logical '%c'",oper);
            return PARSE_ERR;
        }
        reg2 = result_reg(BOOL);

        qgen("\tR%d = R%d %c%c R%d;",
                reg1,reg1,oper,oper,reg2); // let's be smart with the regs
        free_32_reg(reg2);

        if(first_call) { // meaning this & / | is the root operator
            qgen("\tIF(R%d)",reg1);
            free_32_reg(reg1);
        } 

        // already shifted in bexp()

        return BOOL;
    case '=':

        oper = next.code;
        shift();

        if((operand_type_1 = nexp()) == PARSE_ERR) {
            error("bexp: unable to parse first operand of comparison");
            return PARSE_ERR;
        }
        reg1 = result_reg(operand_type_1);

        // already shifted in nexp()

        if((operand_type_2 = nexp()) == PARSE_ERR) {
            error("bexp: unable to parse second operand of comparison");
            return PARSE_ERR;
        }
        reg2 = result_reg(operand_type_2);

        if(operand_type_1 != operand_type_2) {
            verbose("bexp: comparison operands do not match in type: %i and %i",
                    operand_type_1, operand_type_2);
        }
        if(operand_type_1 == STR || operand_type_2 == STR) {
            error("bexp: operations involving strings are not allowed");
            return PARSE_ERR;
        }

        qgen_log_comp_op(oper,operand_type_1,reg1,reg2);
        if(first_call) {
            qgen("\tIF(R%d)",reg1);
            free_32_reg(reg1);
        }

        //already shifted in nexp()

        return BOOL;
    case '>':
    case '<':
        // check for '='

        oper = next.code;
        shift();

        if(next.code == '=') {
            shift();
            or_equal = true;
        }

        if((operand_type_1 = nexp()) == PARSE_ERR) {
            error("bexp: unable to parse first operand of comparison");
            return PARSE_ERR;
        }
        reg1 = result_reg(operand_type_1);
        verbose("bexp: first expression of the comparison parsed \
                (type = %i)",operand_type_1);

        if((operand_type_2 = nexp()) == PARSE_ERR) {
            error("bexp: unable to parse second operand of comparison");
            return PARSE_ERR;
        }
        if(operand_type_1 != operand_type_2) {
            verbose("bexp: comparison operands do not match in type: %i and %i",
                    operand_type_1, operand_type_2);
        }
        if(operand_type_1 == STR || operand_type_2 == STR) {
            error("bexp: operations involving strings are not allowed");
            return PARSE_ERR;
        }
        reg2 = result_reg(operand_type_2);
        verbose("bexp: second expression of the comparison parsed\
                (type = %i)",operand_type_1);
        verbose("bexp: successfully parsed comparison operation");

        if(or_equal)
            qgen_log_comp_op((oper == '>')?';':':', // different encoding
                    operand_type_1,reg1,reg2);
        else
            qgen_log_comp_op(oper,operand_type_1,reg1,reg2);

        if(first_call) {
            if(operand_type_1 == FLOAT) {
                qgen("\tIF(RR%d)",reg1);
                free_64_reg(reg1);
            }
            else {
                qgen("\tIF(R%d)",reg1);
                free_32_reg(reg1);
            }
        }

        // already shifted in nexp()

        return BOOL;
    default:;
    }
    return PARSE_ERR;
}

/*
 * Previous token: {type}
 * Description: fairly trivial
 * Eq. rule:
 *      {type} ':' ID
 */
int argument() {

    int type = next.code;
    shift();

    if(next.code != ':') {
        error("argument: expected ':', got %s",next.text);
        return PARSE_ERR;
    }

    shift();

    if(next.code != ID) {
        error("argument: expected identifier, got %s",next.text);
        return PARSE_ERR;
    }
    char *name = strdup(next.text);

    try {
        table->store_symbol(ARG_T,type,local_num_args,name);
    } catch(const char* msg) {
        p_error(msg);
        return PARSE_ERR;
    }
    free(name);

    // no codegen here: just syntactic info

    shift();

    return PARSE_OK;
}

/*
 * Previous token: ID|tsutaeru
 * Description: checks for passed params and checks
 * each param's type against correspondant function
 * argument
 * Eq. rule:
 *      [ID] '(' parameter* ')'
 */
int call() {

    // This function does:
    // save the registers
    // push params to the stack
    // push return address
    // goto function label
    // label return address
    // pop result

    int numargs = 0;
    int *param_types, *pushed_32, *pushed_64;
    unsigned int label = qgen_reserve_tag();
    char *fname;

    SymbolRegister *arg;
    SymbolRegister *func = table->get_symbol(next.text);
    if(!func)
        return PARSE_ERR; // we should never get here, but better safe than sorry
    fname = strdup(next.text);
    numargs = func->get_info();
    param_types = (int*)malloc(sizeof(int)*numargs);
    arg = func;
    for(int i=0; i<numargs; i++) {
        arg = arg->next;
        verbose("call: %s expects %i parameter to be of type %i",
                fname,i,arg->get_return());
        param_types[i] = arg->get_return();
    }

    // save the registers
    pushed_32 = qgen_push_32_regs();
    pushed_64 = qgen_push_64_regs();

    // reserve 8 for result
    qgen_take_stack(8);

    // push context (R6)
    qgen_take_stack(4);
    qgen("\tP(R7) = R6;");

    // push ret addr
    qgen_take_stack(4);
    qgen("\tP(R7) = %d;",label);

    shift();

    if(next.code != '(') {
        error("argument: expected '(', got %s",next.text);
        return PARSE_ERR;
    }

    int param_count = 0, of_type;

    shift();

    while(next.code != ')') {
        if((of_type = parameter()) == PARSE_ERR)
            return PARSE_ERR;

        verbose("call: parameter %i is of type %i",
                param_count,of_type);
        if(param_types[param_count] != of_type) {
            error("call: parameter %i is of type %i when %s expects %i",
                    param_count,of_type,fname,param_types[param_count]);
            return PARSE_ERR;
        }
        param_count++;

        // already shifted in parameter()
    }

    if(numargs != param_count) {
        error("call: parsed %i parameters, when function %s expects %i",
                param_count,func->get_name(),numargs);
        return PARSE_ERR;
    }

    verbose("call: parsed %i parameters",param_count);

    // push num args
    qgen("\tI(R7) = %d;",numargs*4);

    // set new context
    qgen("\tR6 = R7;");

    // and finally jump
    qgen_jmp(func->get_name());

    // return label
    qgen_write_reserved_tag(label);

    // -------------------------------------
    // worked our way through: now undo it
    // -------------------------------------

    // pop context
    qgen_release_stack(4);
    qgen("\tR6 = I(R7);");

    // pop return
    qgen_release_stack(4);
    if(func->get_return() == FLOAT)
        qgen_pop_result(func->get_return(),pushed_64);
    else
        qgen_pop_result(func->get_return(),pushed_32);

    // pop regs
    // WARN: this overrides the return we just popped
    qgen_release_stack(8);
    qgen_pop_64_regs(pushed_64);
    qgen_pop_32_regs(pushed_32);

    // the loop ends with next being ')', so go ahead
    shift();

    free(fname);
    return PARSE_OK;
}

/*
 * Previous token: ID|first_of(nexp)
 * Description: very simple match, using the
 * same distinguish logic between b and n exp
 * Eq. rule:
 *      STR | nexp
 */
int parameter() {
    /* note: this block is actually significant to the 
     * language spec, since we are basically saying that
     * no operations can be performed on string literals,
     * which makes them pretty useless. but it keeps this
     * particular function extremely simple */
    if(next.code == STR) {
        unsigned int address = qgen_str_lit(next.text);
        qgen("\tR%d = 0x%x;\t//this is not going to be used",get_32_reg(),address);
        return STRING;
    } 

    SymbolRegister *reg = NULL;
    int func_or_var, type;
    unsigned int addr;


    /* note: this one is also such a block. it states that
     * a param can only be a variable (or argument) */

    // push params to the stack... but only identifiers
    if(next.code == ID) {
        if(!find_in_table(next.text,reg,&func_or_var,&type,&addr)) {
            error("parameter: %s was not previously declared",next.text);
            return PARSE_ERR;
        } else if(func_or_var == VAR_T || func_or_var == ARG_T) {
            // TODO: qgen - push param
            qgen_push_param(type,func_or_var,addr,table->get_scope());
            shift();
            return type;
        }
    }

    // any other thing
    return PARSE_ERR;
}

