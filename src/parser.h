
struct binstr {
    void *instr;
    void *a1;
    void *a2;
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
};

struct module *parse_module(char * filename, map_t *instrs_regs);
void destroy_module(struct module *m);

