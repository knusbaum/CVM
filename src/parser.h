struct binstr {
    void *instr;
    unsigned char a1 : 4;
    unsigned char a2 : 4;
    size_t msize : 4;
    size_t offset;
    union {
        uintptr_t constant;
        char *label;
        size_t offset2;
        void *target;
    };
};

struct parsed_member {
    char *name;
    size_t size;
    size_t offset;
};

struct parsed_struct {
    size_t struct_size;
    map_t *members;
};

struct module {
    char * filename;
    char * modname;
    size_t export_space;
    size_t export_count;
    char **exports;
    size_t binstr_count; // Note: this is not related to instrs, but the actual instructions.
    struct binstr *binstrs;
    map_t *instrs_regs;
    map_t *data;
    map_t *labels;

    // map_t<char *, map_t<char *, size_t>>
    // struct_name -> member_name -> offset
    map_t *structures;

};

struct module *parse_module(char * filename, map_t *instrs_regs);
void destroy_module(struct module *m);
