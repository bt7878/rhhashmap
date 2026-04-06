#include "../src/rhhashmap.h"

#include <cmocka.h>
#include <stdio.h>

static void test_rhhashmap_create_destroy(void **state) {
    (void) state;
    rhhashmap_t map;

    // Create a new map and verify initial state
    assert_true(rhhashmap_create(&map));
    assert_non_null(map.table);
    assert_int_equal(map.len, 0);
    assert_int_equal(map.cap, RHHASHMAP_MIN_CAPACITY);

    // Clean up
    rhhashmap_destroy(&map);
}

static void test_rhhashmap_insert_get_basic(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Basic insertion
    const int val1 = 42;
    assert_true(rhhashmap_insert_typed(&map, "key_1", val1));
    assert_int_equal(map.len, 1);

    // Basic retrieval
    int out_val = 0;
    assert_true(rhhashmap_get_typed(&out_val, &map, "key_1"));
    assert_int_equal(out_val, 42);

    // Retrieval of non-existent key
    assert_false(rhhashmap_get_typed(&out_val, &map, "key_nonexistent"));

    rhhashmap_destroy(&map);
}

static void test_rhhashmap_insert_update(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Insert first value
    const int val1 = 42;
    const int val2 = 84;
    assert_true(rhhashmap_insert_typed(&map, "key_1", val1));
    assert_int_equal(map.len, 1);

    // Update with second value
    assert_true(rhhashmap_insert_typed(&map, "key_1", val2));
    assert_int_equal(map.len, 1);

    // Verify update
    int out_val = 0;
    assert_true(rhhashmap_get_typed(&out_val, &map, "key_1"));
    assert_int_equal(out_val, 84);

    rhhashmap_destroy(&map);
}

static void test_rhhashmap_remove_basic(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Insert an element
    const int val = 123;
    rhhashmap_insert_typed(&map, "key_single", val);
    assert_int_equal(map.len, 1);

    // Remove it
    assert_true(rhhashmap_remove(&map, "key_single"));
    assert_int_equal(map.len, 0);

    // Verify it's gone
    int out_val;
    assert_false(rhhashmap_get_typed(&out_val, &map, "key_single"));

    // Attempt double removal
    assert_false(rhhashmap_remove(&map, "key_single"));

    rhhashmap_destroy(&map);
}

static void test_rhhashmap_resize(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));
    const size_t initial_cap = map.cap;

    // Insert enough elements to trigger a resize
    for (int i = 0; i < 15; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        assert_true(rhhashmap_insert_typed(&map, key, i));
    }

    assert_int_equal(map.len, 15);
    assert_true(map.cap > initial_cap);
    assert_int_equal(map.cap, initial_cap * 2);

    // Verify all elements are still there
    for (int i = 0; i < 15; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        int out_val;
        assert_true(rhhashmap_get_typed(&out_val, &map, key));
        assert_int_equal(out_val, i);
    }

    rhhashmap_destroy(&map);
}

static void test_rhhashmap_collisions_and_psl_swap(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Fill the map significantly to ensure probes
    for (int i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        assert_true(rhhashmap_insert_typed(&map, key, i));
    }

    // Remove from the middle to check backward shift
    assert_true(rhhashmap_remove(&map, "key_5"));
    assert_int_equal(map.len, 9);

    int out_val;
    assert_false(rhhashmap_get_typed(&out_val, &map, "key_5"));

    // Check others still there
    for (int i = 0; i < 10; i++) {
        if (i == 5) continue;
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        assert_true(rhhashmap_get_typed(&out_val, &map, key));
        assert_int_equal(out_val, i);
    }

    rhhashmap_destroy(&map);
}

static void test_rhhashmap_large_insert_remove(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Fill the map with many elements
    const int count = 1000;
    for (int i = 0; i < count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_large_%d", i);
        assert_true(rhhashmap_insert_typed(&map, key, i));
    }
    assert_int_equal(map.len, count);

    // Remove every other element
    for (int i = 0; i < count; i += 2) {
        char key[32];
        snprintf(key, sizeof(key), "key_large_%d", i);
        assert_true(rhhashmap_remove(&map, key));
    }
    assert_int_equal(map.len, count / 2);

    // Verify correct elements remain
    for (int i = 0; i < count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key_large_%d", i);
        int out_val;
        if (i % 2 == 0) {
            assert_false(rhhashmap_get_typed(&out_val, &map, key));
        } else {
            assert_true(rhhashmap_get_typed(&out_val, &map, key));
            assert_int_equal(out_val, i);
        }
    }

    rhhashmap_destroy(&map);
}

static bool foreach_callback_count(const char *key, const void *value, void *ctx) {
    (void) key;
    (void) value;
    int *count = ctx;
    (*count)++;
    return true;
}

static void test_rhhashmap_foreach_basic(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Verify empty map iteration
    int count = 0;
    rhhashmap_foreach(&map, foreach_callback_count, &count);
    assert_int_equal(count, 0);

    // Fill with some elements
    const int n = 50;
    for (int i = 0; i < n; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        rhhashmap_insert_typed(&map, key, i);
    }

    // Verify total count via foreach
    count = 0;
    rhhashmap_foreach(&map, foreach_callback_count, &count);
    assert_int_equal(count, n);

    rhhashmap_destroy(&map);
}

static bool foreach_callback_early_stop(const char *key, const void *value, void *ctx) {
    (void) key;
    (void) value;
    int *count = ctx;
    (*count)++;
    if (*count == 5) {
        return false;
    }
    return true;
}

static void test_rhhashmap_foreach_early_stop(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Fill with elements
    for (int i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        rhhashmap_insert_typed(&map, key, i);
    }

    // Verify early stop by callback
    int count = 0;
    rhhashmap_foreach(&map, foreach_callback_early_stop, &count);
    assert_int_equal(count, 5);

    rhhashmap_destroy(&map);
}

typedef struct {
    int count;
    bool seen[100];
} ForeachCtx;

static bool foreach_callback_verify(const char *key, const void *value, void *ctx) {
    ForeachCtx *fctx = ctx;
    fctx->count++;
    const int val = *(const int *) value;
    if (val >= 0 && val < 100) {
        fctx->seen[val] = true;
    }
    char expected_key[16];
    snprintf(expected_key, sizeof(expected_key), "key_%d", val);
    assert_string_equal(key, expected_key);
    return true;
}

static void test_rhhashmap_foreach_verify(void **state) {
    (void) state;
    rhhashmap_t map;
    assert_true(rhhashmap_create(&map));

    // Insert elements to verify later
    const int n = 20;
    for (int i = 0; i < n; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        rhhashmap_insert_typed(&map, key, i);
    }

    // Use callback to check all keys and values
    ForeachCtx fctx = {0, false};
    rhhashmap_foreach(&map, foreach_callback_verify, &fctx);
    assert_int_equal(fctx.count, n);
    for (int i = 0; i < n; i++) {
        assert_true(fctx.seen[i]);
    }

    rhhashmap_destroy(&map);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_rhhashmap_create_destroy),
        cmocka_unit_test(test_rhhashmap_insert_get_basic),
        cmocka_unit_test(test_rhhashmap_insert_update),
        cmocka_unit_test(test_rhhashmap_remove_basic),
        cmocka_unit_test(test_rhhashmap_resize),
        cmocka_unit_test(test_rhhashmap_collisions_and_psl_swap),
        cmocka_unit_test(test_rhhashmap_large_insert_remove),
        cmocka_unit_test(test_rhhashmap_foreach_basic),
        cmocka_unit_test(test_rhhashmap_foreach_early_stop),
        cmocka_unit_test(test_rhhashmap_foreach_verify),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
