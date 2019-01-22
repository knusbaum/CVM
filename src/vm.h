
#define FLAG_EQUAL 0x1
#define FLAG_LESS (0x1 << 1)
#define FLAG_GREATER (0x1 << 2)

#define R0  0
#define R1  1
#define R2  2
#define R3  3
#define R4  4
#define R5  5
#define R6  6
#define R7  7
#define R8  8
#define R9  9
#define R10 10
#define R11 11
#define R12 12
#define IP  13
#define SP  14

#define STACKSIZE 10240

map_t *get_registers();
void run_module(struct module *module);
struct module *load_module(char *filename);

struct module *lookup_module(char *modname);

void dump_regs();
