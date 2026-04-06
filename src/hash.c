#include "hash.h"

#include <rapidhash.h>

size_t hash(const char *s, const size_t cap) {
    const size_t h = (size_t) rapidhash(s, strlen(s));
    return h % cap;
}
