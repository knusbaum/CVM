#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "errors.h"
#include "lexer.h"
#include "map.h"
#include "parser.h"

static void translate_instruction(struct module *m, lexed_instr *instr, struct binstr *b);
static void ensure_export_space(struct module *m);

static void ensure_zero_arg(struct module *m, lexed_instr *instr);
static void ensure_one_arg(struct module *m, lexed_instr *instr);
static void ensure_two_arg(struct module *m, lexed_instr *instr);

static void convert_instr(struct module *m, lexed_instr *instr, struct binstr *bin);
static void *parse_arg(struct module *m, char *arg);
static void *get_instr(struct module *m, int line, char * instr);

struct module *parse_module(char * filename, map_t *instrs_regs) {
    lexed_instr *instrs_p = lex_module(filename);
    lexed_instr *instrs = instrs_p;
    struct module *m = malloc(sizeof (struct module));

    m->filename = filename;
    m->modname = NULL;
    m->export_space = 0;
    m->export_count = 0;
    m->exports = NULL;
    m->binstr_count = 0;
    m->binstrs = malloc(sizeof (struct binstr) * 4096);
    m->instrs_regs = instrs_regs;
    m->data = NULL;
    m->labels = map_create();
    m->structures = map_create();

    while(instrs->instr != NULL) {
        //printf("{[%d], [%s], [%s], [%s]}\n", instrs->type, instrs->instr, instrs->arg1, instrs->arg2);
        struct binstr b;
        translate_instruction(m, instrs, &b);
        if(b.instr) {
            m->binstrs[m->binstr_count++] = b;
        }
        instrs++;
    }
    info("MODULE [%s] exports: [\n", m->filename);
    for(int i = 0; i < m->export_count; i++) {
        info("\t%s\n", m->exports[i]);
    }
    info("]\n");
    info("instructions: [\n");
    for(int i = 0; i < m->binstr_count; i++) {
        info("\t%p: { %p, %p, %p }\n",
             m->binstrs + i,
             m->binstrs[i].instr, m->binstrs[i].a1, m->binstrs[i].a2);
    }
    info("]\n");
    lex_destroy(instrs_p);
    return m;
}

static void add_structure(struct module *m, lexed_instr *instr) {
    struct parsed_struct *structure = malloc(sizeof (struct parsed_struct));
    structure->struct_size = 0;
    structure->members = map_create();
    for(size_t i = 0; i < instr->lexed_struct->member_count; i++) {
        lexed_member *member = instr->lexed_struct->members + i;
        if(map_present(structure->members, member->name)) {
            fatal("Line %d: Duplicate member %s in structure %s.\n",
                  5, instr->line, member->name, instr->arg1);
        }
        map_put(structure->members, member->name, (void *)i);
        structure->struct_size++;
    }
    map_put(m->structures, instr->arg1, structure);
}

