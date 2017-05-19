%{
    #include <stdio.h>
    #include <stdlib.h>

    #include "symtable.h"

    int yylex(void);

    SymbolTable* table = new SymbolTable();
    int line=1;
    char err_string[256];
    char verb_string[256];
    void verbose(char*);
    void store(char* name, int type, int ret);
    int verify_ret(char* name, int type);
    void yyerror(char* msg);
%}

%union {
    int tval;
    long dval;
    double fval;
    int bval;
    char *sval;
}

%token WHILE IF ELSE FUNC RET MAIN EXIT PRINT SCAN

%token INT UINT CHAR STRING FLOAT BOOL VOID

%token <sval> ID
%token <dval> INT_N
%token <fval> FLO_N
%token <sval> STR
%token <bval> TRUE
%token <bval> FALSE
%token ARROW

/* types MUST be defined in %union */

%left '-' '+' '*' '/' '%'
%left '=' '&' '|' '!' '<' '>'
%left '{' '}' ':'

%%

/*
 * a program may be comprised of any number of variable
 * declarations and function definitions and only one 
 * obligatory main function
 */
program     : declaration program   /* as many global variables */
            | definition program    /* as many function definitions */
            | main                  /* an obligatory main function */
;

/*
 * a variable declaration may include the initial assignment
 */
declaration : INT ID[i1]            { store($i1,VAR_T,INT);}
            | INT assignment[a1]    { store($<sval>a1,VAR_T,INT); }
            | UINT ID[i3]           { store($i3,VAR_T,UINT); }
            | UINT assignment[a3]   { store($<sval>a3,VAR_T,UINT); }
            | CHAR ID[i4]           { store($i4,VAR_T,CHAR); }
            | CHAR assignment[a4]   { store($<sval>a4,VAR_T,CHAR); }
            | STRING ID[i5]         { store($i5,VAR_T,STRING); }
            | STRING assignment[a5] { store($<sval>a5,VAR_T,STRING); }
            | FLOAT ID[i6]          { store($i6,VAR_T,FLOAT); }
            | FLOAT assignment[a6]  { store($<sval>a6,VAR_T,FLOAT); }
;

/*
 * an identifier can only get a numerical value or a string,
 * as boolean variables are not defined (it can take values
 * from the scan function)
 */
assignment  : ID ARROW nexp { $<sval>$=$1; /*verbose("a numeric assignment");*/ }
            | ID ARROW STR { $<sval>$=$1; /*verbose("a string assignment");*/ }
            | ID ARROW SCAN { $<sval>$=$1; /*verbose("a scan assignment");*/ }
;

/*
 * a numerical expression is the result of operations and/or
 * literal numbers, that can also be the return of functions
 */
nexp    : INT_N | FLO_N | ID | call
        | '-' nexp nexp
        | '+' nexp nexp
        | '*' nexp nexp
        | '/' nexp nexp
        | '%' nexp nexp
        | '+' '+' ID
        | '-' '-' ID
;

/*
 * a logic or boolean expression is the result of logical 
 * operations, literal truth values, function calls, or 
 * numerical comparisons
 */
bexp    : TRUE | FALSE | ID | call
        /* purely boolean */
        | '=' bexp bexp
        | '&' bexp bexp
        | '|' bexp bexp
        /* numeric comparisons */
        | '=' nexp nexp
        | '<' nexp nexp
        | '>' nexp nexp
        | '<' '=' nexp nexp
        | '>' '='nexp nexp
        /* negation */
        | '!' bexp
;

/*
 * a function definition is comprised by its "signature"
 * (the name, the return type and the arguments) and the
 * code that the function executes
 */
definition  : signature ARROW arguments '{' code '}';
signature   : FUNC ID ':' INT       { store($2,FUNC_T,INT); }
            | FUNC ID ':' UINT      { store($2,FUNC_T,UINT); }
            | FUNC ID ':' CHAR      { store($2,FUNC_T,CHAR); }
            | FUNC ID ':' STRING    { store($2,FUNC_T,STRING); }
            | FUNC ID ':' FLOAT     { store($2,FUNC_T,FLOAT); }
            | FUNC ID ':' BOOL      { store($2,FUNC_T,BOOL); }
            | FUNC ID ':' VOID      { store($2,FUNC_T,VOID); }
