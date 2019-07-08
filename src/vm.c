#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ffi.h>
#include "gc.h"
#include "errors.h"
#include "map.h"
#include "parser.h"
#include "vm.h"


uintptr_t stack[STACKSIZE];
uintptr_t registers[15];
uintptr_t flags;

static void register_module(struct module *module);

map_t *get_registers() {
    map_t *m = map_create();
    map_put(m, "R0",  (void *)R0);
    map_put(m, "R1",  (void *)R1);
    map_put(m, "R2",  (void *)R2);
    map_put(m, "R3",  (void *)R3);
    map_put(m, "R4",  (void *)R4);
    map_put(m, "R5",  (void *)R5);
    map_put(m, "R6",  (void *)R6);
    map_put(m, "R7",  (void *)R7);
    map_put(m, "R8",  (void *)R8);
    map_put(m, "R9",  (void *)R9);
    map_put(m, "R10", (void *)R10);
    map_put(m, "R11", (void *)R11);
    map_put(m, "R12", (void *)R12);
    map_put(m, "IP",  (void *)IP);
    map_put(m, "SP",  (void *)SP);
    return m;
}

void dump_regs() {
    info("Current State:\n");
    info("R0:  0x%.16lX\n", registers[0]);
    info("R1:  0x%.16lX\n", registers[1]);
    info("R2:  0x%.16lX\n", registers[2]);
    info("R3:  0x%.16lX\n", registers[3]);
    info("R4:  0x%.16lX\n", registers[4]);
    info("R5:  0x%.16lX\n", registers[5]);
    info("R6:  0x%.16lX\n", registers[6]);
    info("R7:  0x%.16lX\n", registers[7]);
    info("R8:  0x%.16lX\n", registers[8]);
    info("R9:  0x%.16lX\n", registers[9]);
    info("R10: 0x%.16lX\n", registers[10]);
    info("R11: 0x%.16lX\n", registers[11]);
    info("R12: 0x%.16lX\n", registers[12]);
    info("IP: 0x%.16lX\n", registers[13]);
    info("SP: 0x%.16lX\n", registers[14]);
}

#include <unistd.h>

//        info("%p\n", bs);
#define NEXTI {                                 \
        lbs += (sizeof (struct binstr));                \
        goto *(bs->instr);                      \
    }


#define bs ((struct binstr *)registers[IP])
#define lbs (registers[IP])
map_t *regmap;

