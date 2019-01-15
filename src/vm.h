
map_t *get_registers();
void run_module(struct module *module);
struct module *load_module(char *filename);

struct module *lookup_module(char *modname);

void dump_regs();
