
#ifndef _DR_PARSER_H
#define _DR_PARSER_H

// tokens

#define INT         257
#define LONG        258
#define UINT        259
#define CHAR        260
#define STRING      261
#define FLOAT       262
#define DOUBLE      263
#define BOOL        264
#define VOID        264

#define WHILE   266
#define IF      267
#define ELSE    268
#define FUNC    269
#define RET     270
#define MAIN    271
#define EXIT    272
#define PRINT   273
#define SCAN    274

#define ID      275
#define INT_N   276
#define FLO_N   277
#define STR     278
#define TRUE    278
#define FALSE   278

#define ARROW   279

int program();
int declaration();
int definition();
int main();
int assignment();
int nexp();
int bexp();
int signature();
int main_sig();
int arguments();
int argument();
int code();
int call();
int parameters();
int parameter();

int p_error(char*);

#endif
