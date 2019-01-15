#include <stdlib.h>
#include <stdint.h>
#include "gc.h"
#include "errors.h"
#include "map.h"
#include "vm.h"
#include "parser.h"

// Registers
uintptr_t R0;
uintptr_t R1;
uintptr_t R2;
uintptr_t R3;
uintptr_t R4;
uintptr_t R5;
uintptr_t R6;
uintptr_t R7;
uintptr_t R8;
uintptr_t R9;
uintptr_t R10;
uintptr_t R11;
uintptr_t R12;

uintptr_t registers[13];
uintptr_t flags;

#define FLAG_EQUAL 0x1
#define FLAG_LESS (0x1 << 1)
#define FLAG_GREATER (0x1 << 2)

map_t *get_registers() {
    map_t *m = map_create();
    map_put(m, "R0", (void *)0);
    map_put(m, "R1", (void *)1);
    map_put(m, "R2", (void *)2);
    map_put(m, "R3", (void *)3);
    map_put(m, "R4", (void *)4);
    map_put(m, "R5", (void *)5);
    map_put(m, "R6", (void *)6);
    map_put(m, "R7", (void *)7);
    map_put(m, "R8", (void *)8);
    map_put(m, "R9", (void *)9);
    map_put(m, "R10", (void *)10);
    map_put(m, "R11", (void *)11);
    map_put(m, "R12", (void *)12);
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
}

//        info("%p\n", bs);
#define NEXTI {                                 \
        bs++;                                   \
        goto *bs->instr;                        \
    }

void run_module(char *filename) {
    map_t *m = get_registers();
    map_put(m, "movrr", &&movrr);
    map_put(m, "movrc", &&movrc);
    map_put(m, "movro", &&movro);
    map_put(m, "movor", &&movor);
    map_put(m, "movoc", &&movoc);
    map_put(m, "movoo", &&movoo);

    // Integer arithmetic
    map_put(m, "incr", &&incr);
    map_put(m, "cmprr", &&cmprr);
    map_put(m, "cmprc", &&cmprc);
    map_put(m, "addrr", &&addrr);
    map_put(m, "addrc", &&addrc);
    map_put(m, "subrr", &&subrr);
    map_put(m, "subrc", &&subrc);

    // Jumps
    map_put(m, "jmpcalc", &&jmpcalc);
    map_put(m, "jecalc", &&jecalc);
    map_put(m, "jnecalc", &&jnecalc);
    map_put(m, "jgcalc", &&jgcalc);
    map_put(m, "jgecalc", &&jgecalc);
    map_put(m, "jlcalc", &&jlcalc);
    map_put(m, "jlecalc", &&jlecalc);

    map_put(m, "new", &&new);

    map_put(m, "dumpreg", &&dumpreg);
    map_put(m, "exit", &&exit);

    struct module *module = parse_module(filename, m);
    struct binstr *bs = module->binstrs;

    info("Successfully loaded module %s. Running...\n", module->modname);

    goto *bs->instr;
    return;

    char *label;
    void *target;
    uintptr_t *ob, *ob2;

movrr:
//    info("Executing [MOVRR] on regs %d, %d\n", bs->a1, bs->a2);
    registers[bs->a1] = registers[bs->a2];
    NEXTI;
movrc:
//    info("Executing [MOVRC] on reg %d %lu\n", bs->a1, (uintptr_t)bs->a2);
    registers[bs->a1] = bs->constant;
    NEXTI;
movro:
//    info("Executing [MOVRO] on reg %d object %p(%d)\n", bs->a1, bs->a2, bs->offset2);
    ob = (uintptr_t *)registers[bs->a2];
    ob += bs->offset2;
    registers[bs->a1] = *ob;
    NEXTI;
movor:
//    info("Executing [MOVOR] on object %p(%d), reg %d\n", bs->a1, bs->offset, bs->a2);
    ob = (uintptr_t *)registers[bs->a1];
    ob += bs->offset;
    *ob = registers[bs->a2];
    NEXTI;
movoc:
//    info("Executing [MOVOC] on object %p(%d), %d\n", bs->a1, bs->offset, bs->constant);
    ob = (uintptr_t *)registers[bs->a1];
    ob += bs->offset;
    *ob = bs->constant;
    NEXTI;
movoo:
//    info("Executing [MOVOO] on object %p(%d), object %p(%d)\n", bs->a1, bs->offset, bs->a2, bs->offset2);
    ob = (uintptr_t *)registers[bs->a1];
    ob += bs->offset;
    ob2 = (uintptr_t *)registers[bs->a2];
    ob2 += bs->offset2;
    *ob = *ob2;
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

jmpcalc:
    bs->instr = &&jmp;
docalc:
    label = bs->label;
    target = map_get(module->labels, label);
    if(target == NULL) {
        fatal("Can't jump to label: [%s] because it doesn't exist.\n", 7, label);
    }
    bs->target = target;
    goto *bs->instr;

jecalc:
    bs->instr = &&je;
    goto docalc;

jnecalc:
    bs->instr = &&jne;
    goto docalc;

jgcalc:
    bs->instr = &&jg;
    goto docalc;

jgecalc:
    bs->instr = &&jge;
    goto docalc;

jlcalc:
    bs->instr = &&jl;
    goto docalc;

jlecalc:
    bs->instr = &&jle;
    goto docalc;
    
jmp:
//    info("Executing [JMP] to %p\n", bs->target);
    bs = bs->target;
    goto *bs->instr;

je:
//    info("Executing [JE] to %p ", bs->target);
    if(flags & FLAG_EQUAL) {
//        info(" (Jumping)\n");
        bs = bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jne:
//    info("Executing [JNE] to %p ", bs->target);
    if(!(flags & FLAG_EQUAL)) {
//        info(" (Jumping)\n");
        bs = bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jg:
//    info("Executing [JG] to %p ", bs->target);
    if(flags & FLAG_GREATER) {
//        info(" (Jumping)\n");
        bs = bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");
    NEXTI;

jge:
//    info("Executing [JGE] to %p ", bs->target);
    if(flags & (FLAG_GREATER | FLAG_EQUAL)) {
//        info(" (Jumping)\n");
        bs = bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");    
    NEXTI;

jl:
//    info("Executing [JL] to %p ", bs->target);
    if(flags & FLAG_LESS) {
//        info(" (Jumping)\n");
        bs = bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");    
    NEXTI;

jle:
//    info("Executing [JLE] to %p ", bs->target);
    if(flags & (FLAG_LESS | FLAG_EQUAL)) {
//        info(" (Jumping)\n");
        bs = bs->target;
        goto *bs->instr;
    }
//    info(" (Not jumping)\n");    
    NEXTI;
    
new:
    registers[bs->a1] = (uintptr_t)GC_MALLOC(bs->constant * sizeof (void *));
//    info("Executing [NEW] object at %p in reg %d size %lu\n",
//         registers[bs->a1], bs->a1, bs->constant * sizeof (void *));
    NEXTI;

dumpreg:
    printf("R%d: 0x%.16lX\n", bs->a1, registers[bs->a1]);
    NEXTI;
exit:
//    info("CVM got EXIT @ %p\n", bs);
    destroy_module(module);
    map_destroy(m);
    return;
}
