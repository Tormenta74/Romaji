
#include <iostream>

#include "lexer.h"

int main(int argc, char *argv[]) {
    if(argc != 2)
        load("stdin");
    else
        load(argv[1]);

    string token,
           all_tokens;
    do {
        try {
            token = read_until_delimiter();
        } catch (const char* msg) {
            //cout << msg << endl;
            return 0;
        }
        //cout << "read token" << endl;
        all_tokens += token;
    } while(true);

    //cout << all_tokens << endl;

    return 0;
}

