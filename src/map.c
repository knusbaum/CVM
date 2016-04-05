#include <stdlib.h>
#include "map.h"

typedef struct map_t {
    char *keys;
    void *vals;
    size_t count;
    size_t space;
} map_t;

void increase_map_space(map_t *m) {
    m->space *= 2;
    m->keys = realloc(m->keys, sizeof(char *) * m->space);
    m->vals = realloc(m->vals, sizeof(char *) * m->space);
}

#define INITIAL_SPACE 8
map_t *map_create() {
    map_t *m = malloc(sizeof (map_t));
    m->keys = malloc(sizeof (char *) * INITIAL_SPACE);
    m->vals = malloc(sizeof (char *) * INITIAL_SPACE);
    m->count = 0;
    m->space = INITIAL_SPACE;
}

void map_put(map_t *m, char *key, void *val) {
    if(m->count == m->space) {
        increase_map_space(m);
    }
    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
            m->vals[i] = val;
            return;
        }
    }
    m->keys[m->count] = key;
    m->vals[m->count] = val;
    m->count++;
}

void *map_get(map_t *m, char *key) {
    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
            return m->vals[i];
        }
    }
    return NULL;
}

void *map_delete(map_t *m, char *key) {
    
}

void map_destroy(map_t *m) {
    free(m->keys);
    free(m->vals);
    free(m);
}