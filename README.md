# rhhashmap

`rhhashmap` is a C library that implements a high-performance hash map using **Robin Hood hashing**. It is designed for
string keys and generic values, providing efficient insertion, retrieval, and removal operations.

## Features

- **Robin Hood Hashing**: Minimizes variance in probe sequence lengths (PSL) for consistent lookup performance.
- **Fast Hashing**: Uses [rapidhash](https://github.com/Nicoshev/rapidhash) for extremely fast and high-quality string
  hashing.
- **Generic Values**: Supports values of any size by storing copies of provided data.
- **Dynamic Resizing**: Automatically resizes the table when the load factor (default 0.875) is exceeded.
- **Manual Capacity Control**: Pre-allocate memory using `rhhashmap_create_with_capacity` or `rhhashmap_reserve` to
  minimize reallocations.
- **Typed Macros**: Helper macros for simplified insertion and retrieval of typed values.
- **Comprehensive API**: Includes insert, retrival, removal, clearing, and iteration functions.

## Building and Installation

This project uses CMake (version 3.28 or later).

### Prerequisites

- A C17-compliant compiler (e.g., GCC, Clang, MSVC).
- CMake 3.28+.

### Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/bt7878/rhhashmap.git
   cd rhhashmap
   ```

2. Generate the build files and build the project:
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```

3. (Optional) Run tests:
   ```bash
   ctest --output-on-failure
   ```

## Usage Example

```c
#include "rhhashmap.h"
#include <stdio.h>

int main() {
    rhhashmap_t map;
    if (!rhhashmap_create(&map)) {
        return 1;
    }

    // Insert data using typed macro
    int value = 42;
    rhhashmap_insert_typed(&map, "my_key", value);

    // Retrieve data using typed macro
    int retrieved;
    if (rhhashmap_get_typed(&retrieved, &map, "my_key")) {
        printf("Found value: %d\n", retrieved);
    }

    // Iterate over all entries
    rhhashmap_foreach(&map, my_callback, NULL);

    // Clean up
    rhhashmap_destroy(&map);
    return 0;
}
```

## API Reference

### Initialization & Cleanup

- `bool rhhashmap_create(rhhashmap_t *map)`: Initializes a new hash map.
- `bool rhhashmap_create_with_capacity(rhhashmap_t *map, size_t cap)`: Initializes a hash map with at least `cap`
  capacity.
- `void rhhashmap_destroy(const rhhashmap_t *map)`: Frees all resources associated with the map.

### Operations

- `bool rhhashmap_reserve(rhhashmap_t *map, size_t cap)`: Reserves at least `cap` capacity.
- `bool rhhashmap_insert(rhhashmap_t *map, const char *key, const void *value, size_t value_size)`: Inserts or updates a
  value.
- `bool rhhashmap_get(void *out, size_t out_size, const rhhashmap_t *map, const char *key)`: Retrieves a value by key.
- `bool rhhashmap_remove(rhhashmap_t *map, const char *key)`: Removes a key-value pair.
- `void rhhashmap_clear(rhhashmap_t *map)`: Removes all entries from the map.
- `void rhhashmap_foreach(const rhhashmap_t *map, rhhashmap_callback_t callback, void *ctx)`: Iterates over all entries
  with optional user context.

### Helper Macros

- `rhhashmap_insert_typed(map, key, value)`: Wrapper for `rhhashmap_insert` that handles `sizeof(value)` automatically.
- `rhhashmap_get_typed(out, map, key)`: Wrapper for `rhhashmap_get` that handles `sizeof(*(out))` automatically.

## Dependencies

The project automatically fetches its dependencies via `FetchContent`:

- [rapidhash](https://github.com/Nicoshev/rapidhash): For fast string hashing.
- [cmocka](https://cmocka.org/): For unit testing (only when `BUILD_TESTING` is enabled).
