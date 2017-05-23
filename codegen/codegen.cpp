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
    //fprintf(fp,"#define STK R7\n");
    //fprintf(fp,"#define END -2\n");
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
int qgen_tag(char *fname) {
    function_space[current_tag-1] = strdup(fname);
    qgen("L %d:\t// kansu %s",current_tag,fname);
    return current_tag++;
}

// reserve a label for conditional guards
int qgen_reserve_tag() {
    function_space[current_tag-1] = strdup("shi");
    return current_tag++;
}

// writes a reserved label
void qgen_write_reserved_tag(int label) {
    qgen("L %d:\t// conditional guarded block",label);
}

// generate a goto instruction with the corresponding
// label
void qgen_jmp(char *fname) {
    int i;
    for(i=0; i<63; i++) 
        if(strcmp(function_space[i],fname) == 0) {
            qgen("\tGT(%d);",i+1);
            return;
        }
    qgen("\t// codegen failed to find the label of the %s function",fname);
}

// generate a goto instruction with the corresponding
// label
void qgen_jmp(int label) {
    if(label > 0) {
        qgen("\tGT(%d);",label);
    } else {
        qgen("\t// jumping to negative labels is not allowed");
    }
}

// pushes R1..R6 and RR0..RR3 to the top of the stack
void qgen_push_regs() {

}

// pushes R1..R6 and RR0..RR3 to the top of the stack
void qgen_pop_regs() {

}

// reserve memory

unsigned int qgen_str(char *string) {
    stat_address -= strlen(string)+1;

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,%s)",stat_address,string);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;

    return stat_address;
}

unsigned int qgen_var(int type) {
    stat_address -= type_length(type);

    qgen("STAT(%i)",current_statcode);
    if(type == FLOAT) {
        qgen("\tDAT(0x%x,D,0.0);",
                stat_address);
    } else {
        qgen("\tDAT(0x%x,%s,0);",
                stat_address,
                type_access(type));
    }
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;

    return stat_address;
}

unsigned int qgen_str_var(int size) {
    char *filler = (char*)malloc(sizeof(char)*size);

    for(int i=0; i<size; i++)
        strcat(filler," ");

    stat_address -= size;

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;

    free(filler);

    return stat_address;
}

unsigned int qgen_str_var(char *string) {
    char *filler = strndup(string+1,strlen(string)-2);

    stat_address -= strlen(filler) + 1;

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;

    free(filler);

    return stat_address;
}

unsigned int qgen_str_var(int size, char *string) {
    char *filler = (char*)malloc(sizeof(char)*size);

    strncpy(filler,string+1,strlen(string)-2);
    for(int i=0; i<(size-(int)strlen(string)); i++)
        strcat(filler," ");

    stat_address -= strlen(filler)+1;

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;

    free(filler);

    return stat_address;
}

unsigned int qgen_str_var_scan(int size) {
    unsigned int addr = qgen_str_var(size);
    qgen("\n\tscanf(\"%c%is\",&U(0x%x));\n",
            PERCENT_ASCII,size,addr);
    return addr;
}

void qgen_scan(int type, unsigned int addr) {

    char *format_string = strdup("%%");
    strcat(format_string,type_format(type));
    qgen("\tscanf(\"%s\",&%s(0x%x));\t// scan functionality breaks Q compatibility",
            format_string,type_access(type),addr);

}

//stack

void qgen_raise_stack() {
    qgen("\tR7 = R7 - 4;");
}
void qgen_lower_stack() {
    qgen("\tR7 = R7 - 4;");
}

//TODO
void qgen_push_par(int type, int offset, int reg) {
    qgen("\t");
}

// arithmetics

int regs32[7] = {0,0,0,0,0,0,0};
int regs64[4] = {0,0,0,0};

int last_fetched_32_reg = 0;
int last_fetched_64_reg = 0;

// gets the first free 32 register
int get_32_reg() {
    int ret = -1;
    for(int i=0; i<7; i++)
        if(regs32[i] == 0) {
            ret = last_fetched_32_reg = i;
            regs32[i] = 1;
            break;
        }
    return ret;
}

// gets the first free 64 register
int get_64_reg() {
    int ret = -1;
    for(int i=0; i<4; i++)
        if(regs64[i] == 0) {
            ret = last_fetched_64_reg = i;
            regs64[i] = 1;
            break;
        }
    return ret;
}

//
int result_reg(int type) {
    if(type == FLOAT)
        return last_fetched_64_reg;
    return last_fetched_32_reg;
}

// marks the registers as free
void free_32_reg(int reg) {
    if(reg < 0 && reg > 6)
        return;
    regs32[reg] = 0;
}
void free_64_reg(int reg) {
    if(reg < 0 && reg > 3)
        return;
    regs64[reg] = 0;
}
void free_reg(int type, int reg) {
    if(type == FLOAT)
        free_64_reg(reg);
    else
        free_32_reg(reg);
}

// store variable value
void qgen_get_var(int type, int reg, unsigned int addr) {
    if(type == FLOAT) {
        qgen("\tRR%d = D(0x%x);",
                reg,addr);
    } else {
        qgen("\tR%d = %s(0x%x);",
                reg,type_access(type),addr);
    }
}

void qgen_get_int_val(int val, int reg) {
    qgen("\tR%d = %d;",reg,val);
}
void qgen_get_flo_val(double val, int reg) {
    qgen("\tRR%d = %f;",reg,val);
}

void qgen_bi_op(int oper, int type, int reg1, int reg2) {
    if(type == FLOAT) {
        qgen("\tRR%d = RR%d %c RR%d;",
                get_64_reg(),reg1,oper,reg2);
    } else {
        qgen("\tR%d = R%d %c R%d;",
                get_32_reg(),reg1,oper,reg2);
    }
    free_reg(type,reg1);
    free_reg(type,reg2);
}

void qgen_log_op(int oper, int reg1, int reg2) {
}

void qgen_log_comp_op(int oper, int type, int reg1, int reg2) {
    char *oper_str;
    if(oper == '=') {
        oper_str= strdup("==");
    } else if(oper == ';') {
        oper_str= strdup(">=");
    } else if(oper == ':') {
        oper_str= strdup("<=");
    } else {
        oper_str = (char*)malloc(sizeof(char));
        sprintf(oper_str,"%c",oper);
    }

    if(type == FLOAT) {
        qgen("\tRR%d = RR%d %s RR%d;",
                reg1,reg1,oper_str,reg2);
        last_fetched_64_reg = reg1;
    } else {
        qgen("\tR%d = R%d %s R%d;",
                reg1,reg1,oper_str,reg2);
        last_fetched_32_reg = reg1;
    }
    free(oper_str);
    free_reg(type,reg2);
}

void qgen_un_op(int plus_or_minus, int type, unsigned int addr) {
    qgen("\t%s(0x%x) = %s(0x%x) %c 1;",
            type_access(type),addr,
            type_access(type),addr,plus_or_minus);
}

void qgen_assign(int type, unsigned int address) {
    if(type == FLOAT) {
        qgen("\tD(0x%x) = RR%d;",
                address,result_reg(type));
    } else {
        qgen("\t%s(0x%x) = R%d;",
                type_access(type),address,result_reg(type));
    }
    free_reg(type,result_reg(type));
}
