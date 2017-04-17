

#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#define VAR_T 0
#define FUNC_T 1
#define PARAM_T 2

#include <stdio.h>
#include <string.h>


using namespace std;

class SymbolRegister {
    unsigned short level;
    int type;
    int ret;
    char *name;

public:
    SymbolRegister* next;

    SymbolRegister(int type, int ret, char* name);
    ~SymbolRegister();
    unsigned short get_level();
    int get_type();
    int get_return();
    char* get_name();

    void set_level(unsigned short level);
    void print();
};

class SymbolTable {
    unsigned int size;
    unsigned short ambit;
    SymbolRegister* head;

public:
    SymbolTable();
    ~SymbolTable();
    void store_symbol(int type, int ret, char* name);
    SymbolRegister *get_symbol(char* name);
    void push_ambit();
    void pop_ambit();
    void print();
};


#endif
