
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"

char* names(int i) {
    char* ret;
    switch(i) {
        case 267:
            ret = strdup("seisu");
            break;
        case 268:
            ret = strdup("naga seisu");
            break;
        case 269:
            ret = strdup("nashi seisu");
            break;
        case 270:
            ret = strdup("baito");
            break;
        case 271:
            ret = strdup("mojiretsu");
            break;
        case 272:
            ret = strdup("furotingu");
            break;
        case 273:
            ret = strdup("daburu");
            break;
        case 274:
            ret = strdup("shinri");
            break;
        case 275:
            ret = strdup("kyo");
            break;
        default:
            ret = strdup("null");
    }
    return ret;
}

// -------------------------------
// Register

SymbolRegister::SymbolRegister(int _type, int _ret, int _info, char* _name) {
    int l;
    if((l=strlen(_name)) < 1) {
        throw "symbol can't have zero-length name\n";
        return;
    }
    name = (char*)malloc(l*sizeof(char)+1);
    strcpy(name,_name);
    this->type = _type;
    this->ret = _ret;
    this->info = _info;
}

SymbolRegister::~SymbolRegister() {
    free(name);
}

void SymbolRegister::set_level(unsigned int level) {
    this->level = level;
}

unsigned int SymbolRegister::get_level() {
    return this->level;
}

int SymbolRegister::get_type() {
    return this->type;
}

int SymbolRegister::get_return() {
    return this->ret;
}

int SymbolRegister::get_info() {
    return this->info;
}

char* SymbolRegister::get_name() {
    return this->name;
}

void SymbolRegister::print() {
    char *type = names(this->ret);
    fprintf(stdout,"%s %s:%s (%i) - scope %i\n",
            (this->type == VAR_T)?"variable":(this->type == FUNC_T)?"function":"argument",
            this->name,
            type,
            this->info,
            this->level);
    free(type);
}

// -------------------------------
// Table

SymbolTable::SymbolTable() {
    this->size = 0;
    this->scope = 0;
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

void SymbolTable::store_symbol(int type, int ret, int info, char* name) {
    if(this->get_symbol(name,this->scope)
            || this->get_symbol(name,0)) {
        throw "symbol already declared";
    }
    SymbolRegister* second = this->head;
    SymbolRegister* new_reg;
    try {
        new_reg = new SymbolRegister(type,ret,info,name);
    } catch(const char* msg) {
        fprintf(stderr,"%s",msg);
    }
    this->head = new_reg;
    this->head->next = second;
    this->head->set_level(this->scope);
    this->size++;
}

SymbolRegister* SymbolTable::get_symbol(char* name, unsigned int scope) {
    SymbolRegister* follower = this->head;

    while(follower) {
        if(strcmp(follower->get_name(),name) == 0)
            if(follower->get_level() == scope)
                return follower;
        follower = follower->next;
    }
    return NULL;
}

void SymbolTable::set_scope(unsigned int scope) {
    this->scope = scope;
}

unsigned int SymbolTable::get_scope() {
    return this->scope;
}

void SymbolTable::print() {
    SymbolRegister* follower = this->head;
    while(follower) {
        for(unsigned int i=0;i<follower->get_level();i++)
            fprintf(stdout,"\t");
        follower->print();
        follower = follower->next;
    }
}

