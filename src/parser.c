#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <libgen.h>
#include <inttypes.h>
#include <ffi.h>
#include "gc.h"
#include "errors.h"
#include "lexer.h"
#include "map.h"
#include "parser.h"
#include "vm.h"

#define JMPCALC 0x1
#define JECALC 0x2
#define JNECALC 0x3
#define JGCALC 0x4
#define JGECALC 0x5
#define JLCALC 0x6
#define JLECALC 0x7
#define CALLCALC 0x8

static void translate_instruction(struct module *m, lexed_instr *instr, struct binstr *b);
static void ensure_export_space(struct module *m);

static void ensure_zero_arg(struct module *m, lexed_instr *instr);
static void ensure_one_arg(struct module *m, lexed_instr *instr);
static void ensure_two_arg(struct module *m, lexed_instr *instr);

static void convert_instr(struct module *m, lexed_instr *instr, struct binstr *bin);
//static uintptr_t parse_arg(struct module *m, char *arg, unsigned long line);
static int parse_arg(struct module *m, char *arg, unsigned long line, uintptr_t *outval);
static void *get_instr(struct module *m, int line, char * instr);
static void calculate_jumps(struct module *m);

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
    m->data = map_create();
    m->labels = map_create();
    m->structures = map_create();
    m->ffi_name = map_create();

    while(instrs->instr != NULL) {
        struct binstr b;
        translate_instruction(m, instrs, &b);
        if(b.instr) {
            m->binstrs[m->binstr_count++] = b;
        }
        instrs++;
    }

    calculate_jumps(m);

    info("MODULE [%s] exports: [\n", m->filename);
    for(int i = 0; i < m->export_count; i++) {
        info("\t%s\n", m->exports[i]);
    }
    info("]\n");
    info("instructions: [\n");
    for(int i = 0; i < m->binstr_count; i++) {
        info("\t%p: { instr: %p, a1: %x, a2: %x, offset: %.16lX, msize: %d, extra: %.16lx}\n",
             m->binstrs + i,
             m->binstrs[i].instr, m->binstrs[i].a1, m->binstrs[i].a2,
             m->binstrs[i].offset, m->binstrs[i].msize, m->binstrs[i].constant);
    }
    info("]\n");
    lex_destroy(instrs_p);
    return m;
}

static void add_structure(struct module *m, lexed_instr *instr) {
    struct parsed_struct *structure = GC_MALLOC(sizeof (struct parsed_struct));
    structure->struct_size = 0;
    structure->members = map_create();
    size_t offset = 0;
    for(int i = 0; i < instr->lexed_struct->member_count; i++) {
        lexed_member *member = instr->lexed_struct->members + i;
        if(map_present(structure->members, member->name)) {
            fatal("Duplicate member %s in structure %s. %s:%d\n",
                  5, member->name, instr->arg1, instr->line, m->filename);
        }
        struct parsed_member *s_member = GC_MALLOC(sizeof (struct parsed_struct));
        s_member->name = member->name;
        //s_member->size = (size_t)parse_arg(m, member->size, instr->line);
        if (parse_arg(m, member->size, instr->line, &s_member->size) < 1) {
            fatal("Failed to scan constant from string \"%s\". %s:%d\n",
                  8, member->size, m->filename, instr->line);
        }
        s_member->offset = offset;
        offset += s_member->size;
        map_put(structure->members, member->name, (void *)s_member);
        structure->struct_size++;
    }
    map_put(m->structures, instr->arg1, structure);
}

static void parse_jump(char *reginstr, uintptr_t calcinstr, struct module *m, lexed_instr *instr, struct binstr *b) {
    if(map_present(m->instrs_regs, instr->arg1)) {
        b->instr = get_instr(m, instr->line, reginstr);
        b->a1 = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
    }
    else {
        b->instr = (void *)calcinstr;
        char *label = GC_MALLOC(strlen(instr->arg1));
        strcpy(label, instr->arg1);
        b->label = label;
    }
}

static ffi_type *parse_ffi_type(char *type) {
    if(strcmp(type, "sint32") == 0) {
        return &ffi_type_sint32;
    }
    else if(strcmp(type, "sint64") == 0) {
        return &ffi_type_sint64;
    }
    else if(strcmp(type, "uint64") == 0) {
        return &ffi_type_uint64;
    }
    else if(strcmp(type, "pointer") == 0) {
        return &ffi_type_pointer;
    }
    else if(strcmp(type, "sint") == 0) {
        return &ffi_type_sint;
    }
    else if(strcmp(type, "uint") == 0) {
        return &ffi_type_uint;
    }
    else if(strcmp(type, "slong") == 0) {
        return &ffi_type_slong;
    }
    else if(strcmp(type, "ulong") == 0) {
        return &ffi_type_ulong;
    }
    fatal("No size for ffi type %p.\n", 10, type);
    return NULL;
}

