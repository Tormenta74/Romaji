%{
    #include <stdio.h>
    #include <stdlib.h>

    #include "symtable.h"
    #include "format.h"

    int yylex(void);

    int line=1;
    void verbose(char*);
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
declaration : INT ID
            | INT assignment
            | UINT ID
            | UINT assignment
            | CHAR ID
            | CHAR assignment
            | STRING ID
            | STRING assignment
            | FLOAT ID
            | FLOAT assignment
;

/*
 * an identifier can only get a numerical value or a string,
 * as boolean variables are not defined (it can take values
 * from the scan function)
 */
assignment  : ID ARROW nexp
            | ID ARROW STR
            | ID ARROW SCAN
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
;

/*
 * an increment does not count as an arithmetic expression,
 * but is rather a statement to be included in the code
 * (same with decrement)
 */
increment:  '+' '+' ID ;
decrement:  '-' '-' ID ;

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
signature   : FUNC ID ':' INT
            | FUNC ID ':' UINT
            | FUNC ID ':' CHAR
            | FUNC ID ':' STRING
            | FUNC ID ':' FLOAT
            | FUNC ID ':' BOOL
            | FUNC ID ':' VOID
;

/*
 * the main function definition is almost the same as a
 * general function's, only the kansu keyword and the
 * identifier are substituted by the omo keyword
 * edit: changed the grammar and simplified the rules
 */
main        : MAIN ':' INT '{' code '}'
            | MAIN ':' VOID '{' code '}'
;

/*
 * arguments are defined as type:id and can be as many
 * as the programmer wants. shinri and kyo are not allowed
 * as variables of these types are not defined
 */
arguments   : /* no arguments */
            | argument arguments
;
argument    : INT ':' ID
            | UINT ':' ID
            | CHAR ':' ID
            | STRING ':' ID
            | FLOAT ':' ID
;

code    : declaration code
        | assignment code
        | increment code
        | decrement code
        | WHILE bexp '{' code '}' code
        | IF bexp '{' code '}' ELSE '{' code '}' code
        | IF bexp  '{' code '}' code
        | call code
        | PRINT '(' parameters ')' code
        | RET parameter code
        | RET code
        | EXIT 
        |
;
call    : ID '(' parameters ')';
parameters  : /* no parameters */ 
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
}

void verbose(char* msg) {
    fprintf(stdout, "bison%i: " BLUE "%s" CLEAR_FORMAT "\n", line, msg);
}

void yyerror(char* msg) {
    fprintf(stdout, "bison%i: syntax error " RED "%s" CLEAR_FORMAT "\n", line, msg);
}
