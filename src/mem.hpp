#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "runik.hpp"

// This is just to get going, will fill it later.

inline size_t chk_mul(size_t x, size_t y) {
    size_t r;
    if (__builtin_mul_overflow(x, y, &r)) assert(false);
    return r;
}
using namespace runik;

namespace mem {

struct header {
    // header(const header &)            = default;
    // header(header &&)                 = default;
    // header &operator=(const header &) = default;
    // header &operator=(header &&)      = default;
    union {
        header *next;
        vec     v;
        mat     m;
    } pack_align(32);
} pack_align(32);
static_assert(sizeof(header) == 32);

inline void *alloc_raw(size_t s) {
    assert(s % data_align == 0);
    return ::operator new(s);
}

inline void free_raw(void *v) { free(v); }

inline header *alloc_header() {
    header *h = (header *)alloc_raw(sizeof(header));
    std::memset(static_cast<void*>(h), 0, sizeof(header));
    return h;
}

inline void free_header(header *h) { ::operator delete((void *)h); }

inline void *alloc_data(size_t s) { return alloc_raw(s); }

inline vec *allocv(size_t n, size_t s) {
    uint64_t r = chk_mul(n, s);
    assert(r % block_size == 0);
    vec *v    = &alloc_header()->v;
    v->v_void = alloc_data(r);
    return v;
}

inline void freev(vec *v) {
    free_raw(v->v_void);
    free_raw(v);
}

};   // namespace mem
