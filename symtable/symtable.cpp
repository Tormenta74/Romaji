
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"

// -------------------------------
// Register

SymbolRegister::SymbolRegister(int _type, double _value, char* _name) {
    int l;
    if((l=strlen(_name)) < 2) {
        throw "symbol can't have zero-length name\n";
        return;
    }
    name = (char*)malloc(l*sizeof(char)+1);
    strcpy(name,_name);
    type = _type;
    value = _value;
}

SymbolRegister::~SymbolRegister() {
    free(name);
}

void SymbolRegister::set_level(unsigned short level) {
    this->level = level;
}

unsigned short SymbolRegister::get_level() {
    return this->level;
}

int SymbolRegister::get_type() {
    return this->type;
}

double SymbolRegister::get_value() {
    return this->value;
}

char* SymbolRegister::get_name() {
    return this->name;
}

// -------------------------------
// Table

SymbolTable::SymbolTable() {
    this->size = 0;
    this->ambit = 0;
    this->head = NULL;
}

SymbolTable::~SymbolTable() {
    SymbolRegister* follower = this->head;
    SymbolRegister* next = NULL;

    while(follower->next) {
        next = follower->next;
        delete follower;
        follower = next;
    }

    delete follower;
}

void SymbolTable::store_symbol(int type, double value, char* name) {
    SymbolRegister* second = this->head;
    SymbolRegister* new_reg;
    try {
        new_reg = new SymbolRegister(type,value,name);
    } catch(const char* msg) {
        throw msg;
    }
    this->head = new_reg;
    this->head->next = second;
    this->head->set_level(this->ambit);
}

SymbolRegister* SymbolTable::get_symbol(char* name) {
    SymbolRegister* follower = this->head;

    while(follower) {
        if(strcmp(follower->get_name(),name) == 0)
            return follower;
        follower = follower->next;
    }
    return NULL;
}

void SymbolTable::push_ambit() {
    this->ambit++;
}

void SymbolTable::pop_ambit() {
    this->ambit--;
}

