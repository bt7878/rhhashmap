#include "rhhashmap.h"

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <rapidhash.h>

static size_t next_pow2(size_t n) {
    n--;
    for (size_t i = 1; i < sizeof(size_t) * CHAR_BIT; i <<= 1) {
        n |= n >> i;
    }

    return n + 1;
}

static void insert(rhhashmap_t *map, char *key, void *value, const size_t hash) {
    rhhashmap_entry_t to_insert = {key, value, hash, 0};
    size_t i = hash & (map->cap - 1);

    while (true) {
        rhhashmap_entry_t *entry = &map->table[i];

        if (!entry->key) {
            *entry = to_insert;
            map->len++;
            return;
        }

        if (entry->hash == to_insert.hash && !strcmp(entry->key, to_insert.key)) {
            free(entry->key);
            entry->key = to_insert.key;
            entry->value = to_insert.value;
            return;
        }

        if (to_insert.psl > entry->psl) {
            const rhhashmap_entry_t tmp = *entry;
            *entry = to_insert;
            to_insert = tmp;
        }

        i = (i + 1) & (map->cap - 1);
        to_insert.psl++;
    }
}

static bool get_index(size_t *out, const rhhashmap_t *map, const char *key) {
    const size_t hash = (size_t) rapidhash(key, strlen(key));
    size_t i = hash & (map->cap - 1);
    size_t psl = 0;

    while (true) {
        const rhhashmap_entry_t *entry = &map->table[i];

        if (!entry->key || psl > entry->psl) {
            return false;
        }
        if (entry->hash == hash && !strcmp(entry->key, key)) {
            *out = i;
            return true;
        }

        i = (i + 1) & (map->cap - 1);
        psl++;
    }
}

static bool create(rhhashmap_t *map, const size_t cap) {
    rhhashmap_entry_t *table = calloc(cap, sizeof(rhhashmap_entry_t));
    if (!table) {
        return false;
    }

    map->table = table;
    map->len = 0;
    map->cap = cap;

    return true;
}

static bool resize(rhhashmap_t *map, const size_t cap) {
    if (cap <= map->cap) {
        return true;
    }

    rhhashmap_entry_t *new_table = calloc(cap, sizeof(rhhashmap_entry_t));
    if (!new_table) {
        return false;
    }

    rhhashmap_entry_t *old_table = map->table;
    const size_t old_cap = map->cap;
    map->table = new_table;
    map->len = 0;
    map->cap = cap;

    for (size_t i = 0; i < old_cap; i++) {
        if (old_table[i].key) {
            insert(map, old_table[i].key, old_table[i].value, old_table[i].hash);
        }
    }
    free(old_table);

    return true;
}

bool rhhashmap_create(rhhashmap_t *map) {
    return create(map, RHHASHMAP_MIN_CAPACITY);
}

bool rhhashmap_create_with_capacity(rhhashmap_t *map, size_t cap) {
    if (cap > RHHASHMAP_MAX_CAPACITY) {
        return false;
    }
    cap = cap < RHHASHMAP_MIN_CAPACITY ? RHHASHMAP_MIN_CAPACITY : next_pow2(cap);

    return create(map, cap);
}

void rhhashmap_destroy(const rhhashmap_t *map) {
    for (size_t i = 0; i < map->cap; i++) {
        free(map->table[i].key);
    }
    free(map->table);
}

bool rhhashmap_reserve(rhhashmap_t *map, size_t cap) {
    if (cap > RHHASHMAP_MAX_CAPACITY) {
        return false;
    }
    cap = cap < RHHASHMAP_MIN_CAPACITY ? RHHASHMAP_MIN_CAPACITY : next_pow2(cap);

    return resize(map, cap);
}

bool rhhashmap_insert(rhhashmap_t *map, const char *key, const void *value, const size_t value_size) {
    const size_t key_size = strlen(key) + 1;

    char *key_owned = malloc(key_size + value_size);
    if (!key_owned) {
        return false;
    }
    void *value_owned = key_owned + key_size;

    memcpy(key_owned, key, key_size);
    memcpy(value_owned, value, value_size);

    insert(map, key_owned, value_owned, (size_t) rapidhash(key_owned, key_size - 1));
    if ((double) map->len / (double) map->cap > RHHASHMAP_LOAD_FACTOR) {
        if (map->cap >= RHHASHMAP_MAX_CAPACITY || !resize(map, map->cap * 2)) {
            rhhashmap_remove(map, key);
            return false;
        }
    }

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

    while (true) {
        const size_t next_i = (i + 1) & (map->cap - 1);
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

void rhhashmap_clear(rhhashmap_t *map) {
    for (size_t i = 0; i < map->cap; i++) {
        free(map->table[i].key);
        map->table[i] = (rhhashmap_entry_t){0};
    }
    map->len = 0;
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
