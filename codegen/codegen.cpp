
#include <stdlib.h>
#include <string.h>
#include "codegen.h"


FILE *obj_file = NULL;
int current_tag = 0;
char *function_space[64]; // a somewhat random limit

void init_q_file(char *filename) {
    FILE *fp = fopen(filename,"w");
    if(!fp)
        throw "Unable to open object file";

    /* initialize the object file */
    fprintf(fp,"#include \"Q.h\"\n");
    fprintf(fp,"#define PILA R7\n");
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
    function_space[current_tag] = strdup(fname);
    qgen("L %d:\t",current_tag++); // leaves the line open
}

void qgen_jmp(char *fname) {
    int i;
    for(i=0; i<63; i++) 
        if(strcmp(function_space[i],fname) == 0) {
            qgen("\tGT(%d);",i);
            return;
        }
    qgen("\t// codegen failed to find the label of the %s function",fname);
}

void qgen_str(char *string) {
    // somewhere along these lines
    qgen("STAT(%i)",0);
    qgen("CODE(%i)",0);
}
