enum token_type {
    MODULE,
    EXPORT,
    EXTERN,
    MOV,
    PUSH,
    POP,
    ADD,
    SUB,
    INC,
    CMP,
    DATA,
    EXIT,
    JMP,
    JE,
    JNE,
    JG,
    JGE,
    JL,
    JLE,
    LABEL,
    STRUCT,
    DUMPREG,
    NEW,
    CALL,
    RET,
    IMPORT
};

typedef struct lexed_member {
    char *name;
    char *size;
} lexed_member;

typedef struct lexed_struct {
    int member_count;
    int member_length;
    lexed_member *members;
} lexed_struct;

typedef struct lexed_instr {
    enum token_type type;
    unsigned long line;
    char * instr;
    char * arg1;
    union {
        char * arg2;
        lexed_struct *lexed_struct;
        char **types;
    };
} lexed_instr;

lexed_instr *lex_module(char *filename);
void lex_destroy(lexed_instr *instrs);
