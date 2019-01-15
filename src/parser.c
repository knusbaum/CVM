#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "gc.h"
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
static uintptr_t parse_arg(struct module *m, char *arg, unsigned long line);
static void *get_instr(struct module *m, int line, char * instr);

struct module *parse_module(char * filename, map_t *instrs_regs) {
    lexed_instr *instrs_p = lex_module(filename);
    lexed_instr *instrs = instrs_p;
    struct module *m = GC_MALLOC(sizeof (struct module));

    m->filename = filename;
    m->modname = NULL;
    m->export_space = 0;
    m->export_count = 0;
    m->exports = NULL;
    m->binstr_count = 0;
    m->binstrs = GC_MALLOC(sizeof (struct binstr) * 4096);
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
        info("\t%p: { instr: %p, a1: %x, a2: %x, offset: %.16lX extra: %.16lx}\n",
             m->binstrs + i,
             m->binstrs[i].instr, m->binstrs[i].a1, m->binstrs[i].a2,
             m->binstrs[i].offset, m->binstrs[i].constant);
    }
    info("]\n");
    lex_destroy(instrs_p);
    return m;
}

static void add_structure(struct module *m, lexed_instr *instr) {
    struct parsed_struct *structure = GC_MALLOC(sizeof (struct parsed_struct));
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

static void parse_jump(char *reginstr, char *calcinstr, struct module *m, lexed_instr *instr, struct binstr *b) {
    if(map_present(m->instrs_regs, instr->arg1)) {
        b->instr = get_instr(m, instr->line, reginstr);
        b->a1 = map_get(m->instrs_regs, instr->arg1);
    }
    else {
        b->instr = get_instr(m, instr->line, calcinstr);
        char *label = GC_MALLOC(strlen(instr->arg1));
        strcpy(label, instr->arg1);
        b->label = label;
    }
}

static void translate_instruction(struct module *m, lexed_instr *instr, struct binstr *b) {
    b->instr = NULL;
    b->a1 = 0;
    b->a2 = 0;
    b->offset = 0;
    b->constant = 0;

    char *label;
    switch(instr->type) {
    case MODULE:
        ensure_one_arg(m, instr);
        if(m->modname != NULL) {
            fatal("Multiple module definition in file: %s : %lu\n", 4, m->filename, instr->line);
        }
        char *modname = GC_MALLOC(strlen(instr->arg1) + 1);
        strcpy(modname, instr->arg1);
        m->modname = modname;
        break;
    case EXPORT:
        ensure_one_arg(m, instr);
        ensure_export_space(m);
        char *export = GC_MALLOC(strlen(instr->arg1) + 1);
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
    case CMP:
        ensure_two_arg(m, instr);
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
        label = GC_MALLOC(strlen(instr->instr));
        strcpy(label, instr->instr);
        label[strlen(label)-1] = 0;
        info("Creating label [%s] for: %p\n",
             label, m->binstrs + m->binstr_count);
        map_put(m->labels, label, m->binstrs + m->binstr_count);
        break;
    case JMP:
        ensure_one_arg(m, instr);
        parse_jump("jmpr", "jmpcalc", m, instr, b);
//        if(map_present(m->instrs_regs, instr->arg1)) {
//            b->instr = get_instr(m, instr->line, "jmpr");
//            b->a1 = map_get(m->instrs_regs, instr->arg1);
//        }
//        else {
//            b->instr = get_instr(m, instr->line, "jmpcalc");
//            label = GC_MALLOC(strlen(instr->arg1));
//            strcpy(label, instr->arg1);
//            b->label = label;
//        }
        info("JUMP: instr->instr: %s\n", instr->instr);
        break;
    case JE:
        ensure_one_arg(m, instr);
        parse_jump("jer", "jecalc", m, instr, b);
//        b->instr = get_instr(m, instr->line, "jecalc");
//        label = GC_MALLOC(strlen(instr->arg1));
//        strcpy(label, instr->arg1);
//        b->label = label;
        break;
    case JNE:
        ensure_one_arg(m, instr);
        parse_jump("jner", "jnecalc", m, instr, b);
//        b->instr = get_instr(m, instr->line, "jnecalc");
//        label = GC_MALLOC(strlen(instr->arg1));
//        strcpy(label, instr->arg1);
//        b->label = label;
        break;
    case JG:
        ensure_one_arg(m, instr);
        parse_jump("jgr", "jgcalc", m, instr, b);
//        b->instr = get_instr(m, instr->line, "jgcalc");
//        label = GC_MALLOC(strlen(instr->arg1));
//        strcpy(label, instr->arg1);
//        b->label = label;
        break;
    case JGE:
        ensure_one_arg(m, instr);
        parse_jump("jger", "jgecalc", m, instr, b);
//        b->instr = get_instr(m, instr->line, "jgecalc");
//        label = GC_MALLOC(strlen(instr->arg1));
//        strcpy(label, instr->arg1);
//        b->label = label;
        break;
    case JL:
        ensure_one_arg(m, instr);
        parse_jump("jlr", "jlcalc", m, instr, b);
//        b->instr = get_instr(m, instr->line, "jlcalc");
//        label = GC_MALLOC(strlen(instr->arg1));
//        strcpy(label, instr->arg1);
//        b->label = label;
        break;
    case JLE:
        ensure_one_arg(m, instr);
        parse_jump("jler", "jlecalc", m, instr, b);
//        b->instr = get_instr(m, instr->line, "jlecalc");
//        label = GC_MALLOC(strlen(instr->arg1));
//        strcpy(label, instr->arg1);
//        b->label = label;
        break;
    case STRUCT:
        ensure_two_arg(m, instr);
        info("Creating struct with name %s\n", instr->arg1);
        if(map_present(m->structures, instr->arg1)) {
            fatal("Line %d: Redefinition of struct %s.\n", 4, instr->line, instr->arg1);
        }
        add_structure(m, instr);
        break;
    case DUMPREG:
        ensure_one_arg(m, instr);
        b->instr = get_instr(m, instr->line, "dumpreg");
        if(map_present(m->instrs_regs, instr->arg1)) {
            b->a1 = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
        }
        else {
            fatal("Line %d: No such register %s\n",
                  7, instr->line, instr->arg1);
        }
        break;
    case NEW:
        ensure_two_arg(m, instr);
        b->instr = get_instr(m, instr->line, "new");
        char reg = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
        struct parsed_struct *st = map_get(m->structures, instr->arg2);
        if(!st) {
            fatal("Line %d: No such struct: %s.\n",
                  7, instr->line, instr->arg2);
        }
        b->a1 = reg;
        b->constant = (uintptr_t)st->struct_size;
        info("New on register %s(%p) of size %d.\n",
             instr->arg1, reg, st->struct_size);
        break;
    case PUSH:
        ensure_one_arg(m, instr);
        if(map_present(m->instrs_regs, instr->arg1)) {
            b->instr = get_instr(m, instr->line, "pushr");
            b->a1 = map_get(m->instrs_regs, instr->arg1);
        }
        else {
            b->instr = get_instr(m, instr->line, "pushc");
            b->constant = parse_arg(m, instr->arg1, instr->line);
        }
        break;
    case POP:
        ensure_one_arg(m, instr);
        if(!map_present(m->instrs_regs, instr->arg1)) {
            fatal("Line %d: No such register %s\n",
                  10, instr->line, instr->arg1);
        }
        b->instr = get_instr(m, instr->line, "popr");
        b->a1 = map_get(m->instrs_regs, instr->arg1);
        break;
    case CALL:
        ensure_one_arg(m, instr);
        b->instr = get_instr(m, instr->line, "call");
        char *label = GC_MALLOC(strlen(instr->arg1));
        strcpy(label, instr->arg1);
        b->label = label;
        break;
    case RET:
        ensure_zero_arg(m, instr);
        b->instr = get_instr(m, instr->line, "ret");
        break;
    case DATA:
    default:
        fatal("INSTRUCTION NOT IMPLEMENTED: [%s] %s : %lu\n",
              6, instr->instr, m->filename, instr->line);
        break;
    }
}

#define INITIAL_EXPORTS 1
static void ensure_export_space(struct module *m) {

    if(m->export_space == 0) {
        m->exports = GC_MALLOC(sizeof (char **) * INITIAL_EXPORTS);
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

static int parse_structmember(struct module *m, lexed_instr *instr, char *regname, char *reg_p, size_t *offset) {
    size_t length = strlen(regname);
    char reg[length];
    char struct_name[length];
    char struct_member[length];
    int scanned = sscanf(regname, "%[^(](%[^.].%[^)])", reg, struct_name, struct_member);
    if(scanned != 3) {
//        fatal("Line %d: Failed to parse struct offset from: %s\n",
//              instr->line, regname);
        return 0;
    }

    if(!map_present(m->instrs_regs, reg)) {
        fatal("Line %d: No such register: %s\n",
              instr->line, reg);
        return 0;
    }
    *reg_p = (uintptr_t)map_get(m->instrs_regs, reg);

    struct parsed_struct *st = map_get(m->structures, struct_name);
    if(!st) {
        fatal("Line %d: No such struct: %s\n",
              instr->line, struct_name);
        return 0;
    }

    if(!map_present(st->members, struct_member)) {
        fatal("Line %d: No such member %s in struct: %s\n",
              6, instr->line, struct_member, struct_name);
        return 0;
    }
    *offset = (size_t)map_get(st->members, struct_member);
    return 1;
}

static void convert_instr(struct module *m, lexed_instr *instr, struct binstr *bin) {
    char a1 = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
    char a2 = (uintptr_t)map_get(m->instrs_regs, instr->arg2);

    char instr_name[10];
    strcpy(instr_name, instr->instr);
    if(map_present(m->instrs_regs, instr->arg1)) {
        bin->a1 = a1;
        strcat(instr_name, "r");
    }
    else if(instr->arg1) {
        char reg_p;
        size_t offset;
        if(parse_structmember(m, instr, instr->arg1, &reg_p, &offset)) {
            bin->a1 = reg_p;
            bin->offset = offset;
            strcat(instr_name, "o");
        }
        else {
            fatal("Target must be a register. Moving to memory not implemented. %s : %d\n", 6,
                  m->filename, instr->line);
        }
    }

    if(map_present(m->instrs_regs, instr->arg2)) {
        bin->a2 = a2;
        strcat(instr_name, "r");
    }
    else if(instr->arg2) {
        char reg_p;
        size_t offset;
        if(parse_structmember(m, instr, instr->arg2, &reg_p, &offset)) {
            bin->a2 = reg_p;
            bin->offset2 = offset;
            strcat(instr_name, "o");
        }
        else {
            bin->constant = parse_arg(m, instr->arg2, instr->line);
            strcat(instr_name, "c");
        }
    }

    bin->instr = get_instr(m, instr->line, instr_name);
}

static uintptr_t parse_arg(struct module *m, char *arg, unsigned long line) {
    uintptr_t x;
    int scanned = sscanf(arg, "$%ld", &x);
    if(scanned < 1) {
        scanned = sscanf(arg, "0x%lx", &x);
    }
    if(scanned < 1) {
        fatal("Line %d: Failed to scan constant from string \"%s\"\n",
              8, line, arg);
    }
    return x;
}

void destroy_module(struct module *m) {
//    free(m->modname);
//    if(m->exports) {
//        for(int i = 0; i < m->export_count; i++) {
//            free(m->exports[i]);
//        }
//        free(m->exports);
//    }
//    if(m->binstrs) free(m->binstrs);
//    if(m->data) map_destroy(m->data);
//    map_destroy(m->labels);
//    free(m);
}
