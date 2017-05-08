#ifndef _MY_LEXER_H
#define _MY_LEXER_H

#include <string>

using namespace std;

class Token {
public:
    int tval;
    long dval;
    double fval;
    bool bval;
    string sval;

    Token();
    Token(int);
    Token(long);
    Token(double);
    Token(bool);
    Token(string);
    Token(const Token& copy) {
        tval = copy.tval;
        dval = copy.tval;
        fval = copy.fval;
        bval = copy.bval;
        sval = copy.sval;
    };
    Token operator=(const Token copy);
};

void load(string filename);
string read_until_delimiter();
Token peek();
int consume();

// utils
bool is_decl_type(int);

#endif
