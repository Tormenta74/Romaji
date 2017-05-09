/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_BUILD_RJIPARSE_TAB_H_INCLUDED
# define YY_YY_BUILD_RJIPARSE_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    WHILE = 258,
    IF = 259,
    ELSE = 260,
    FUNC = 261,
    RET = 262,
    MAIN = 263,
    EXIT = 264,
    PRINT = 265,
    SCAN = 266,
    INT = 267,
    LONG = 268,
    UINT = 269,
    CHAR = 270,
    STRING = 271,
    FLOAT = 272,
    DOUBLE = 273,
    BOOL = 274,
    VOID = 275,
    ID = 276,
    INT_N = 277,
    FLO_N = 278,
    STR = 279,
    TRUE = 280,
    FALSE = 281,
    ARROW = 282
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 22 "bison/rjiparse.y" /* yacc.c:1909  */

    int tval;
    long dval;
    double fval;
    int bval;
    char *sval;

#line 90 "build/rjiparse.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

/* my modification: */
int yylex (void);

#endif /* !YY_YY_BUILD_RJIPARSE_TAB_H_INCLUDED  */
