#include <stdio.h>
#include "lexer.h"
#include "parser.h"

int main(void) {

    lexed_instr *instrs = lex_module("test.cvm");
    parse_module("test.cvm", instrs);
    
}
