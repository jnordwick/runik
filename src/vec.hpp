#pragma once

#include "runik.hpp"

namespace runik {

struct vec {
    rtype    type;
    uint16_t ref;
    uint8_t  pad1_[1];
    vattr    attr;
    uint8_t  pad2_[3];
    uint64_t cap;
    uint64_t len;
    union {
        void       *v_void;
        atom       *v_atom;
        sym        *v_sym;
        nano       *v_nano;
        bit        *v_bit;
        int8_t     *a_i8;
        int16_t    *v_i16;
        int32_t    *v_i32;
        int64_t    *v_i64;
        uint8_t    *v_char;
        uint8_t    *v_u8;
        uint16_t   *v_u16;
        uint32_t   *v_u32;
        uint64_t   *v_u64;
        bfloat16_t *v_bf16;
        float16_t  *v_f16;
        float      *v_f32;
        double     *v_f64;
    } pack_align(8);

    template <typename T>
    T &get(uint64_t x = 0) {
        return static_cast<T *>(v_void)[x];
    }

    template <typename T>
    T const &get(uint64_t x = 0) const {
        return static_cast<T const *>(v_void)[x];
    }

    char *head() { return static_cast<char *>(v_void); }

    char *tail() { return head() + len * type.size_class(); }

    static vec *make(rtype t, uint64_t n, uint32_t ref = 0);
    static vec *make(vec *old, uint64_t extra);
    static vec *from(rtype t, void *v, size_t nmem);

    void ensure(uint64_t x) {
        if (x > cap) grow(x);
    }
    void more(uint64_t x) { ensure(len + x); }

    vec *uref() {
        ref += 1;
        return this;
    }

    void dref() {
        if (ref > 0) {
            if (--ref == 0) vec_unmake(this);
        }
    }

    void append(void *dat, uint64_t s = 1) {
        more(s);
        std::memcpy(tail(), dat, s * type.size_class());
        len += s;
    }

   private:
    __attribute__((__cold__)) void grow(uint64_t new_cap);
    vec() {}

} pack_align(8);
static_assert(sizeof(vec) == 32);

std::ostream &operator<<(std::ostream &os, vec const &v);

void vec_unmake(vec *v);

};   // namespace runik
