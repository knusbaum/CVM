#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"
#include "lexer.h"
#include "parser.h"

// Registers
void *R0;
void *R1;
void *R2;
void *R3;
void *R4;
void *R5;
void *R6;
void *R7;
void *R8;
void *R9;
void *R10;
void *R11;
void *R12;

struct module {
    char * filename;
    char * modname;
    size_t export_space;
    size_t export_count;
    char **exports;
    lexed_instr *instrs;
    size_t instr_count; // Note: this is not related to instrs, but the actual instructions.
};

static void translate_instruction(struct module *m, lexed_instr *instr);
static void ensure_export_space(struct module *m);

//static void ensure_zero_arg(struct module *m, lexed_instr *instr);
static void ensure_one_arg(struct module *m, lexed_instr *instr);
static void ensure_two_arg(struct module *m, lexed_instr *instr);

static void translate_args(lexed_instr *instr);
static void *get_register(char *arg);

void parse_module(char * filename, lexed_instr *instrs) {
    struct module *m = malloc(sizeof (struct module));

    m->filename = filename;
    m->modname = NULL;
    m->export_space = 0;
    m->export_count = 0;
    m->exports = NULL;
    m->instrs = instrs;

    while(instrs->instr != NULL) {
        //printf("{[%d], [%s], [%s], [%s]}\n", instrs->type, instrs->instr, instrs->arg1, instrs->arg2);
        translate_instruction(m, instrs);

        instrs++;
    }
    info("Done installing module.\n", instrs);
    info("MODULE [%s] exports: [\n", m->filename);
    for(int i = 0; i < m->export_count; i++) {
        info("\t%s\n", m->exports[i]);
    }
    info("]\n");
}


static void translate_instruction(struct module *m, lexed_instr *instr) {
    switch(instr->type) {
    case MODULE:
        ensure_one_arg(m, instr);
        if(m->modname != NULL) {
            fatal("Multiple module definition in file: %s : %lu\n", 4, m->filename, instr->line);
        }
        m->modname = instr->arg1;
        break;
    case EXPORT:
        ensure_one_arg(m, instr);
        ensure_export_space(m);
        m->exports[m->export_count++] = instr->arg1;
        break;
    case MOV:
        ensure_two_arg(m, instr);
        translate_args(instr);
    case PUSH:
    case POP:
    case ADD:
    case SUB:
    case LABEL:
        warn("INSTRUCTION NOT IMPLEMENTED: %s : %lu\n", m->filename, instr->line);
    }
}

#define INITIAL_EXPORTS 1
static void ensure_export_space(struct module *m) {

    if(m->export_space == 0) {
        m->exports = malloc(sizeof (char **) * INITIAL_EXPORTS);
        m->export_space = INITIAL_EXPORTS;
        *m->exports = NULL;
    }

    if(m->export_count == m->export_space) {
        m->export_space *= 2;
        m->exports = realloc(m->exports, sizeof (char **) * m->export_space);
    }
}


//static void ensure_zero_arg(struct module *m, lexed_instr *instr) {
//    if(instr->arg1 != NULL || instr->arg2 != NULL) {
//        fatal("Expected no args: %s : %lu\n", 5, m->filename, instr->line);
//    }
//}

static void ensure_one_arg(struct module *m, lexed_instr *instr) {
    if(instr->arg1 == NULL || instr->arg2 != NULL) {
        fatal("Expected one arg: %s : %lu\n", 5, m->filename, instr->line);
    }
}

static void ensure_two_arg(struct module *m, lexed_instr *instr) {
    if(instr->arg1 == NULL || instr->arg2 == NULL) {
        fatal("Expected no args: %s : %lu\n", 5, m->filename, instr->line);
    }
}

static void translate_args(lexed_instr *instr) {
    void * reg = get_register(instr->arg1);
    if(reg) {
        
    }
}

static void *get_register(char *name) {
    return NULL;
}
