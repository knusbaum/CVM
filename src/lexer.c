#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "gc.h"
#include "lexer.h"
#include "errors.h"
#include "buffwriter.h"

int look;

unsigned long line_number;
unsigned long char_number;

static void get_char(FILE *f) {
    look = fgetc(f);

    if(look == '#') {
        do {
            look = fgetc(f);
        } while (look != '\n');
    }

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

    char *ret = GC_MALLOC(pbindex+1);
    strcpy(ret, lexbuff);
    return ret;
}

static void tolowers(char *s) {
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
    else if(strcmp(lexbuff, "extern") == 0) {
        instr->type = EXTERN;
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
    else if(strcmp(lexbuff, "cmp") == 0) {
        instr->type = CMP;
    }
    else if(strcmp(lexbuff, "data") == 0) {
        instr->type = DATA;
    }
    else if(strcmp(lexbuff, "exit") == 0) {
        instr->type = EXIT;
    }
    else if(strcmp(lexbuff, "jmp") == 0) {
        instr->type = JMP;
    }
    else if(strcmp(lexbuff, "je") == 0) {
        instr->type = JE;
    }
    else if(strcmp(lexbuff, "jne") == 0) {
        instr->type = JNE;
    }
    else if(strcmp(lexbuff, "jg") == 0) {
        instr->type = JG;
    }
    else if(strcmp(lexbuff, "jge") == 0) {
        instr->type = JGE;
    }
    else if(strcmp(lexbuff, "jl") == 0) {
        instr->type = JL;
    }
    else if(strcmp(lexbuff, "jle") == 0) {
        instr->type = JLE;
    }
    else if(strcmp(lexbuff, "struct") == 0) {
        instr->type = STRUCT;
    }
    else if(strcmp(lexbuff, "dumpreg") == 0) {
        instr->type = DUMPREG;
    }
    else if(strcmp(lexbuff, "new") == 0) {
        instr->type = NEW;
    }
    else if(strcmp(lexbuff, "push") == 0) {
        instr->type = NEW;
    }
    else if(strcmp(lexbuff, "call") == 0) {
        instr->type = CALL;
    }
    else if(strcmp(lexbuff, "ret") == 0) {
        instr->type = RET;
    }
    else if(strcmp(lexbuff, "import") == 0) {
        instr->type = IMPORT;
    }
    else {
        instr->type = LABEL;
    }
}

static void lex_struct(lexed_struct *s, FILE *f) {
    while(1) {
        consume_whitespace(f);

        if(look == EOF) {
            return;
        }
        char *member_name = lex_white_separated(f);
        consume_space(f);
        char *size = lex_white_separated(f);
        if(member_name == NULL) {
            fatal("%lu:%lu Expected struct member name, but found nothing.\n", 1, line_number, char_number);
        }
        match_char('\n', f);

        if(strcmp(member_name, "endstruct") == 0) {
            return;
        }

        if (size == NULL) {
            fatal("%lu:%lu Expected struct member size, but found nothing.\n", 1, line_number, char_number);
        }

//        info("Adding member %s(%s) to struct.\n", member_name, size);

        if(s->member_count == s->member_length) {
//            info("Reallocing.\n");
            s->member_length *= 2;
            s->members = GC_REALLOC(s->members, s->member_length * sizeof (lexed_member));
        }

        s->members[s->member_count].name = member_name;
        s->members[s->member_count++].size = size;
    }
}

static void lex_extern(lexed_instr *i, FILE *f) {
    size_t arg_count = 0;
    size_t arg_size = 2;
    char **arg_types = GC_MALLOC(arg_size * sizeof (char *));
    char *arg = NULL;
    consume_space(f);
    while ((arg = lex_white_separated(f)) != NULL) {
        consume_space(f);
        info("ARG: (%s)\n", arg);
        arg_types[arg_count++] = arg;
        if(arg_count == arg_size - 1) {
            arg_size *= 2;
            arg_types = GC_REALLOC(arg_types, arg_size * sizeof (char *));
            info("REALLOCING!\n");
        }
    }
    arg_types[arg_count] = NULL; // Null terminated char * array
    i->types = arg_types;
}

static void parse_data(lexed_instr *i, FILE *f) {
    writable_buffer buf;
    createbuffer(&buf);

    consume_space(f);
    if(look == ',') {
        get_char(f);
    }
    if(look == '\n') {
        fatal("Line %d: Expected data declaration.\n", i->line);
    }
    if(look == '"') {
        get_char(f);
        while(look != '"') {
            if(look == '\\') {
                get_char(f);
                switch(look) {
                case 'n': buffer_write_byte(&buf, '\n'); break;
                case 't': buffer_write_byte(&buf, '\t'); break;
                case '\\': buffer_write_byte(&buf, '\\'); break;
                }
                get_char(f);
                continue;
            }
            
            buffer_write_byte(&buf, look);
            get_char(f);
        }
        get_char(f);
    }
    else {
        fatal("Line %d: Can only accept string data currently.\n", i->line);
    }
    size_t length = buf.buff_ptr - buf.buff;
    char *data = GC_MALLOC(length + 1);
    memcpy(data, buf.buff, length);
    data[length] = 0;
//    info("FOUND STRING %s\n", data);
    i->arg2 = data;
}

static void next_instruction(lexed_instr *i, FILE *f) {
    i->instr = NULL;
    i->arg1 = NULL;
    i->arg2 = NULL;

    consume_whitespace(f);

    if(look == EOF) {
        return;
    }
    i->line = line_number;
    i->instr = lex_white_separated(f);
    apply_type(i);
    if(i->type == EXTERN) {
        lex_extern(i, f);
        return;
    }

    consume_space(f);
    i->arg1 = lex_white_separated(f);
    // Interrupt the reading here if we have a DATA instruction.
    if(i->type == DATA) {
        parse_data(i, f);
        return;
    }

    consume_space(f);
    i->arg2 = lex_white_separated(f);
    consume_space(f);

    match_char('\n', f);

    if(i->type == STRUCT) {
//        info("Lexer creating struct.\n");
        lexed_struct *structure = GC_MALLOC(sizeof (lexed_struct));
        structure->member_count = 0;
        structure->member_length = 8;
        structure->members = GC_MALLOC(sizeof (lexed_member) * structure->member_length);
        lex_struct(structure, f);
        i->lexed_struct = structure;
    }
}

#define INSTRS_PER_ALLOC 2048

lexed_instr *lex_module(char *filename) {
    FILE *source = fopen(filename, "r");
    if(source == NULL) {
        printf("Failed to open file %s\n", filename);
        perror("");
        abort();
    }

    lexer_init(source);

    int instrs_scale = 1;
    lexed_instr *instrs = GC_MALLOC(sizeof (lexed_instr)
                                  * instrs_scale
                                  * INSTRS_PER_ALLOC);
    unsigned long currindex = 0;
    if(instrs == NULL) {
        fatal("Failed to allocate room for instruction parsing.\n", 3);
    }

    lexed_instr *instr;
    do {
        if(currindex == instrs_scale * INSTRS_PER_ALLOC) {
//            info("Allocing more instr space for module.\n");
            instrs_scale *= 2;
            instrs = GC_REALLOC(instrs,
                             sizeof (lexed_instr)
                             * instrs_scale
                             * INSTRS_PER_ALLOC);
        }
        instr = &instrs[currindex++];
        next_instruction(instr, source);
    } while(instr->instr != NULL);
    fclose(source);
    return instrs;
}

void lex_destroy(lexed_instr *instrs) {
//    lexed_instr *curr = instrs;
//    while(curr->instr) {
//        if(curr->type == STRUCT) {
//            if(curr->instr) {
//                free(curr->instr);
//            }
//            if(curr->arg1) {
//                free(curr->arg1);
//            }
//            if(curr->lexed_struct) {
//                for(int i = 0; i < curr->lexed_struct->member_count; i++) {
//                    free(curr->lexed_struct->members[i].name);
//                }
//                free(curr->lexed_struct->members);
//                free(curr->lexed_struct);
//            }
//        }
//        else {
//            if(curr->instr) {
//                free(curr->instr);
//            }
//            if(curr->arg1) {
//                free(curr->arg1);
//            }
//            if(curr->arg2) {
//                free(curr->arg2);
//            }
//        }
//        curr++;
//    }
//    free(instrs);
}
