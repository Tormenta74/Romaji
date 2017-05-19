
#include <fstream>
#include "lexer.h"


ifstream source;

Token::Token(int type) {
    this->tval = type;
}

Token::Token(long value) {
    this->dval = value;
}

Token::Token(double value) {
    this->fval = value;
}

Token::Token(bool value) {
    this->bval = value;
}

Token::Token(string value) {
    this->sval = value;
}

// the good shit

// throws on fail
void load(string filename) {
    if(filename == "stdin")
        source.open(filename,ios::in);
    else
        source.open(filename,ios::in);
    if(!source.is_open())
        throw "error opening the file!";
}

void close() {
    source.close();
}

void read_until_return() {

}

string read_until_delimiter() {
    string token;
    char c;
    // get to the first non space/tab/carriage return
    
    while((c = source.get()) == ' '
            || c == '\t'
            || c == '\n')
        if (c == EOF)
            throw "end of file!";

    // c is no longer one such character, so we start appending
    do {
        if(c == EOF) {
            token += '\0';
            break;
        }
        token += c;
        c = source.get();
    } while(c != ' '
            && c != '\t'
            && c != '\n');

    return token;
}

// source.tellg()
// source.read_until(' ','\t','\n')
// source.seekg(previous)
/*Token peek() {

    streampos previous = source.tellg();
    string token = read_until_delimiter();
    source.seekg(previous);

    return Token();
}*/
