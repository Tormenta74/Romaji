
%{
#include "rjiparse.tab.h"

extern int line;
void l_error(char*,char);
%}

%option noyywrap

letter          [a-zA-z_?]
digit           [0-9]
letterdigit     {letter}|{digit}
hex             [0-9a-fA-F]
hexnum          ("0x"){hex}+
arrow           "<-"

%%

\n         |
^#.*       |
#[^\n]*\n  {++line;}
[ \t]+     {/* ignore whitespace */}

 /* keywords */

 /** type **/
"seisu"         {return INT;    }
"naga seisu"    {return LONG;   }
"nashi seisu"   {return UINT;   }
"baito"         {return CHAR;   }
"mojiretsu"     {return STRING; }
"furotingu"     {return FLOAT;  }
"daburu"        {return DOUBLE; }
"shinri"        {return BOOL;   }
"kyo"           {return VOID;   }

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
"shin"      {return TRUE;}
"nise"      {return FALSE;}

 /** assignment **/
{arrow}     {return ARROW;  }

[-+*/%=&|!<>{}():]   {return *yytext;}

{letter}{letterdigit}*          {return ID;     }
("-")?{digit}+                  {return INT_N;  }
{hexnum}                        {return INT_H;  }
(("-")?{digit})?+("."){digit}+  {return FLO_N;  }
\"(\\.|[^\\"])*\"               {return STR;    }

.   {l_error("unrecognized character", *yytext);}

<<EOF>>     {return 0;}

%%

void l_error(char* msg, char c) {
    fprintf(stderr, "flex: %i: %c %s\n", line, c, msg);
}