;

/*
 * the main function definition is almost the same as a
 * general function's, only the kansu keyword and the
 * identifier are substituted by the omo keyword
 */
main        : main_sig ARROW arguments '{' code '}'
            ;
main_sig    : MAIN ':' INT
            | MAIN ':' UINT
            | MAIN ':' CHAR
            | MAIN ':' STRING
            | MAIN ':' FLOAT
            | MAIN ':' BOOL
            | MAIN ':' VOID
;

/*
 * arguments are defined as type:id and can be as many
 * as the programmer wants. shinri and kyo are not allowed
 * as variables of these types are not defined
 */
arguments   : /* no arguments */
            | argument arguments
;
argument    : INT ':' ID[i1]    { store($i1,PARAM_T,INT); }
            | UINT ':' ID[i3]   { store($i3,PARAM_T,UINT); }
            | CHAR ':' ID[i4]   { store($i4,PARAM_T,CHAR); }
            | STRING ':' ID[i5] { store($i5,PARAM_T,STRING); }
            | FLOAT ':' ID[i6]  { store($i6,PARAM_T,FLOAT); }
;

code    : declaration code
        | assignment code
        | nexp code /* generally not useful, but increment and decrement operators take advantage of it */
        /* a block of code inside a loop */
        | WHILE bexp {table->push_ambit();} '{' code '}' {table->pop_ambit();} code
            { /*verbose("while structure");*/ }
        /* a block of code inside a one condition guard */
        | IF bexp '{' code '}' ELSE '{' code '}' code
            { /*verbose("if/else structure");*/ }
        /* a block of code inside a one condition guard */
        | IF bexp  '{' code '}' code
            { /*verbose("if structure");*/ }
        /* define the oneliners code blocks here */

/* function calls on their own */
        | call code
            { /*verbose("function call");*/ }
        | PRINT '(' parameters ')' code
            { /*verbose("print structure");*/ }
        | RET parameter code
            { /*verbose("return (with value)");*/ }
        | RET code
            { /*verbose("return (no value)");*/ }
        | EXIT 
            { /*verbose("exit");*/ }
        |
;
call        : ID '(' parameters ')' {
                $<tval>$ = verify_ret($1,FUNC_T);
            } ;
            parameters   : /* no parameters */ 
            | parameter parameters
;
parameter   : nexp
            | bexp
            | STR
;


%%

int main(int argc, char *argv[]) {
    extern FILE *yyin;
    yyin = fopen(argv[1], "r");
    yyparse();
    fprintf(stdout,"\nSymbol table at the end of the parsing:\n");
    table->print();
    delete table;
}

void store(char* name, int type, int ret) {
    if(table->get_symbol(name)) {
        sprintf(err_string,"symbol \"%s\" was already in the symbol table",name);
        yyerror(err_string);
        sprintf(err_string,"");
    }
    else {
        if(type == VAR_T)
            sprintf(verb_string,"recognized new variable \"%s\"",name);
        else if(type == FUNC_T)
            sprintf(verb_string,"recognized new function \"%s\"",name);
        else 
            sprintf(verb_string,"recognized new argument \"%s\"",name);
        table->store_symbol(type,ret,name);
        sprintf(verb_string,"%s; type/return %i",verb_string,table->get_symbol(name)->get_return());
        verbose(verb_string);
        sprintf(verb_string,"");
    }
}

int verify_ret(char* name, int type) {
    SymbolRegister* r = table->get_symbol(name);
    if(!r) {
        sprintf(err_string,"\"%s\" was not found in the symbol table",name);
        yyerror(err_string);
        sprintf(err_string,"");
        return -1;
    }
    else if(r->get_type() != type) {
        sprintf(err_string,"\"%s\" is not declared as ",name);
        sprintf(err_string,"%s%s",err_string,(type == FUNC_T) ? "function" : (type == VAR_T) ? "variable" : "argument");
        yyerror(err_string);
        sprintf(err_string,"");
        return -1;
    }
    /* get the return for the function */
    return r->get_return();
}

void verbose(char* msg) {
    fprintf(stdout, "%i: %s\n", line, msg);
}

void yyerror(char* msg) {
    fprintf(stderr, "%i: %s\n", line, msg);
}
