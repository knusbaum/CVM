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

map_t *get_registers() {
    map_t *m = map_create();
    map_put(m, "R0", &R0);
    map_put(m, "R1", &R1);
    map_put(m, "R2", &R2);
    map_put(m, "R3", &R3);
    map_put(m, "R4", &R4);
    map_put(m, "R5", &R5);
    map_put(m, "R6", &R6);
    map_put(m, "R7", &R7);
    map_put(m, "R8", &R8);
    map_put(m, "R9", &R9);
    map_put(m, "R10", &R10);
    map_put(m, "R11", &R11);
    map_put(m, "R12", &R12);
    return m;
}

void dump_regs() {
    info("Current State:\n");
    info("R0:  0x%.16lX\n", R0);
    info("R1:  0x%.16lX\n", R1);
    info("R2:  0x%.16lX\n", R2);
    info("R3:  0x%.16lX\n", R3);
    info("R4:  0x%.16lX\n", R4);
    info("R5:  0x%.16lX\n", R5);
    info("R6:  0x%.16lX\n", R6);
    info("R7:  0x%.16lX\n", R7);
    info("R8:  0x%.16lX\n", R8);
    info("R9:  0x%.16lX\n", R9);
    info("R10: 0x%.16lX\n", R10);
    info("R11: 0x%.16lX\n", R11);
    info("R12: 0x%.16lX\n", R12);
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
    
    map_put(m, "incr", &&incr);
    map_put(m, "addrr", &&addrr);
    map_put(m, "addrc", &&addrc);
    map_put(m, "subrr", &&subrr);
    map_put(m, "subrc", &&subrc);
    map_put(m, "jmpcalc", &&jmpcalc);
    map_put(m, "new", &&new);
    
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
//    info("Executing [MOVRR] on %p %p\n", bs->a1, bs->a2);
    *((uintptr_t *)bs->a1) = *((uintptr_t *)bs->a2);
    NEXTI;
movrc:
//    info("Executing [MOVRC] on %p %lu\n", bs->a1, (uintptr_t)bs->a2);
    *((uintptr_t *)bs->a1) = (uintptr_t)bs->a2;
    NEXTI;
movro:
    info("Executing [MOVRO] on %p object %p(%d)\n", bs->a1, bs->a2, bs->offset2);
    ob = *((void **)bs->a2);
    ob += bs->offset2;
    *((uintptr_t *)bs->a1) = *ob;
    NEXTI;
movor:
    info("Executing [MOVOR] on object %p(%d), %p \n", bs->a1, bs->offset, bs->a2);
    ob = *((void **)bs->a1);
    info("bs->a1 == %p\n*bs->a1 == %p\n", bs->a1, ob);
    ob += bs->offset;
    *ob = *((uintptr_t *)bs->a2);
    NEXTI;
movoc:
    info("Executing [MOVOC] on object %p(%d), %p \n", bs->a1, bs->offset, (uintptr_t)bs->a2);
    ob = *((void **)bs->a1);
    ob += bs->offset;
    *ob = (uintptr_t)bs->a2;
    NEXTI;
movoo:
    info("Executing [MOVOO] on object %p(%d), object %p(%d)\n", bs->a1, bs->offset, bs->a2, bs->offset2);
    ob = *((void **)bs->a1);
    ob += bs->offset;
    ob2 = *((void **)bs->a2);
    ob2 += bs->offset2;
    *ob = *ob2;
    NEXTI;
incr:
//    info("Executing  [INCR] on %p\n", bs->a1);
    *((uintptr_t *)bs->a1) = *((uintptr_t *)bs->a1)+1;
    NEXTI;
addrr:
//    info("Executing [ADDRR] on %p %p\n", bs->a1, bs->a2);
    *((uintptr_t *)bs->a1) += *((uintptr_t *)bs->a2);
    NEXTI;
addrc:
//    info("Executing [ADDRC] on %p %lu\n", bs->a1, (uintptr_t)bs->a2);
    *((uintptr_t *)bs->a1) += (uintptr_t)bs->a2;
    NEXTI;
subrr:
//    info("Executing [SUBRR] on %p %p\n", bs->a1, bs->a2);
    *((uintptr_t *)bs->a1) -= *((uintptr_t *)bs->a2);
    NEXTI;
subrc:
//    info("Executing [SUBRC] on %p %lu\n", bs->a1, (uintptr_t)bs->a2);
    *((uintptr_t *)bs->a1) -= (uintptr_t)bs->a2;
    NEXTI;

jmpcalc:
    label = bs->a1;
    target = map_get(module->labels, label);
    if(target == NULL) {
        fatal("Can't jump to label: [%s] because it doesn't exist.\n", 7, label);
    }
//    info("Jumping to: %p\n", target);
    bs = target;
    goto *bs->instr;

new:
    *((uintptr_t *)bs->a1) = (uintptr_t)GC_MALLOC(((uintptr_t)bs->a2) * sizeof (void *));
//    info("Executing [NEW] object at %p in %p size %lu\n",
//         *((uintptr_t *)bs->a1), bs->a1, (uintptr_t)bs->a2);
    NEXTI;
    
exit:
//    info("CVM got EXIT @ %p\n", bs);
    destroy_module(module);
    map_destroy(m);
    return;
}