static void translate_instruction(struct module *m, lexed_instr *instr, struct binstr *b) {
    b->instr = NULL;
    b->a1 = NULL;
    b->a2 = NULL;

    char *label;
    switch(instr->type) {
    case MODULE:
        ensure_one_arg(m, instr);
        if(m->modname != NULL) {
            fatal("Multiple module definition in file: %s : %lu\n", 4, m->filename, instr->line);
        }
        char *modname = malloc(strlen(instr->arg1) + 1);
        strcpy(modname, instr->arg1);
        m->modname = modname;
        break;
    case EXPORT:
        ensure_one_arg(m, instr);
        ensure_export_space(m);
        char *export = malloc(strlen(instr->arg1) + 1);
        strcpy(export, instr->arg1);
        m->exports[m->export_count++] = export;
        break;
    case EXIT:
        ensure_zero_arg(m, instr);
        b->instr = get_instr(m, instr->line, "exit");
        break;
    case MOV:
        ensure_two_arg(m, instr);
        convert_instr(m, instr, b);
        break;
    case INC:
        ensure_one_arg(m, instr);
        convert_instr(m, instr, b);
        break;
    case ADD:
        ensure_two_arg(m, instr);
        convert_instr(m, instr, b);
        break;
    case SUB:
        ensure_two_arg(m, instr);
        convert_instr(m, instr, b);
        break;
    case LABEL:
        ensure_zero_arg(m, instr);
        label = malloc(strlen(instr->instr));
        strcpy(label, instr->instr);
        label[strlen(label)-1] = 0;
        info("Creating label [%s] for: %p\n",
             label, m->binstrs + m->binstr_count);
        map_put(m->labels, label, m->binstrs + m->binstr_count);
        break;
    case JMP:
        ensure_one_arg(m, instr);
        b->instr = get_instr(m, instr->line, "jmpcalc");
        label = malloc(strlen(instr->instr));
        strcpy(label, instr->arg1);
        b->a1 = label;
        break;
    case STRUCT:
        ensure_two_arg(m, instr);
        info("Creating struct with name %s\n", instr->arg1);
        if(map_present(m->structures, instr->arg1)) {
            fatal("Line %d: Redefinition of struct %s.\n", 4, instr->line, instr->arg1);
        }
        add_structure(m, instr);
        break;
    case NEW:
        ensure_two_arg(m, instr);
        b->instr = get_instr(m, instr->line, "new");
        void *reg = map_get(m->instrs_regs, instr->arg1);
        struct parsed_struct *st = map_get(m->structures, instr->arg2);
        if(!st) {
            fatal("Line %d: No such struct: %s.\n",
                  7, instr->line, instr->arg2);
        }
        b->a1 = reg;
        b->a2 = (void *)st->struct_size;
        info("New on register %s(%p) of size %d.\n",
             instr->arg1, reg, st->struct_size);
        break;
    case DATA:
    case PUSH:
    case POP:
        fatal("INSTRUCTION NOT IMPLEMENTED: [%s] %s : %lu\n",
              6, instr->instr, m->filename, instr->line);
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


static void ensure_zero_arg(struct module *m, lexed_instr *instr) {
    if(instr->arg1 != NULL || instr->arg2 != NULL) {
        fatal("Expected no args: %s : %lu\n", 5, m->filename, instr->line);
    }
}

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

static void *get_instr(struct module *m, int line, char * instr) {
    void * istr = map_get(m->instrs_regs, instr);
    if(istr == NULL) {
        fatal("Instruction %s not found. %s : %d\n", 6,
              instr, m->filename, line);
    }
    return istr;
}

static void convert_instr(struct module *m, lexed_instr *instr, struct binstr *bin) {
    void * a1 = map_get(m->instrs_regs, instr->arg1);
    void * a2 = map_get(m->instrs_regs, instr->arg2);

    char instr_name[10];
    strcpy(instr_name, instr->instr);
    if(a1) {
        bin->a1 = a1;
        strcat(instr_name, "r");
    }
    else if(instr->arg1) {
//        bin->a1 = parse_arg(m, instr->arg1);
        fatal("Target must be a register. Moving to memory not implemented. %s : %d\n", 6,
              m->filename, instr->line);
    }

    if(a2) {
        bin->a2 = a2;
        strcat(instr_name, "r");
    }
    else if(instr->arg2) {
        bin->a2 = parse_arg(m, instr->arg2);
        strcat(instr_name, "c");
    }

    bin->instr = get_instr(m, instr->line, instr_name);
}

static void *parse_arg(struct module *m, char *arg) {
    intptr_t x;
    sscanf(arg, "$%ld", &x);
    return (void *)x;
}

void destroy_module(struct module *m) {
    free(m->modname);
    if(m->exports) {
        for(int i = 0; i < m->export_count; i++) {
            free(m->exports[i]);
        }
        free(m->exports);
    }
    if(m->binstrs) free(m->binstrs);
    if(m->data) map_destroy(m->data);
    map_destroy(m->labels);
    free(m);
}
