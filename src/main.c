#include <stdio.h>
#include <stdint.h>
#include <ffi.h>
#define LARGE_CONFIG
#include "gc.h"
#include "errors.h"
#include "map.h"
//#include "lexer.h"
#include "parser.h"
#include "vm.h"

typedef struct linked_node {
    struct linked_node *next;
    int val;
} linked_node;

int main(int argc, char **argv) {

//    map_t *m = map_create();
//    map_put(m, "movrr", (void *)0x1001);
//    map_put(m, "movrc", (void *)0x1010);
//    map_put(m, "R1", (void *)0x01);
//    map_put(m, "R2", (void *)0x02);
//    map_put(m, "R3", (void *)0x03);
//
//    parse_module("test.cvm", m);
    if(argc != 2) {
        fatal("Expected a module file as an argument.\n", 10);
    }
    
    GC_INIT();
    struct module *module = load_module(argv[1]);
//    load_module("helpers.cvm");
    run_module(module);
    info("CVM finished. Dumping registers.\n");
    dump_regs();
    info("Size of binstr: %d\n", sizeof (struct binstr));
//#define ALLOC_SIZE (sizeof (linked_node)) + 100
//    linked_node *node = GC_MALLOC(ALLOC_SIZE);
//    for(int i = 0; i < 100000000; i++) {
//        if(i % 10000 == 0)
//            printf("HELLO %d\n", i);
//        linked_node *new = GC_MALLOC(ALLOC_SIZE);
//        new->next = node;
//        new->val = i;
//        node = new;
//    }
//        
//    printf("Last Node: %p\n", node);





//#define ALLOC_SIZE (10 * sizeof (void *))
//    void **node = GC_MALLOC(ALLOC_SIZE);
//    for(int i = 0; i < 100000000; i++) {
//        if(i % 10000 == 0)
//            printf("HELLO %d\n", i);
//        void  **new = GC_MALLOC(ALLOC_SIZE);
//        //new[0] = node;
//        node = new;
//    }
//        
//    printf("Last Node: %p\n", node);
    
}
