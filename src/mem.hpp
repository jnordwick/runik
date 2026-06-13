#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "r.hpp"

// This is just to get going, will fill it later.

struct Header {
    union {
        Header *next;
        Vec v;
        Mat m;
    };
};

struct Alloc {
    static constexpr size_t initial_header_pool = 1000;
    static constexpr size_t data_align = 64;

    Alloc() {
        const size_t sz = initial_header_pool * sizeof(Header);
        assert(sz % data_align == 0);
        header_alloc = ::operator new[](sz);
        memset(header_alloc, 0, sz);
        header_pool = thread(header_alloc, initial_header_pool);
    }

    ~Alloc() { ::operator delete[](header_alloc); }

    Header *alloc_header() {
        if (header_pool == nullptr) {
            std::cerr << "oom\n";
            std::abort();
        }

        Header *h = header_pool;
        header_pool = h->next;
        return h;
    }

    void free_header(Header *h) {
        h->next = header_pool;
        header_pool = h;
    }

    template <typename T> T *alloc_data(size_t sz) {
        return static_cast<T *>(alloc_data_(sizeof(T), sz));
    }

    void *alloc_data_(size_t nmem, size_t sz) {
        return std::aligned_alloc(data_align, nmem * sz);
    }

    void free_data(void *d) { std::free(d); }

    Header *thread(void *mem, int n) {
        Header *last = nullptr;
        Header *h = static_cast<Header *>(mem);
        for (int i = 0; i < n; ++i) {
            h[i].next = last;
            last = h + i;
        }
        return last;
    }

    void *header_alloc;
    Header *header_pool;
};
