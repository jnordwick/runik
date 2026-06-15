#pragma once

#include <cassert>
#include <cstdlib>

#include "runik.hpp"

// This is just to get going, will fill it later.

template <typename T>
inline T chk_mul(T x, T y) {
    int r;
    if (__builtin_mul_overflow(x, y, &r)) assert(false);
    return r;
}

namespace mem {

using namespace runik;

struct header {
    header(const header &)            = default;
    header(header &&)                 = default;
    header &operator=(const header &) = default;
    header &operator=(header &&)      = default;
    union {
        header *next;
        vec     v;
        mat     m;
    } __attribute__((__packed__));
} __attribute__((__packed__));
static_assert(sizeof(header) == 32);

inline void *alloc_raw(size_t s) {
    assert(s % data_align == 0);
    return ::operator new(s);
}

inline void free_raw(void *v) { free(v); }

inline header *alloc_header() { return (header *)alloc_raw(sizeof(header)); }

inline void free_header(header *h) { ::operator delete((void *)h); }

template <typename T>
inline void *alloc_data(size_t n) {
    size_t r = chk_mul(n, sizeof(T));
    return alloc_raw(r);
}

template <typename T>
inline vec *allocv(size_t n) {
    uint64_t r = chk_mul(n, sizeof(T));
    vec     *v = &alloc_header()->v;
    v->data    = alloc_data<T>(n);
    v->cap     = r;
    v->len     = n;
    return v;
}

inline void freev(vec *v) {
    free_raw(v->data);
    free_raw(v);
}

};   // namespace mem
