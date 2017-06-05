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
#include "symtable.h"

#define PERCENT_ASCII   '\045'
#define BASE            R6

FILE *obj_file = NULL;
int current_tag = 1;
char *function_space[64]; // a somewhat random limit
int current_statcode = 0;

unsigned int stat_address = 0x12000;

int local_var_offset = 0;

// 1: marked as used
int regs32[6] = {0,0,0,0,0,0}; 
int regs64[4] = {0,0,0,0};
int regs32_pushed[6] = {0};
int regs64_pushed[4] = {0};

int last_fetched_32_reg = 0;
int last_fetched_64_reg = 0;

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
        return "P";
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
        return 1;
    case STRING: // size of address
        return 2;
    case INT:
    case UINT:
        return 4;
    case FLOAT:
        return 8;
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
    qgen("L %d:\t",label);
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


/** ------------------------------------- **/
/**            MEMORY RESERVE             **/
/** ------------------------------------- **/
void reset_local_vars() {
    local_var_offset = 0;
}

unsigned int qgen_str_lit(char *string) {
    char *filler = strndup(string,strlen(string));
    stat_address -= strlen(filler)+1;

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,%s);",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    free(filler);

    return stat_address;
}

unsigned int qgen_str_stat(char *string) {
    char *filler = strndup(string,strlen(string));
    stat_address -= strlen(filler)+1;

    qgen("STAT(%i)",current_statcode);
    qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
    qgen("CODE(%i)",current_statcode);

    current_statcode ++;
    free(filler);

    return stat_address;
}

unsigned int qgen_declare_var(int type, int scope) {
    if(scope == 0) {
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
    } else {
        qgen_take_stack(type_length(type));
        local_var_offset += type_length(type);
        return local_var_offset;
    }
}

unsigned int qgen_declare_str_var(int size, int scope) {
    char *filler;
    unsigned int addr;

    if(scope == 0) {
        filler = (char*)malloc(sizeof(char)*size);
        for(int i=0; i<size; i++)
            strcat(filler," ");

        stat_address -= size;

        qgen("STAT(%i)",current_statcode);
        qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
        qgen("CODE(%i)",current_statcode);

        current_statcode ++;
        addr = stat_address;
    } else {
        qgen_take_stack(size);
        local_var_offset += size;
        addr = local_var_offset;
    }

    free(filler);

    return addr;
}

unsigned int qgen_str_var(char *string, int scope) {
    char *filler = strndup(string+1,strlen(string)-2);
    unsigned int addr;

    if(scope == 0) {
        stat_address -= strlen(filler) + 1;

        qgen("STAT(%i)",current_statcode);
        qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
        qgen("CODE(%i)",current_statcode);

        current_statcode ++;
        addr = stat_address;
    } else {
        qgen_take_stack(strlen(filler) + 1);
        local_var_offset += strlen(filler) + 1;
        addr = local_var_offset;
    }

    free(filler);

    return addr;
}

unsigned int qgen_str_var(int size, char *string, int scope) {
    char *filler = (char*)malloc(sizeof(char)*size);
    unsigned int addr;

    strncpy(filler,string+1,strlen(string)-2);
    for(int i=0; i<(size-(int)strlen(string)); i++)
        strcat(filler," ");

    if(scope == 0) {
        stat_address -= size;

        qgen("STAT(%i)",current_statcode);
        qgen("\tSTR(0x%x,\"%s\");",stat_address,filler);
        qgen("CODE(%i)",current_statcode);

        current_statcode ++;
        addr = stat_address;
    } else {
        qgen_take_stack(size);
        local_var_offset += size;
        addr = local_var_offset;
    }

    free(filler);

    return addr;
}

unsigned int qgen_declare_str_var_scan(int size, int scope) {
    unsigned int addr = qgen_declare_str_var(size,scope);
    qgen("\n\tscanf(\"%c%is\",&U(0x%x));\n",
            PERCENT_ASCII,size,addr);
    return addr;
}

// i/o

void qgen_scan(int type, unsigned int addr) {

    char *format_string = strdup("%%");
    strcat(format_string,type_format(type));
    qgen("\tscanf(\"%s\",&%s(0x%x));\t// scan functionality breaks Q compatibility",
            format_string,type_access(type),addr);

}

//stack

void qgen_release_stack(int n) {
    qgen("\tR7 = R7 + %d;",n);
}
void qgen_take_stack(int n) {
    qgen("\tR7 = R7 - %d;",n);
}
void qgen_push_context() {
    qgen_take_stack(4);
    qgen("\tI(R7) = R6;");
}
void qgen_push_ret_addr(int label) {
    qgen_take_stack(4);
    qgen("\tI(R7) = %d;",label);
}
// offset is number of args * 4
void qgen_set_context() {
    qgen("\tR6 = R7;"); // position R6
}

// pushes R1..R6 to the top of the stack
int *qgen_push_32_regs() {
    int *pushed = (int*)malloc(4*sizeof(int));
    for(int j=0; j<6; j++) {
        pushed[j] = 0; // initialize to zero, just in case
        if(regs64[j] == 1) {
            pushed[j] = 1; // mark as pushed
            regs32[j] = 0; // mark as free
            qgen_release_stack(8);
            qgen("\tD(R7) = RR%d;", j); // 8 bytes
        }
    }
    return pushed;
}

// pushes RR0..RR3 to the top of the stack
int *qgen_push_64_regs() {
    int *pushed = (int*)malloc(4*sizeof(int));
    for(int j=0; j<3; j++) {
        pushed[j] = 0; // initialize to zero, just in case
        if(regs64[j] == 1) {
            pushed[j] = 1; // mark as pushed
            regs64[j] = 0; // mark as free
            qgen_release_stack(8);
            qgen("\tD(R7) = RR%d;", j); // 8 bytes
        }
    }
    return pushed;
}

