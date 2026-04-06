#include "rhhashmap.h"
#include "hash.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void insert(rhhashmap_t *map, char *key, void *value) {
    int psl = 0;
    size_t i = hash(key, map->cap);

    while (true) {
        rhhashmap_entry_t *entry = &map->table[i];

        if (!entry->key) {
            entry->key = key;
            entry->value = value;
            entry->psl = psl;
            map->len++;
            return;
        }

        if (strcmp(entry->key, key) == 0) {
            free(entry->value);
            entry->value = value;
            free(key);
            return;
        }

        if (psl > entry->psl) {
            char *tmp_k = entry->key;
            void *tmp_v = entry->value;
            const int tmp_psl = entry->psl;

            entry->key = key;
            entry->value = value;
            entry->psl = psl;

            key = tmp_k;
            value = tmp_v;
            psl = tmp_psl;
        }

        i = (i + 1) % map->cap;
        psl++;
    }
}

static bool get_index(size_t *out, const rhhashmap_t *map, const char *key) {
    size_t i = hash(key, map->cap);
    int psl = 0;

    while (true) {
        const rhhashmap_entry_t *entry = &map->table[i];

        if (!entry->key || psl > entry->psl) {
            return false;
        }
        if (strcmp(entry->key, key) == 0) {
            *out = i;
            return true;
        }

        i = (i + 1) % map->cap;
        psl++;
    }
}

static bool resize(rhhashmap_t *map) {
    const size_t new_cap = map->cap * 2;
    rhhashmap_entry_t *new_table = calloc(new_cap, sizeof(rhhashmap_entry_t));
    if (!new_table) {
        return false;
    }

    rhhashmap_entry_t *old_table = map->table;
    const size_t old_cap = map->cap;
    map->table = new_table;
    map->len = 0;
    map->cap = new_cap;

    for (size_t i = 0; i < old_cap; i++) {
        if (old_table[i].key) {
            insert(map, old_table[i].key, old_table[i].value);
        }
    }
    free(old_table);

    return true;
}

bool rhhashmap_create(rhhashmap_t *map) {
    rhhashmap_entry_t *table = calloc(RHHASHMAP_MIN_CAPACITY, sizeof(rhhashmap_entry_t));
    if (!table) {
        return false;
    }

    map->table = table;
    map->len = 0;
    map->cap = RHHASHMAP_MIN_CAPACITY;

    return true;
}

void rhhashmap_destroy(const rhhashmap_t *map) {
    for (size_t i = 0; i < map->cap; i++) {
        free(map->table[i].key);
        free(map->table[i].value);
    }
    free(map->table);
}

bool rhhashmap_insert(rhhashmap_t *map, const char *key, const void *value, const size_t value_size) {
    if ((double) (map->len + 1) / (double) map->cap > RHHASHMAP_LOAD_FACTOR) {
        if (!resize(map)) {
            return false;
        }
    }

    const size_t key_size = strlen(key) + 1;
    char *key_owned = malloc(key_size);
    if (!key_owned) {
        return false;
    }
    memcpy(key_owned, key, key_size);

    void *value_owned = malloc(value_size);
    if (!value_owned) {
        free(key_owned);
        return false;
    }
    memcpy(value_owned, value, value_size);

    insert(map, key_owned, value_owned);
    return true;
}

bool rhhashmap_get(void *out, const size_t out_size, const rhhashmap_t *map, const char *key) {
    size_t i;
    if (!get_index(&i, map, key)) {
        return false;
    }

    memcpy(out, map->table[i].value, out_size);
    return true;
}

bool rhhashmap_remove(rhhashmap_t *map, const char *key) {
    size_t i;
    if (!get_index(&i, map, key)) {
        return false;
    }

    free(map->table[i].key);
    free(map->table[i].value);

    while (true) {
        const size_t next_i = (i + 1) % map->cap;
        rhhashmap_entry_t *cur = &map->table[i];
        const rhhashmap_entry_t *next = &map->table[next_i];

        if (!next->key || !next->psl) {
            *cur = (rhhashmap_entry_t){0};
            map->len--;
            return true;
        }

        *cur = *next;
        cur->psl -= 1;
        i = next_i;
    }
}

void rhhashmap_foreach(const rhhashmap_t *map, const rhhashmap_callback_t callback, void *ctx) {
    for (size_t i = 0; i < map->cap; i++) {
        if (map->table[i].key) {
            if (!callback(map->table[i].key, map->table[i].value, ctx)) {
                return;
            }
        }
    }
}
