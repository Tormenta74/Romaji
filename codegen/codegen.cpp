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

#define PERCENT_ASCII   '\045'

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


// tags and gotos

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

// reserve memory

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
    qgen("\tDAT(0x%x,%s,0);",
            stat_address,
            type_access(type));
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    stat_address -= type_length(type);

    return ret;
}

unsigned int qgen_str_var(int size) {
    unsigned int ret = stat_address;
    char *filler = (char*)malloc(sizeof(char)*size);

    for(int i=0; i<size; i++)
        strcat(filler," ");

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    stat_address -= size;

    free(filler);

    return ret;
}

unsigned int qgen_str_var(char *string) {
    unsigned int ret = stat_address;
    char *filler = strndup(string+1,strlen(string)-2);

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    stat_address -= strlen(filler);

    free(filler);

    return ret;
}

unsigned int qgen_str_var(int size, char *string) {
    unsigned int ret = stat_address;
    char *filler = (char*)malloc(sizeof(char)*size);

    strncpy(filler,string+1,strlen(string)-2);
    for(int i=0; i<(size-(int)strlen(string)); i++)
        strcat(filler," ");

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    stat_address -= strlen(filler);

    free(filler);

    return ret;
}

unsigned int qgen_str_scan(int size) {
    unsigned int addr = qgen_str_var(size);
    qgen("\n\tscanf(\"%c%is\",&U(0x%x));\n",
            PERCENT_ASCII,size,addr);
    return addr;
}

void qgen_scan(int type, unsigned int addr) {

    qgen("\tscanf(\"%%%s\",&%s(0x%x));\t// scan functionality breaks Q compatibility",
            type_format(type),type_access(type),addr);

}

void qgen_unary_op(int plus_or_minus, int type, unsigned int addr) {
    qgen("\t%s(0x%x) %c= %s(0x%x)",
            type_access(type),addr,plus_or_minus,
            type_access(type),addr);
}

/*
 * argument fetching procedure:
 * when we use the arg in the function, we get the
 * address into one of the registers by accessing the
 * stack into the offset (previously calculated by
 * walking through the symbol table)
 */
