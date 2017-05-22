
%{
    #ifdef _USING_BISON_PARSER
    #include "rjiparse.tab.h"
    #else
    #include "bison-bridge.h"
    #endif

    #include <stdlib.h>
    #include <string.h>

    YYSTYPE yylval;

    extern int line;
    void l_error(const char *msg, char c);
%}

%option noyywrap

letter          [a-zA-Z_?]
digit           [0-9]
letterdigit     {letter}|{digit}
arrow           "<-"

%%

\n         |
^#.*       |
#[^\n]*\n  {++line;}
[ \t]+     {/* ignore whitespace */}

 /* keywords */

 /** type **/
"seisu"         { l_error(yytext,' ');return INT;    }
"nashi seisu"   { l_error(yytext,' ');return UINT;   }
"baito"         { l_error(yytext,' ');return CHAR;   }
"mojiretsu"     { l_error(yytext,' ');return STRING; }
"furotingu"     { l_error(yytext,' ');return FLOAT;  }
"shinri"        { l_error(yytext,' ');return BOOL;   }
"kyo"           { l_error(yytext,' ');return VOID;   }

 /** control **/
"naka"      {l_error(yytext,' ');return WHILE;  }
"to"        {l_error(yytext,' ');return IF;     }
"ta"        {l_error(yytext,' ');return ELSE;   }

 /** structure **/
"kansu"     {l_error(yytext,' ');return FUNC;   }
"kisu"      {l_error(yytext,' ');return RET;    }
"omo"       {l_error(yytext,' ');return MAIN;   }
"shi"       {l_error(yytext,' ');return EXIT;   }

 /** I/O **/
"tsutaeru"  {l_error(yytext,' ');return PRINT;    }
"uketoru"   {l_error(yytext,' ');return SCAN;     }

 /** special values **/
"shin"      {yylval.bval = 0;l_error(yytext,' ');return TRUE;}
"nise"      {yylval.bval = 1;l_error(yytext,' ');return FALSE;}

 /** assignment **/
{arrow}     {l_error(yytext,' ');return ARROW;  }
              
[-+*/%=&|!<>{}()\[\]:]   {l_error(yytext,' ');return *yytext;}

{letter}{letterdigit}*  {
        yylval.sval = strdup(yytext);
        l_error(yytext,' ');
        return ID;}
("-")?{digit}+  {
        yylval.dval = atoi(yytext);
        l_error(yytext,' ');
        return INT_N;}
(("-")?{digit})?+("."){digit}+  {
        yylval.fval = atof(yytext);
        l_error(yytext,' ');
        return FLO_N;}
\"(\\.|[^\\"])*\"   {
        yylval.sval = strdup(yytext);
        l_error(yytext,' ');
        return STR;}

.   {l_error("unrecognized character", *yytext);}

<<EOF>>     {return 0;}

%%

void l_error(const char* msg, char c) {
    fprintf(stderr, "flex: %i: %c %s\n", line, c, msg);
}
