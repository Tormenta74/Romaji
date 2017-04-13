

#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#define VAR 0
#define FUNC 1
#define PARAM 2

using namespace std;

class SymbolRegister {
    unsigned short level;
    int type;
    double value;
    char *name;

public:
    SymbolRegister* next;

    SymbolRegister(int type, double value, char* name);
    ~SymbolRegister();
    unsigned short get_level();
    int get_type();
    double get_value();
    char* get_name();

    void set_level(unsigned short level);
};

class SymbolTable {
    unsigned int size;
    unsigned short ambit;
    SymbolRegister* head;

public:
    SymbolTable();
    ~SymbolTable();
    void store_symbol(int type, double value, char* name);
    SymbolRegister *get_symbol(char* name);
    void push_ambit();
    void pop_ambit();
};


#endif