void qgen_pop_32_regs(int *pushed) {
    if(!pushed) return;
    for(int i=0; i<6; i++)
        if(pushed[i] == 1) {
            qgen_take_stack(4);
            qgen("\tR%d = P(R7);",i);
            regs32[i] = 1;
        }
    free(pushed);
}

void qgen_pop_64_regs(int *pushed) {
    if(!pushed) return;
    for(int i=0; i<4; i++)
        if(pushed[i] == 1) {
            qgen_take_stack(4);
            qgen("\tR%d = P(R7);",i);
            regs64[i] = 1;
        }
    free(pushed);
}

void qgen_push_result(int type) {
    qgen("\tR0 = I(R6);"); // get numargs
    qgen("\tR0 = R0 + 12;"); // up fields: numargs, ret addr, ret contx
    if(type == VOID)
        return;
    if(type == FLOAT) {
        qgen("\tD(R6+R0) = RR%d;",result_reg(type));
    } else {
        qgen("\t%s(R6+R0) = R%d;",
                type_access(type),result_reg(type));
    }
}

void qgen_pop_result(int type, int *unavailable) {
    int reg_save = -1;
    if(type == VOID)
        return;
    if(type == FLOAT) {
        for(int i=0; i<4; i++)
            if(unavailable[i] == 0) {
                reg_save = i;
                regs64[i] = 1; // mark as used
                last_fetched_64_reg = i; // signal return is here
                break;
            }
        qgen("\tRR%d = D(R7);",reg_save);
    } else {
        for(int i=0; i<6; i++)
            if(unavailable[i] == 0) {
                reg_save = i;
                regs32[i] = 1; // mark as used
                last_fetched_32_reg = i; // signal return is here
                break;
            }
        qgen("\tR%d = %s(R7);",
                reg_save,type_access(type));
    }
}

void qgen_push_param(int type, int arg_or_var, unsigned int addr, int scope) {
    int reg_addr;
    qgen_take_stack(4);
    if(scope == 0) {
        qgen("\tP(R7) = 0x%x;",addr);           // push global address (easy)
    } else {
        if(arg_or_var == VAR_T) {
            qgen("\tP(R7) = R6 - %d;",addr);    // address is relative to current
                                                // context
        } else {
            reg_addr = get_32_reg();
            qgen("\tR%d = P(R6+%d);",           // fetch mem address
                    reg_addr,addr);
            qgen("\tP(R7) = R%d;",reg_addr);    // push fetched addr
            free_32_reg(reg_addr);
        }
    }
}

// arithmetics

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

void qgen_get_vararg(int type, int arg_or_var, unsigned int addr, int scope) {
    int reg;
    if(scope == 0) {
        if(type == FLOAT) {
            qgen("\tRR%d = D(0x%x);",
                    get_64_reg(),addr);
        } else {
            qgen("\tR%d = %s(0x%x);",
                    get_32_reg(),type_access(type),addr);
        }
    } else {
        if(type == FLOAT) {
            if(arg_or_var == VAR_T) {
                reg = get_64_reg();
                qgen("\tRR%d = D(R6-%d);",   // up/down 1 in stack
                        reg,addr);
            } else {
                reg = get_32_reg();
                qgen("\tR%d = P(R6+%d);",  // get memory addr
                        reg,addr);
                qgen("\tRR%d = %s(R%d);",
                        get_64_reg(),type_access(type),reg);
                free_32_reg(reg);
            }
        } else {
            if(arg_or_var == VAR_T) {
                reg = get_32_reg();
                qgen("\tR%d = %s(R6-%d);",   // up/down 1 in stack
                        reg,type_access(type),addr);
            } else {
                reg = get_32_reg();
                qgen("\tR%d = U(R6+%d);",  // get memory addr
                        reg,addr);
                qgen("\tR%d = %s(R%d);",
                        reg,type_access(type),reg);
            }
        }
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

void qgen_un_op(int plus_or_minus, int type, int arg_or_var, unsigned int addr, int scope) {
    int reg_addr;
    if(scope == 0) {
        qgen("\t%s(0x%x) %c%c;",
                type_access(type),addr,plus_or_minus,plus_or_minus);
    } else {
        if(arg_or_var == VAR_T) {
            qgen("\t%s(R6-%d) %c%c;",   // up/down 1 in stack
                    type_access(type),addr,plus_or_minus,plus_or_minus);
        } else {
            reg_addr = get_32_reg();
            qgen("\tR%d = U(R6+%d);",  // get memory address
                    reg_addr,addr);
            qgen("\t%s(R%d) %c%c;",   // up/down 1 in memory
                    type_access(type),reg_addr,plus_or_minus,plus_or_minus);
            free_32_reg(reg_addr);
        }
    }
}

void qgen_assign(int type, int arg_or_var, unsigned int address, int scope) {
    char up_or_down = (arg_or_var == VAR_T) ? '-' : '+';
    if(scope == 0) {                // put value in stat mem
        if(type == FLOAT) {
            qgen("\tD(0x%x) = RR%d;",
                    address,result_reg(type));
        } else {
            qgen("\t%s(0x%x) = R%d;",
                    type_access(type),address,result_reg(type));
        }
    } else {                        // put value in the stack
        if(type == FLOAT) {
            qgen("\tD(R6%c%d) = RR%d;",
                    up_or_down,address,result_reg(type));
        } else {
            qgen("\t%s(R6%c%d) = R%d;",
                    type_access(type),up_or_down,address,result_reg(type));
        }
    }
    free_reg(type,result_reg(type));
}

