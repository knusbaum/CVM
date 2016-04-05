#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"
#include "errors.h"

int look;

unsigned long line_number;
unsigned long char_number;

static void get_char(FILE *f) {
    look = fgetc(f);

    char_number++;
    if(look == '\n') {
        line_number++;
        char_number = 1;
    }
}

static void lexer_init(FILE *f) {
    line_number = 1;
    char_number = 1;
    get_char(f);
}

static void match_char(char c, FILE *f) {
    if(look != c) {
        fatal("%lu:%lu Expected: %c, but found %c\n", 1, line_number, char_number, c, look);
    }
    get_char(f);
}

static void consume_whitespace(FILE *f) {
    while(look == ' '
          || look == '\t'
          || look == '\n') {
        get_char(f);
    }
}

static void consume_space(FILE *f) {
    while(look == ' '
          || look == '\t') {
        get_char(f);
    }
}

static void consume_line(FILE *f) {
    while(look != '\n'
          && look != EOF) {
        get_char(f);
    }
    consume_whitespace(f);
}

#define PBSIZE 1025

static char *lex_white_separated(FILE *f) {
    char lexbuff[PBSIZE];
    int pbindex = 0;

    if( look == '\n') return NULL;

    while(look != ' '
          && look != '\t'
          && look != '\n'
          && pbindex < PBSIZE) {
        lexbuff[pbindex++] = look;
        get_char(f);
    }
    if(pbindex >= PBSIZE) {
        fatal("%lu:%lu Token too long\n", 2, line_number, char_number);
    }
    lexbuff[pbindex] = 0;

    char *ret = malloc(pbindex+1);
    strcpy(ret, lexbuff);
    return ret;
}

static void next_instruction(lexed_instr *i, FILE *f) {
    i->instr = NULL;
    i->arg1 = NULL;
    i->arg2 = NULL;

    consume_whitespace(f);
    while(look == '#') {
        consume_line(f);
    }
    if(look == EOF) {
        return;
    }
    i->line = line_number;
    i->instr = lex_white_separated(f);

    consume_space(f);
    i->arg1 = lex_white_separated(f);

    consume_space(f);
    i->arg2 = lex_white_separated(f);
    consume_space(f);

    match_char('\n', f);
}

void tolowers(char *s) {
    while(*s) {
        *s = tolower(*s);
        s++;
    }
}

static void apply_type(lexed_instr *instr) {
    char lexbuff[PBSIZE];
    if(instr->instr == NULL) return;
    
    strcpy(lexbuff, instr->instr);
    tolowers(lexbuff);
    
    if(strcmp(lexbuff, "module") == 0) {
        instr->type = MODULE;
    }
    else if(strcmp(lexbuff, "export") == 0) {
        instr->type = EXPORT;
    }
    else if(strcmp(lexbuff, "mov") == 0) {
        instr->type = MOV;
    }
    else if(strcmp(lexbuff, "push") == 0) {
        instr->type = PUSH;
    }
    else if(strcmp(lexbuff, "pop") == 0) {
        instr->type = POP;
    }
    else if(strcmp(lexbuff, "add") == 0) {
        instr->type = ADD;
    }
    else if(strcmp(lexbuff, "sub") == 0) {
        instr->type = SUB;
    }
    else if(strcmp(lexbuff, "inc") == 0) {
        instr->type = INC;
    }
    else if(strcmp(lexbuff, "data") == 0) {
        instr->type = DATA;
    }
    else if(strcmp(lexbuff, "exit") == 0) {
        instr->type = EXIT;
    }
    else {
        instr->type = LABEL;
    }
}

#define INSTRS_PER_ALLOC 2048

lexed_instr *lex_module(char *filename) {
    FILE *source = fopen(filename, "r");

    lexer_init(source);

    int instrs_scale = 1;
    lexed_instr *instrs = malloc(sizeof (lexed_instr)
                                  * instrs_scale
                                  * INSTRS_PER_ALLOC);
    unsigned long currindex = 0;
    if(instrs == NULL) {
        fatal("Failed to allocate room for instruction parsing.\n", 3);
    }

    lexed_instr *instr;
    do {
        if(currindex == instrs_scale * INSTRS_PER_ALLOC) {
            info("Allocing more instr space for module.\n");
            instrs_scale *= 2;
            instrs = realloc(instrs,
                             sizeof (lexed_instr)
                             * instrs_scale
                             * INSTRS_PER_ALLOC);
        }
        instr = &instrs[currindex++];
        next_instruction(instr, source);
        apply_type(instr);
    } while(instr->instr != NULL);
    info("Returning instrs.\n");
    fclose(source);
    return instrs;
}

void lex_destroy(lexed_instr *instrs) {
    lexed_instr *curr = instrs;
    while(curr->instr) {
        if(curr->instr) {
            free(curr->instr);
        }
        if(curr->arg1) {
            free(curr->arg1);
        }
        if(curr->arg2) {
            free(curr->arg2);
        }
        curr++;
    }
    free(instrs);
}
