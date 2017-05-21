/*
 * Some notes:
 * in Romaji, we are going to treat arguments passed to functions
 * always as passed by reference, so assignments made inside functions
 * have effects in the rest of the program.
 */

#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "bison-bridge.h"


FILE *obj_file = NULL;
int current_tag = 1;
char *function_space[64]; // a somewhat random limit
int current_statcode = 0;
unsigned int stat_address = 0x11fff;


/*==============================================*/
/*= ** **        HELPER FUNCTIONS        ** ** =*/
/*==============================================*/

const char *type_access(int type) {
    switch(type) {
        case CHAR:
            return "U";
        case INT:
            return "I";
        case UINT:
            return "P";
        case FLOAT:
            return "D";
        case STRING:
            return "S";
        default:
            return NULL;
    }
}

const char *type_format(int type) {
    switch(type) {
        case CHAR:
            return "1c";
        case INT:
            return "d";
        case UINT:
            return "u";
        case FLOAT:
            return "f";
        case STRING:
            return "s"; // ohhhh shit, what I just realized
        default:
            return NULL;
    }
}

unsigned short type_length(int type) {
    switch(type) {
        case CHAR:
            return 8;
        case STRING: // size of address
            return 16;
        case INT:
        case UINT:
            return 32;
        case FLOAT:
            return 64;
        default:
            return 0;
    }
}


/*==============================================*/
/*= ** ** ** **   QGEN FUNCTIONS   ** ** ** ** =*/
/*==============================================*/


void init_q_file(char *filename) {
    FILE *fp = fopen(filename,"w");
    if(!fp)
        throw "Unable to open object file";

    /* initialize the object file */
    fprintf(fp,"#include \"Q.h\"\n");
    fprintf(fp,"#define STK R7\n");
    fprintf(fp,"#define END -2\n");
    fprintf(fp,"BEGIN\n");

    /* set the pointer to the global object file */
    obj_file = fp;
}

void quit_codegen() {
    fclose(obj_file);
    for(int i=0; i<63; i++)
        if(function_space[i])
            free(function_space[i]);
}

void codegen(char *qline) {
    fprintf(obj_file,qline);
}

// add tag to the object file to reference
// function code
void qgen_tag(char *fname) {
    function_space[current_tag-1] = strdup(fname);
    qgen("L %d:\t// kansu %s",current_tag++,fname);
}

void qgen_jmp(char *fname) {
    int i;
    for(i=0; i<63; i++) 
        if(strcmp(function_space[i],fname) == 0) {
            qgen("\tGT(%d);",i+1);
            return;
        }
    qgen("\t// codegen failed to find the label of the %s function",fname);
}

unsigned int qgen_str(char *string) {
    unsigned int ret = stat_address;

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,%s)",stat_address,string);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    stat_address -= strlen(string)+1;

    return ret;
}

unsigned int qgen_var(int type) {
    unsigned int ret = stat_address;

    qgen("STAT(%i)",current_statcode);
    qgen("\tDAT(0x%x,%s,0)",
            stat_address,
            type_access(type));
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    stat_address -= type_length(type);

    return ret;
}

void qgen_scan(int type, unsigned int addr) {

    qgen("\tscanf(\"%%%s\",&%s(0x%x));\t// scan functionality breaks Q compatibility",
            type_format(type),type_access(type),addr);

}

/*
 * argument fetching procedure:
 * when we use the arg in the function, we get the
 * address into one of the registers by accessing the
 * stack into the offset (previously calculated by
 * walking through the symbol table)
 */