static size_t size_for_ffi_type(ffi_type *type) {
    if(type == &ffi_type_sint32) return 4;
    else if(type == &ffi_type_sint64) return 8;
    else if(type == &ffi_type_uint64) return 8;
    else if(type == &ffi_type_pointer) return sizeof (void *);
    else if(type == &ffi_type_sint) return sizeof (int);
    else if(type == &ffi_type_uint) return sizeof (unsigned int);
    else if(type == &ffi_type_slong) return sizeof (long);
    else if(type == &ffi_type_ulong) return sizeof (unsigned long);
    fatal("No size for ffi type %p.\n", 10, type);
    return 0;
}

#include <unistd.h>

static void parse_extern(struct module *m, lexed_instr *instr) {
    //ffi_cif cif; // = GC_MALLOC(sizeof (ffi_cif));
//    ffi_type *args[100];
//    void *values[100];
//    char *s;
//    ffi_arg rc;

    ffi_type **args = GC_MALLOC(100 * sizeof (ffi_type *));

    int i = 0;
    char **arg = instr->types;

    ffi_type *ret = parse_ffi_type(*arg);
    if(ret == NULL) {
        fatal("Don't understand extern type %s\n", 1, *arg);
    }
    arg++;

    char *fn_name = *arg;
    arg++;

    while (*arg != NULL) {
        //print("%s ", *arg);
        args[i] = parse_ffi_type(*arg);

        if(args[i] == NULL) {
            fatal("Don't understand extern type %s\n", 1, *arg);
        }
        arg++;
        i++;
    }

    size_t *arg_sizes = GC_MALLOC(i * sizeof (size_t));
    for(int j = 0; j < i; j++) {
        arg_sizes[j] = size_for_ffi_type(args[j]);
    }

    struct ffi_call *fcall = GC_MALLOC(sizeof (struct ffi_call));
    //memset(fcall, 0, 10 * sizeof (struct ffi_call));
    info("CFFI_PREP_CIF %d args.\n", i);
    if (ffi_prep_cif(&fcall->cif, FFI_DEFAULT_ABI, i, ret, args) == FFI_OK) {

        //fcall->cif = cif;
        fcall->fptr = &write;
        fcall->arg_count = i;
        fcall->arg_sizes = arg_sizes;
        info("map_put(m->ffi_name, %s, fcall);\n", fn_name);
        map_put(m->ffi_name, fn_name, fcall);
        printf("Fcall: %p\n", fcall);
    }
    else {
        fatal("Failed to prep ffi for %s.\n", 1, fn_name);
    }

}

static void load_relative(struct module *m, char *modname) {
    // Put together the filename of the desired module relative to the
    // location of the module we're loading.
    size_t filename_len = strlen(m->filename) + strlen(modname) + 1;
    char filename[filename_len];
    strcpy(filename, m->filename);
    dirname(filename);
    strcat(filename, "/");
    strcat(filename, modname);
//    info("IMPORTING: %s\n", filename);
    load_module(filename);
}

