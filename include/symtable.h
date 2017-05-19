

#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#define VAR_T 0
#define FUNC_T 1
#define ARG_T 2

#include <stdio.h>
#include <string.h>


using namespace std;

class SymbolRegister {
    unsigned int level;
    int type;   // var | arg | func
    int ret;    // type
    int info;   // func: #args | var: none | arg: pos
    char *name;

public:
    SymbolRegister* next;

    SymbolRegister(int type, int ret, int info, char* name);
    ~SymbolRegister();
    unsigned int get_level();
    int get_type();
    int get_return();
    int get_info();
    char* get_name();

    void set_level(unsigned int level);
    void print();
};

class SymbolTable {
    unsigned int size;
    unsigned int scope;
    SymbolRegister* head;

public:
    SymbolTable();
    ~SymbolTable();
    void store_symbol(int type, int ret, int info, char* name);
    SymbolRegister *get_symbol(char* name, unsigned int scope=0);
    void set_scope(unsigned int scope);
    unsigned int get_scope();
    void print();
};


#endif
