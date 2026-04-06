#pragma once

#include <stdbool.h>
#include <stddef.h>

#define RHHASHMAP_MIN_CAPACITY 16
#define RHHASHMAP_LOAD_FACTOR 0.875

#define rhhashmap_insert_typed(map, key, value) \
    rhhashmap_insert(map, key, &value, sizeof(value))

#define rhhashmap_get_typed(out, map, key) \
    rhhashmap_get(out, sizeof(*(out)), map, key)

typedef struct rhhashmap_entry_t {
    char *key;
    void *value;
    int psl;
} rhhashmap_entry_t;

typedef struct rhhashmap_t {
    rhhashmap_entry_t *table;
    size_t len;
    size_t cap;
} rhhashmap_t;

typedef bool (*rhhashmap_callback_t)(const char *key, const void *value, void *ctx);

bool rhhashmap_create(rhhashmap_t *map);

void rhhashmap_destroy(const rhhashmap_t *map);

bool rhhashmap_insert(rhhashmap_t *map, const char *key, const void *value, size_t value_size);

bool rhhashmap_get(void *out, size_t out_size, const rhhashmap_t *map, const char *key);

bool rhhashmap_remove(rhhashmap_t *map, const char *key);

void rhhashmap_foreach(const rhhashmap_t *map, rhhashmap_callback_t callback, void *ctx);