static void translate_instruction(struct module *m, lexed_instr *instr, struct binstr *b) {
    b->instr = NULL;
    b->a1 = 0;
    b->a2 = 0;
    b->offset = 0;
    b->msize = 0;
    b->constant = 0;

    char *label;
    switch(instr->type) {
    case MODULE:
        ensure_one_arg(m, instr);
        if(m->modname != NULL) {
            fatal("Multiple module definition in file: %s:%lu\n", 4, m->filename, instr->line);
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
    case EXTERN:
//        info("GOT EXTERN: ");
//        char **arg = instr->types;
//        while (*arg != NULL) {
//            print("%s ", *arg);
//            arg++;
//        }
//        print("\n");
        parse_extern(m, instr);
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
//        info("Creating label [%s] for: %p\n",
//             label, m->binstrs + m->binstr_count);
        map_put(m->labels, label, m->binstrs + m->binstr_count);
        break;
    case JMP:
        ensure_one_arg(m, instr);
        parse_jump("jmpr", JMPCALC, m, instr, b);
        break;
    case JE:
        ensure_one_arg(m, instr);
        parse_jump("jer", JECALC, m, instr, b);
        break;
    case JNE:
        ensure_one_arg(m, instr);
        parse_jump("jner", JNECALC, m, instr, b);
        break;
    case JG:
        ensure_one_arg(m, instr);
        parse_jump("jgr", JGCALC, m, instr, b);
        break;
    case JGE:
        ensure_one_arg(m, instr);
        parse_jump("jger", JGECALC, m, instr, b);
        break;
    case JL:
        ensure_one_arg(m, instr);
        parse_jump("jlr", JLCALC, m, instr, b);
        break;
    case JLE:
        ensure_one_arg(m, instr);
        parse_jump("jler", JLECALC, m, instr, b);
        break;
    case STRUCT:
        ensure_two_arg(m, instr);
//        info("Creating struct with name %s\n", instr->arg1);
        if(map_present(m->structures, instr->arg1)) {
            fatal("Redefinition of struct %s. %s:%d\n", 4, instr->arg1, m->filename, instr->line);
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
            fatal("No such register %s. %s:%d\n",
                  7, instr->arg1, m->filename, instr->line);
        }
        break;
    case NEW:
        ensure_two_arg(m, instr);
        char reg = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
        struct parsed_struct *st = map_get(m->structures, instr->arg2);
        if(st) {
            // Second argument is a struct name.
            b->instr = get_instr(m, instr->line, "newc");
            b->a1 = reg;
            b->constant = (uintptr_t)st->struct_size;
//            info("New struct %s on register %s(0x%x) of size %d.\n",
//                 instr->arg2, instr->arg1, reg, st->struct_size);
        }
        else if(map_present(m->instrs_regs, instr->arg2)) {
            // Second argument is a register.
            b->instr = get_instr(m, instr->line, "newr");
            b->a1 = reg;
            b->a2 = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
//            info("New array on register %s(0x%x) of size from register %s\n",
//                 instr->arg1, reg, instr->arg2);
        }
        else{
            // Second argument is constant.
//            info("CREATING NEW ARRAY.\n");
            //uintptr_t size = parse_arg(m, instr->arg2, instr->line);
            uintptr_t size;
            if(parse_arg(m, instr->arg2, instr->line, &size) < 1) {
                fatal("Failed to scan constant from string \"%s\". %s:%d\n",
                      8, instr->arg2, m->filename, instr->line);
            }
//            info("LENGTH IS %ld\n", size);
            b->instr = get_instr(m, instr->line, "newc");
            b->a1 = reg;
            b->constant = size;
//            info("New array on register %s(0x%x) of size %ld bytes.\n",
//                 instr->arg1, reg, size);
        }
        break;
    case PUSH:
        ensure_one_arg(m, instr);
        if(map_present(m->instrs_regs, instr->arg1)) {
            b->instr = get_instr(m, instr->line, "pushr");
            b->a1 = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
        }
        else {
            b->instr = get_instr(m, instr->line, "pushc");
            //b->constant = parse_arg(m, instr->arg1, instr->line);
            if(parse_arg(m, instr->arg1, instr->line, &b->constant) < 1) {
                fatal("Failed to scan constant from string \"%s\". %s:%d\n",
                      8, instr->arg1, m->filename, instr->line);
            }
        }
        break;
    case POP:
        ensure_one_arg(m, instr);
        if(!map_present(m->instrs_regs, instr->arg1)) {
            fatal("No such register %s. %s:%d\n",
                  10, instr->arg1, m->filename, instr->line);
        }
        b->instr = get_instr(m, instr->line, "popr");
        b->a1 = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
        break;
    case CALL:
        ensure_one_arg(m, instr);
        info("HAVE CALL (%p)%s\n", instr->arg1, instr->arg1);
        b->instr = (void *)CALLCALC;
        char *label = GC_MALLOC(strlen(instr->arg1));
        strcpy(label, instr->arg1);
        b->label = label;
        break;
    case RET:
        ensure_zero_arg(m, instr);
        b->instr = get_instr(m, instr->line, "ret");
        break;
    case IMPORT:
        ensure_one_arg(m, instr);
        load_relative(m, instr->arg1);
        break;
    case DATA:
//        info("Got data: %s = [%s]\n", instr->arg1, instr->arg2);
        map_put(m->data, instr->arg1, instr->arg2);
        break;
    default:
        fatal("INSTRUCTION NOT IMPLEMENTED: [%s] %s:%lu\n",
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
        m->exports = GC_REALLOC(m->exports, sizeof (char **) * m->export_space);
    }
}


static void ensure_zero_arg(struct module *m, lexed_instr *instr) {
    if(instr->arg1 != NULL || instr->arg2 != NULL) {
        fatal("Expected no args: %s:%lu\n", 5, m->filename, instr->line);
    }
}

static void ensure_one_arg(struct module *m, lexed_instr *instr) {
    if(instr->arg1 == NULL || instr->arg2 != NULL) {
        fatal("Expected one arg: %s:%lu\n", 5, m->filename, instr->line);
    }
}

static void ensure_two_arg(struct module *m, lexed_instr *instr) {
    if(instr->arg1 == NULL || instr->arg2 == NULL) {
        fatal("Expected no args: %s:%lu\n", 5, m->filename, instr->line);
    }
}

static void *get_instr(struct module *m, int line, char * instr) {
    void * istr = map_get(m->instrs_regs, instr);
    if(istr == NULL) {
        fatal("Instruction %s not found. %s:%d\n", 6,
              instr, m->filename, line);
    }
    return istr;
}

static int parse_offset(struct module *m, lexed_instr *instr, char *regoff, char *reg_p, size_t *offset, size_t *size) {
    size_t length = strlen(regoff);
    char reg[length];
    long scanned_offset;
    long scanned_size;
    int scanned = sscanf(regoff, "%[^(]($%ld)[$%ld]", reg, &scanned_size, &scanned_offset);
    if(scanned != 3) {
//        info("Couldn't scan offset from [%s]\n", regoff);
        return 0;
    }

    if(!map_present(m->instrs_regs, reg)) {
        fatal("No such register: %s. %s:%d\n", 6,
              reg, m->filename, instr->line);
        return 0;
    }
    if(scanned_size < 1 || scanned_size > sizeof (uintptr_t)) {
        fatal("Size must be between 1 and %ld. %s:%d\n", 6,
              sizeof (uintptr_t), m->filename, instr->line);
    }
    *reg_p = (uintptr_t)map_get(m->instrs_regs, reg);
    //*offset = scanned_offset * scanned_size;
    *offset = scanned_offset * scanned_size;
    *size = scanned_size;
    return 1;
}

static int parse_structmember(struct module *m, lexed_instr *instr, char *regname, char *reg_p, size_t *offset, size_t *size) {
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
        fatal("No such register: %s. %s:%d\n", 6,
              reg, m->filename, instr->line);
        return 0;
    }
    *reg_p = (uintptr_t)map_get(m->instrs_regs, reg);

    struct parsed_struct *st = map_get(m->structures, struct_name);
    if(!st) {
        fatal("No such struct: %s. %s:%d\n", 6,
              struct_name, m->filename, instr->line);
        return 0;
    }

    if(!map_present(st->members, struct_member)) {
        fatal("No such member %s in struct: %s. %s:%d\n",
              6, struct_member, struct_name, m->filename, instr->line);
        return 0;
    }
    //*offset = (size_t)map_get(st->members, struct_member);
    struct parsed_member *s_member = map_get(st->members, struct_member);
    *offset = s_member->offset;
    *size = s_member->size;
    return 1;
}

static void convert_instr(struct module *m, lexed_instr *instr, struct binstr *bin) {
    char a1 = (uintptr_t)map_get(m->instrs_regs, instr->arg1);
    char a2 = (uintptr_t)map_get(m->instrs_regs, instr->arg2);

    int first_is_offset = 0;
    char instr_name[10];
    strcpy(instr_name, instr->instr);
    if(map_present(m->instrs_regs, instr->arg1)) {
        bin->a1 = a1;
        strcat(instr_name, "r");
    }
    else if(instr->arg1) {
        char reg_p;
        size_t offset;
        size_t size;
        if(parse_offset(m, instr, instr->arg1, &reg_p, &offset, &size)) {
            bin->a1 = reg_p;
            bin->offset = offset;
            bin->msize = size;
            strcat(instr_name, "o");
            first_is_offset = 1;
        }
        else if(parse_structmember(m, instr, instr->arg1, &reg_p, &offset, &size)) {
            bin->a1 = reg_p;
            bin->offset = offset;
//            info("STRUCT OFFSET: %d\n", offset);
            bin->msize = size;
            strcat(instr_name, "o");
            first_is_offset = 1;
        }
        else {
            fatal("Target must be a register. Moving to memory not implemented. %s:%d\n", 6,
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
        size_t size;
        if(parse_offset(m, instr, instr->arg2, &reg_p, &offset, &size)) {
            if(first_is_offset && size != bin->msize) {
                fatal("Memory -> memory copy size mismatch: %d != %d. %s:%d\n", 7, bin->msize, size, m->filename, instr->line);
            }
            bin->a2 = reg_p;
            bin->offset2 = offset;
            bin->msize = size;
            strcat(instr_name, "o");
        }
        else if(parse_structmember(m, instr, instr->arg2, &reg_p, &offset, &size)) {
            if(first_is_offset && size != bin->msize) {
                fatal("Memory -> memory copy size mismatch: %d != %d. %s:%d\n", 7, bin->msize, sizeof (uintptr_t), m->filename, instr->line);
            }
            bin->a2 = reg_p;
            bin->offset2 = offset;
//            info("STRUCT OFFSET: %d\n", offset);
            bin->msize = size;
            strcat(instr_name, "o");
        }
        else {
            //bin->constant = parse_arg(m, instr->arg2, instr->line);
            if(parse_arg(m, instr->arg2, instr->line, &bin->constant) < 1) {
                char *data = map_get(m->data, instr->arg2);
                if(data == NULL) {
                    fatal("Failed to parse constant or data name from string \"%s\". %s:%d.\n",
                          8, instr->arg2, m->filename, instr->line);
                }
                bin->target = data;
            }
            strcat(instr_name, "c");
        }
    }

    bin->instr = get_instr(m, instr->line, instr_name);
}

static int parse_arg(struct module *m, char *arg, unsigned long line, uintptr_t *outval) {
    uintptr_t x;
    if (strcmp(arg, "$ptr") == 0) {
        *outval = sizeof (void *);
        return 1;
    }
    int scanned = sscanf(arg, "$%" PRIdPTR, &x);
    if(scanned < 1) {
        scanned = sscanf(arg, "0x%" PRIxPTR, &x);
    }
//    if(scanned < 1) {
//        fatal("Failed to scan constant from string \"%s\". %s:%d\n",
//              8, arg, m->filename, line);
//    }
//    return x;
    *outval = x;
    return scanned;
}

static void *module_call_lookup(struct module *m, unsigned long line, char *label) {
    char modname[1024]; // Static size is bad. Can cause overruns. Do something smarter here.
    char target_label[1024];
    int ret = sscanf(label, "%[^.].%s", modname, target_label);
    if(ret != 2) {
        //fatal("1Can't call label: [%s] because it doesn't exist. %s:%d\n", 7, label, m->filename, line);
        return NULL;
    }
    struct module *module = lookup_module(modname);
    if(module == NULL) {
        //fatal("2Can't call label: [%s] because it doesn't exist. %s:%d\n", 7, label, m->filename, line);
        return NULL;
    }

    void *target = map_get(module->labels, target_label);
    if(target == NULL) {
        //fatal("3Can't call label: [%s] because it doesn't exist. %s:%d\n", 7, label, m->filename, line);
        return NULL;
    }
//    info("Found remote target @ %p\n", target);
    return target;

}

static void calculate_jump(struct module *m, struct binstr *bs) {

    struct binstr *target;
    struct ffi_call *fcall;
    switch((uintptr_t)bs->instr) {
    case JMPCALC:
        bs->instr = get_instr(m, 0, "jmp");
        target = map_get(m->labels, bs->label);
        bs->target = target;
        break;
    case JECALC:
        bs->instr = get_instr(m, 0, "je");
        target = map_get(m->labels, bs->label);
        bs->target = target;
        break;
    case JNECALC:
        bs->instr = get_instr(m, 0, "jne");
        target = map_get(m->labels, bs->label);
        bs->target = target;
        break;
    case JGCALC:
        bs->instr = get_instr(m, 0, "jg");
        target = map_get(m->labels, bs->label);
        bs->target = target;
        break;
    case JGECALC:
        bs->instr = get_instr(m, 0, "jge");
        target = map_get(m->labels, bs->label);
        bs->target = target;
        break;
    case JLCALC:
        bs->instr = get_instr(m, 0, "jl");
        target = map_get(m->labels, bs->label);
        bs->target = target;
        break;
    case JLECALC:
        bs->instr = get_instr(m, 0, "jle");
        target = map_get(m->labels, bs->label);
        bs->target = target;
        break;
    case CALLCALC:
        target = map_get(m->labels, bs->label);
        if(target == NULL) {
            target = module_call_lookup(m, 0, bs->label);
        }
        if(target == NULL) {
            fcall = map_get(m->ffi_name, bs->label);
            if (fcall == NULL) {
                fatal("Can't find call target: %s\n", 1, bs->label);
            }
            bs->fcall = fcall;
            bs->instr = get_instr(m, 0, "fcall");
        }
        else {
            bs->instr = get_instr(m, 0, "call");
            bs->target = target;
        }
        break;
    }
}

static void calculate_jumps(struct module *m) {
    for(int i = 0; i < m->binstr_count; i++) {
        calculate_jump(m, m->binstrs + i);
    }
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
