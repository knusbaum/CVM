#include <stdlib.h>
#include <string.h>
#include "gc.h"
#include "map.h"

typedef struct map_t {
    char **keys;
    void **vals;
    size_t count;
    size_t space;
} map_t;

void increase_map_space(map_t *m) {
    m->space *= 2;
    m->keys = GC_REALLOC(m->keys, sizeof(char **) * m->space);
    m->vals = GC_REALLOC(m->vals, sizeof(void **) * m->space);
}

#define INITIAL_SPACE 8
map_t *map_create() {
    map_t *m = GC_MALLOC(sizeof (map_t));
    m->keys = GC_MALLOC(sizeof (char **) * INITIAL_SPACE);
    m->vals = GC_MALLOC(sizeof (void **) * INITIAL_SPACE);
    m->count = 0;
    m->space = INITIAL_SPACE;
    return m;
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
    if(key == NULL) return NULL;

    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
            return m->vals[i];
        }
    }
    return NULL;
}

void *map_delete(map_t *m, char *key) {
    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
            void *ret = m->vals[i];
            m->count--;
            m->keys[i] = m->keys[m->count];
            m->vals[i] = m->vals[m->count];
            return ret;
        }
    }
    return NULL;
}

int map_present(map_t *m, char *key) {
    if(key == NULL) return 0;

    for(size_t i = 0; i < m->count; i++) {
        if(strcmp(m->keys[i], key) == 0) {
            return 1;
        }
    }
    return 0;
}

void map_destroy(map_t *m) {
//    free(m->keys);
//    free(m->vals);
//    free(m);
}