static void __vm(struct module *module, map_t **vm_map) {
//    int a1;
//    char *a2;
//    size_t a3;


    void *target;
    unsigned char *ob, *ob2;
    uint8_t *ob8, *ob8_2;
    uint16_t *ob16, *ob16_2;
    uint32_t *ob32, *ob32_2;
    uint64_t *ob64, *ob64_2;
    struct ffi_call *fcall;
    ffi_arg rc;
    void *values[100];

    if(vm_map != NULL) {
        if(regmap != NULL) {
            *vm_map = regmap;
            return;
        }
        regmap = get_registers();
        map_put(regmap, "movrr", &&movrr);
        map_put(regmap, "movrc", &&movrc);
        map_put(regmap, "movro", &&movro);
        map_put(regmap, "movor", &&movor);
        map_put(regmap, "movoc", &&movoc);
        map_put(regmap, "movoo", &&movoo);

        // Integer arithmetic
        map_put(regmap, "incr", &&incr);
        map_put(regmap, "cmprr", &&cmprr);
        map_put(regmap, "cmprc", &&cmprc);
        map_put(regmap, "addrr", &&addrr);
        map_put(regmap, "addrc", &&addrc);
        map_put(regmap, "subrr", &&subrr);
        map_put(regmap, "subrc", &&subrc);

        // Jumps
        map_put(regmap, "jmp", &&jmp);
        map_put(regmap, "je", &&je);
        map_put(regmap, "jne", &&jne);
        map_put(regmap, "jg", &&jg);
        map_put(regmap, "jge", &&jge);
        map_put(regmap, "jl", &&jl);
        map_put(regmap, "jle", &&jle);

        map_put(regmap, "jmpr", &&jmpr);
        map_put(regmap, "jer", &&jer);
        map_put(regmap, "jner", &&jner);
        map_put(regmap, "jgr", &&jgr);
        map_put(regmap, "jger", &&jger);
        map_put(regmap, "jlr", &&jlr);
        map_put(regmap, "jler", &&jler);

        map_put(regmap, "newc", &&newc);
        map_put(regmap, "newr", &&newr);
        map_put(regmap, "pushr", &&pushr);
        map_put(regmap, "pushc", &&pushc);
        map_put(regmap, "popr", &&popr);

        map_put(regmap, "call", &&call);
        map_put(regmap, "fcall", &&fcall);
        map_put(regmap, "ret", &&ret);

        map_put(regmap, "dumpreg", &&dumpreg);
        map_put(regmap, "exit", &&exit);
        *vm_map = regmap;
        return;
    }

    // Initialize the stack.
    info("Stack starts at %p\n", stack);
    registers[SP] = (uintptr_t)stack;

    // Set the initial instruction pointer.
    target = map_get(module->labels, "start");
    if(target == NULL) {
        fatal("Cannot run module %s. label start not present.\n",
              9, module->modname);
    }
    info("Beginning execution at start label %p\n", target);
    lbs = (uintptr_t)target;

    // Begin execution.
    goto *bs->instr;
    return;

movrr:
//    info("Executing [MOVRR] on regs %d, %d\n", bs->a1, bs->a2);
    registers[bs->a1] = registers[bs->a2];
    NEXTI;
movrc:
//    info("Executing [MOVRC] on reg %d %lu\n", bs->a1, (uintptr_t)bs->a2);
    registers[bs->a1] = bs->constant;
    NEXTI;
movro:
    ob = (unsigned char *)registers[bs->a2];
//    info("Executing [MOVRO] on reg %d, object %p(%d)[%d]<%d>\n",
//         bs->a1, ob, bs->msize, bs->offset2 / bs->msize, bs->offset2);
    ob += bs->offset2;

    switch(bs->msize) {
    case 1:
        ob8 = (uint8_t *)ob;
        registers[bs->a1] = *ob8;
        break;
    case 2:
        ob16 = (uint16_t *)ob;
        registers[bs->a1] = *ob16;
        break;
    case 4:
        ob32 = (uint32_t *)ob;
        registers[bs->a1] = *ob32;
        break;
    case 8:
        ob64 = (uint64_t *)ob;
        registers[bs->a1] = *ob64;
        break;
    default:
        fatal("Cannot move %ld bytes.", bs->msize, 21);
        break;
    }
    NEXTI;
movor:
    ob = (unsigned char *)registers[bs->a1];
//    info("Executing [MOVOR] on object %p(%d)[%d]<%d>, reg %d\n",
//         ob, bs->msize, bs->offset / bs->msize, bs->offset, bs->a2);
    ob += bs->offset;

    switch(bs->msize) {
    case 1:
        ob8 = (uint8_t *)ob;
        *ob8 = registers[bs->a2];
        break;
    case 2:
        ob16 = (uint16_t *)ob;
        *ob16 = registers[bs->a2];
        break;
    case 4:
        ob32 = (uint32_t *)ob;
        *ob32 = registers[bs->a2];
        break;
    case 8:
        ob64 = (uint64_t *)ob;
        *ob64 = registers[bs->a2];
        break;
    default:
        fatal("Cannot move %ld bytes.", bs->msize, 21);
        break;
    }

    NEXTI;
movoc:
    ob = (unsigned char *)registers[bs->a1];
//    info("Executing [MOVOC] on object %p(%d)[%d]<%d>, %.16lX\n",
//         ob, bs->msize, bs->offset / bs->msize, bs->offset, bs->constant);
    ob += bs->offset;

    switch(bs->msize) {
    case 1:
        ob8 = (uint8_t *)ob;
        *ob8 = bs->constant;
        break;
    case 2:
        ob16 = (uint16_t *)ob;
        *ob16 = bs->constant;
        break;
    case 4:
        ob32 = (uint32_t *)ob;
        *ob32 = bs->constant;
        break;
    case 8:
        ob64 = (uint64_t *)ob;
        *ob64 = bs->constant;
        break;
    default:
        fatal("Cannot move %ld bytes.", bs->msize, 21);
        break;
    }
    NEXTI;
movoo:
    ob = (uint8_t *)registers[bs->a1];
    ob2 = (uint8_t *)registers[bs->a2];
//    info("Executing [MOVOO] on object %p(%d)[%d]<%d>, from object %p(%d)[%d]<%d>\n",
//         ob, bs->msize, bs->offset / bs->msize, bs->offset,
//         ob2, bs->msize, bs->offset2 / bs->msize, bs->offset2);

    ob += bs->offset;
    ob2 += bs->offset2;

    switch(bs->msize) {
    case 1:
        ob8 = (uint8_t *)ob;
        ob8_2 = (uint8_t *)ob2;
        *ob8 = *ob8_2;
        break;
    case 2:
        ob16 = (uint16_t *)ob;
        ob16_2 = (uint16_t *)ob2;
        *ob16 = *ob16_2;
        break;
    case 4:
        ob32 = (uint32_t *)ob;
        ob32_2 = (uint32_t *)ob2;
        *ob32 = *ob32_2;
        break;
    case 8:
        ob64 = (uint64_t *)ob;
        ob64_2 = (uint64_t *)ob2;
        *ob64 = *ob64_2;
        break;
    default:
        fatal("Cannot move %ld bytes.", bs->msize, 21);
        break;
    }

    NEXTI;
incr:
//    info("Executing  [INCR] on reg %d\n", bs->a1);
    registers[bs->a1]++;
    NEXTI;
cmprr:
//    info("Executing  [CMPRR] on reg %d(%ld), reg %d(%ld) (",
//         bs->a1, registers[bs->a1],
//         bs->a2, registers[bs->a2]);
    flags &= ~(FLAG_EQUAL | FLAG_GREATER | FLAG_LESS);
    if(registers[bs->a1] == registers[bs->a2]) {
        flags |= FLAG_EQUAL;
//        print("EQUAL ");
    }
    else if(registers[bs->a1] > registers[bs->a2]) {
        flags |= FLAG_GREATER;
//        print("GREATER ");
    }
    else {
        flags |= FLAG_LESS;
//        print("LESS ");
    }
//    print(")\n");
    NEXTI;
cmprc:
//    info("Executing  [CMPRC] on reg %d(%ld), constant: %ld (",
//         bs->a1, registers[bs->a1],
//         bs->constant);
    flags &= ~(FLAG_EQUAL | FLAG_GREATER | FLAG_LESS);
    if(registers[bs->a1] == bs->constant) {
        flags |= FLAG_EQUAL;
//        print("EQUAL ");
    }
    else if(registers[bs->a1] > bs->constant) {
        flags |= FLAG_GREATER;
//        print("GREATER ");
    }
    else {
        flags |= FLAG_LESS;
//        print("LESS ");
    }
//    print(")\n");
    NEXTI;
addrr:
//    info("Executing [ADDRR] on regs %d, %d\n", bs->a1, bs->a2);
    registers[bs->a1] += registers[bs->a2];
    NEXTI;
addrc:
//    info("Executing [ADDRC] on reg %d %lu\n", bs->a1, (uintptr_t)bs->a2);
    registers[bs->a1] += bs->constant;
    NEXTI;
subrr:
//    info("Executing [SUBRR] on regs %d, %d\n", bs->a1, bs->a2);
    registers[bs->a1] -= registers[bs->a2];
    NEXTI;
subrc:
//    info("Executing [SUBRC] on reg %d %lu\n", bs->a1, (uintptr_t)bs->a2);
    registers[bs->a1] -= bs->constant;
    NEXTI;

jmpr:
    bs->target = (void *)registers[bs->a1];
jmp:
//    info("Executing [JMP] to %p\n", bs->target);
    lbs = (uintptr_t)bs->target;
    goto *bs->instr;

jer:
    bs->target = (void *)registers[bs->a1];
je:
//    info("Executing [JE] to %p ", bs->target);
    if(flags & FLAG_EQUAL) {
//        info(" (Jumping)\n");
        lbs = (uintptr_t)bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jner:
    bs->target = (void *)registers[bs->a1];
jne:
//    info("Executing [JNE] to %p ", bs->target);
    if(!(flags & FLAG_EQUAL)) {
//        info(" (Jumping)\n");
        lbs = (uintptr_t)bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jgr:
    bs->target = (void *)registers[bs->a1];
jg:
//    info("Executing [JG] to %p ", bs->target);
    if(flags & FLAG_GREATER) {
//        info(" (Jumping)\n");
        lbs = (uintptr_t)bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jger:
    bs->target = (void *)registers[bs->a1];
jge:
//    info("Executing [JGE] to %p ", bs->target);
    if(flags & (FLAG_GREATER | FLAG_EQUAL)) {
//        info(" (Jumping)\n");
        lbs = (uintptr_t)bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jlr:
    bs->target = (void *)registers[bs->a1];
jl:
//    info("Executing [JL] to %p ", bs->target);
    if(flags & FLAG_LESS) {
//        info(" (Jumping)\n");
        lbs = (uintptr_t)bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jler:
    bs->target = (void *)registers[bs->a1];
jle:
//    info("Executing [JLE] to %p ", bs->target);
    if(flags & (FLAG_LESS | FLAG_EQUAL)) {
//        info(" (Jumping)\n");
        lbs = (uintptr_t)bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

newc:
    registers[bs->a1] = (uintptr_t)GC_MALLOC(bs->constant);
//    info("Executing [NEW] object at %p in reg %d size %lu\n",
//         registers[bs->a1], bs->a1, bs->constant * sizeof (void *));
    NEXTI;

newr:
    registers[bs->a1] = (uintptr_t)GC_MALLOC(registers[bs->a2]);
//    info("Executing [NEW] object at %p in reg %d size %lu\n",
//         registers[bs->a1], bs->a1, bs->constant * sizeof (void *));
    NEXTI;

pushr:
    *((uintptr_t *)registers[SP]) = registers[bs->a1];
    registers[SP]+=(sizeof (uintptr_t));
    if(registers[SP] > (uintptr_t)(stack + STACKSIZE)) {
        fatal("Blew the stack.\n", 20);
    }
    NEXTI;

pushc:
    *((uintptr_t *)registers[SP]) = bs->constant;
    registers[SP]+=(sizeof (uintptr_t));
    if(registers[SP] > (uintptr_t)(stack + STACKSIZE)) {
        fatal("Blew the stack.\n", 20);
    }
    NEXTI;

popr:
    registers[SP]-=(sizeof (uintptr_t));
    registers[bs->a1] = *((uintptr_t *)registers[SP]);
    *((uintptr_t *)registers[SP]) = 0; // Clear the stack for GC
    NEXTI;

call:
    *((uintptr_t *)registers[SP]) = (uintptr_t)(bs + 1); //(sizeof (struct binstr));
    registers[SP]+=(sizeof (uintptr_t));
    lbs = (uintptr_t)bs->target;
    goto *bs->instr;

fcall:
    fcall = bs->fcall;
//    for (int i = 0; i < fcall->arg_count; i++) {
//        //values[i] = (void *)registers[i];
//        if(fcall->arg_sizes[i] == 1) {
//            values[i] = (void *) ((uint8_t *)(registers + i));
//        }
//        else if(fcall->arg_sizes[i] == 2) {
//            values[i] = (void *) ((uint16_t *)(registers + i));
//        }
//        else if(fcall->arg_sizes[i] == 4) {
//            values[i] = (void *) ((uint32_t *)(registers + i) );
//        }
//        else if(fcall->arg_sizes[i] == 8) {
//            values[i] = (void *) (registers + i);
//        }
//    }
    for (int i = 0; i < fcall->arg_count; i++) {
        values[i] = &registers[i];
    }
    ffi_call(&fcall->cif, (void *)write, &rc, values);
    registers[R0] = rc;
    NEXTI;

ret:
    registers[SP]-=(sizeof (uintptr_t));
    lbs = *((uintptr_t *)registers[SP]);
//    info("Executing [RET] to instruction %p\n", bs);
    *((uintptr_t *)registers[SP]) = 0; // Clear the stack for GC
    goto *bs->instr;
    NEXTI;

dumpreg:
    printf("R%d: %.16lX (%ld)\n", bs->a1, registers[bs->a1], registers[bs->a1]);
    NEXTI;
exit:
//    info("CVM got EXIT @ %p\n", bs);
    return;
}

struct module *load_module(char *filename) {
    map_t *vm_map;
    __vm(NULL, &vm_map);
    struct module *module = parse_module(filename, vm_map);
    register_module(module);
    info("Successfully loaded module %s.\n", module->modname);
    return module;
}

void run_module(struct module *module) {
    info("Running module %s\n", module->modname);
    __vm(module, NULL);
    destroy_module(module);
}

map_t *module_registry;

static void register_module(struct module *module) {
    if(module_registry == NULL) {
        module_registry = map_create();
    }
    map_put(module_registry, module->modname, module);
}

struct module *lookup_module(char *modname) {
    if(module_registry == NULL) {
        return NULL;
    }
    return map_get(module_registry, modname);
}
