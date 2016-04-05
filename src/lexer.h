enum token_type { MODULE, EXPORT, MOV, PUSH, POP, ADD, SUB, LABEL };

typedef struct lexed_instr {
    enum token_type type;
    unsigned long line;
    char * instr;
    char * arg1;
    char * arg2;
} lexed_instr;

lexed_instr *lex_module(char *filename);

