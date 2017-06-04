
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
"seisu"         { return INT;    }
"nashi seisu"   { return UINT;   }
"baito"         { return CHAR;   }
"mojiretsu"     { return STRING; }
"furotingu"     { return FLOAT;  }
"shinri"        { return BOOL;   }
"kyo"           { return VOID;   }

 /** control **/
"naka"      {return WHILE;  }
"to"        {return IF;     }
"ta"        {return ELSE;   }

 /** structure **/
"kansu"     {return FUNC;   }
"kisu"      {return RET;    }
"omo"       {return MAIN;   }
"shi"       {return EXIT;   }

 /** I/O **/
"tsutaeru"  {return PRINT;    }
"uketoru"   {return SCAN;     }

 /** special values **/
"shin"      {yylval.bval = 1;return TRUE;}
"nise"      {yylval.bval = 0;return FALSE;}

 /** assignment **/
{arrow}     {return ARROW;  }
              
[-+*/%=&|!<>{}()\[\]:]   {return *yytext;}

{letter}{letterdigit}*  {
        yylval.sval = strdup(yytext);
        return ID;
    }
("-")?{digit}+  {
        yylval.dval = atoi(yytext);
        return INT_N;
    }
(("-")?{digit})?+("."){digit}+  {
        yylval.fval = atof(yytext);
        return FLO_N;
    }
\"(\\.|[^\\"])*\"   {
        yylval.sval = strdup(yytext);
        return STR;
    }

.   {l_error("unrecognized character", *yytext);}

<<EOF>>     {return 0;}

%%

void l_error(const char* msg, char c) {
    fprintf(stderr, "flex: %i: %c %s\n", line, c, msg);
}
