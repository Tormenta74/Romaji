
#include <stdio.h>

#include "symtable.h"

int main() {
    SymbolTable* table = new SymbolTable();
    char symbol_name[64];
    char* actual_name;
    sprintf(symbol_name,"var1");
    printf("Pushing symbol \"%s\" to the table...\n",symbol_name);
    try {
        table->store_symbol(VAR,0,symbol_name);
    } catch(const char* msg) {
        fprintf(stderr,"%s",msg);
        return 1;
    }
    printf("Attempting to retrieve \"%s\"...\n",symbol_name);
    actual_name = table->get_symbol(symbol_name)->get_name();
    printf("Got:\t%s\n",actual_name);

    sprintf(symbol_name,"var2");
    table->push_ambit();
    printf("Pushing symbol \"%s\" to the table (one level deeper)...\n",symbol_name);
    try {
        table->store_symbol(VAR,0,symbol_name);
    } catch(const char* msg) {
        fprintf(stderr,"%s",msg);
        return 1;
    }
    printf("Attempting to retrieve \"%s\"...\n",symbol_name);
    actual_name = table->get_symbol(symbol_name)->get_name();
    printf("Got:\t%s with level %i\n",actual_name,table->get_symbol(symbol_name)->get_level());

    // ----------
    delete table;
    return 0;
}
