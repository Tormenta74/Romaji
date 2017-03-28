%{
    #include <stdio.h>
    #include <stdlib.h>

    int line=1;
    void verbose(char*);
%}

%union {
    int dval;
    float fval;
    int bval;
    char *sval;
}

%token INT LONG UINT CHAR STRING FLOAT DOUBLE BOOL VOID
%token WHILE IF ELSE FUNC RET MAIN EXIT PRINT SCAN PRINT_END
%token ID INT_N INT_H FLO_N STR TRUE FALSE
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
declaration : INT ID | INT assignment
            | LONG ID | LONG assignment
            | UINT ID | UINT assignment
            | CHAR ID | CHAR assignment
            | STRING ID | STRING assignment
            | FLOAT ID | FLOAT assignment
            | DOUBLE ID | DOUBLE assignment
;

/*
 * an identifier can only get a numerical value or a string,
 * as boolean variables are not defined (it can take values
 * from the scan function)
 */
assignment  : ID ARROW nexp { verbose("a numeric assignment"); }
            | ID ARROW STR { verbose("a string assignment"); }
            | ID ARROW SCAN { verbose("a scan assignment"); }
;

/*
 * a numerical expression is the result of operations and/or
 * literal numbers, that can also be the return of functions
 */
nexp    : INT_N | INT_H | FLO_N | ID | call
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
definition  : signature ARROW arguments '{' code '}' ;
signature   : FUNC ID ':' INT
            | FUNC ID ':' LONG
            | FUNC ID ':' UINT
            | FUNC ID ':' CHAR
            | FUNC ID ':' STRING
            | FUNC ID ':' FLOAT
            | FUNC ID ':' DOUBLE
            | FUNC ID ':' BOOL
            | FUNC ID ':' VOID
;

/*
 * the main function definition is almost the same as a
 * general function's, only the kansu keyword and the
 * identifier are substituted by the omo keyword
 */
main        : main_sig ARROW arguments '{' code '}'
;
main_sig    : MAIN ':' INT
            | MAIN ':' LONG
            | MAIN ':' UINT
            | MAIN ':' CHAR
            | MAIN ':' STRING
            | MAIN ':' FLOAT
            | MAIN ':' DOUBLE
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
argument    : INT ':' ID
            | LONG ':' ID 
            | UINT ':' ID 
            | CHAR ':' ID 
            | STRING ':' ID 
            | FLOAT ':' ID 
            | DOUBLE ':' ID 
;

code    : declaration code
        | assignment code
        | nexp code /* generally not useful, but increment and decrement operators take advantage of it */
        /* a block of code inside a loop */
        | WHILE bexp '{' code '}' code
            { verbose("while structure"); }
        /* a block of code inside a one condition guard */
        | IF bexp '{' code '}' ELSE '{' code '}' code
            { verbose("if/else structure"); }
        /* a block of code inside a one condition guard */
        | IF bexp '{' code '}' code
            { verbose("if structure"); }
        /* define the oneliners code blocks here */
        
        /* function calls on their own */
        | call code
        | PRINT '(' parameters ')' code
        | RET parameter code
        | RET code
        | EXIT code
        |
;
call        : ID '(' parameters ')' ;
parameters  : /* no parameters */ 
            | parameter parameters
;
parameter   : ID
            | nexp
            | bexp
            | STR
;


%%

int main(int argc, char *argv[]) {
    extern FILE *yyin;
    yyin = fopen(argv[1], "r");
    yyparse();
}

void verbose(char* msg) {
    fprintf(stdout, "%i: %s\n", line, msg);
}

void yyerror(char* msg) {
    fprintf(stderr, "%i: %s\n", line, msg);
}
