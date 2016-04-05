#include <stdio.h>
#include "errors.h"
#include "map.h"
//#include "lexer.h"
 //#include "parser.h"
#include "vm.h"

int main(void) {

//    map_t *m = map_create();
//    map_put(m, "movrr", (void *)0x1001);
//    map_put(m, "movrc", (void *)0x1010);
//    map_put(m, "R1", (void *)0x01);
//    map_put(m, "R2", (void *)0x02);
//    map_put(m, "R3", (void *)0x03);
//
//    parse_module("test.cvm", m);

    run_module("test.cvm");
    info("CVM finished. Dumping registers.\n");
    dump_regs();
}
