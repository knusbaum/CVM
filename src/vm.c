#include <stdlib.h>
#include <stdint.h>
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
    info("R0:  0x%.8X\n", R0);
    info("R1:  0x%.8X\n", R1);
    info("R2:  0x%.8X\n", R2);
    info("R3:  0x%.8X\n", R3);
    info("R4:  0x%.8X\n", R4);
    info("R5:  0x%.8X\n", R5);
    info("R6:  0x%.8X\n", R6);
    info("R7:  0x%.8X\n", R7);
    info("R8:  0x%.8X\n", R8);
    info("R9:  0x%.8X\n", R9);
    info("R10: 0x%.8X\n", R10);
    info("R11: 0x%.8X\n", R11);
    info("R12: 0x%.8X\n", R12);
}

void run_module(char *filename) {
    map_t *m = get_registers();
    map_put(m, "movrr", &&movrr);
    map_put(m, "movrc", &&movrc);
    map_put(m, "incr", &&incr);
    map_put(m, "addrr", &&addrr);
    map_put(m, "addrc", &&addrc);
    map_put(m, "subrr", &&subrr);
    map_put(m, "subrc", &&subrc);
    
    map_put(m, "exit", &&exit);
    
    struct module *module = parse_module(filename, m);
    struct binstr *bs = module->binstrs;
    
    goto *bs->instr;
    return;
    
movrr:
    info("Executing [MOVRR] on %p %p\n", bs->a1, bs->a2);
    *((uintptr_t *)bs->a1) = *((uintptr_t *)bs->a2);
    bs++;
    goto *bs->instr;
movrc:
    info("Executing [MOVRC] on %p %lu\n", bs->a1, (uintptr_t)bs->a2);
    *((uintptr_t *)bs->a1) = (uintptr_t)bs->a2;
    bs++;
    goto *bs->instr;
incr:
    info("Executing  [INCR] on %p\n", bs->a1);
    *((uintptr_t *)bs->a1) = *((uintptr_t *)bs->a1)+1;
    bs++;
    goto *bs->instr;
addrr:
    info("Executing [ADDRR] on %p %p\n", bs->a1, bs->a2);
    *((uintptr_t *)bs->a1) += *((uintptr_t *)bs->a2);
    bs++;
    goto *bs->instr;
addrc:
    info("Executing [ADDRC] on %p %lu\n", bs->a1, (uintptr_t)bs->a2);
    *((uintptr_t *)bs->a1) += (uintptr_t)bs->a2;
    bs++;
    goto *bs->instr;
subrr:
    info("Executing [SUBRR] on %p %p\n", bs->a1, bs->a2);
    *((uintptr_t *)bs->a1) -= *((uintptr_t *)bs->a2);
    bs++;
    goto *bs->instr;
subrc:
    info("Executing [SUBRC] on %p %lu\n", bs->a1, (uintptr_t)bs->a2);
    *((uintptr_t *)bs->a1) -= (uintptr_t)bs->a2;
    bs++;
    goto *bs->instr;


exit:
    info("Executing  [EXIT]\n");
    destroy_module(module);
    map_destroy(m);
    return;
}
