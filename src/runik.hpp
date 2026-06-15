#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdfloat>

#define pack_align(x) __attribute__((__packed__, __aligned__(x)))
#define balign(x)     std::assume_aligned<data_align>(x)

namespace runik {
static constexpr size_t data_align = 64;

struct rtype {
    // clang-format off
    enum t : uint8_t {
        a_gen   = 0, v_gen  = 0 | 128,
        a_sym   = 1, v_sym  = 1 | 128,
        a_nano  = 2, v_nano = 2 | 128,
        a_bit   = 3, v_bit  = 3 | 128,
        a_char  = 4, v_char = 4 | 128,
        a_i8    = 5, v_i18  = 5 | 128,
        a_i16   = 6, v_i16  = 6 | 128,
        a_i32   = 7, v_i32  = 7 | 128,
        a_i64   = 8, v_i64  = 8 | 128,
        a_f16   = 9, v_f16  = 9 | 128,
        a_f32   = 10, v_f32  = 10 | 128,
        a_f64   = 11, v_f64  = 11 | 128,
        a_bf16  = 12, v_bf16 = 12 | 128
    };
    // clang-format on

    rtype::t v;

    rtype(rtype::t x) : v(x) {}
    uint32_t to_int() { return static_cast<uint32_t>(v); }
} pack_align(1);
static_assert(sizeof(rtype) == 1);

using float16_t  = _Float16;
using bfloat16_t = __bf16;

struct sym {
    uint32_t s;
    uint32_t to_int() { return s; }
};

struct nano {
    uint64_t n;
    uint64_t to_int() { return n; }
};

struct bit {
    uint8_t b;
    uint8_t to_int() { return b; }
};

struct atom {
    rtype   type;
    uint8_t pad_[7] = {};

    union {
        void      *u_gen;
        sym        u_sym;
        nano       u_nano;
        bit        u_bit;
        int8_t     u_i8;
        int16_t    u_i16;
        int32_t    u_i32;
        int64_t    u_i64;
        uint8_t    u_char;
        uint8_t    u_u8;
        uint16_t   u_u16;
        uint32_t   u_u32;
        uint64_t   u_u64;
        bfloat16_t u_bf16;
        float16_t  u_f16;
        float      u_f32;
        double     u_f64;
    } pack_align(8);

    atom(rtype t, void *x) : type(t), u_gen(x) {}

    atom(rtype t, int64_t x) : type(t), u_i64(x) {}
    atom(rtype t, int32_t x) : atom(t, static_cast<int64_t>(x)) {}
    atom(rtype t, int16_t x) : atom(t, static_cast<int64_t>(x)) {}
    atom(rtype t, int8_t x) : atom(t, static_cast<int64_t>(x)) {}

    atom(rtype t, uint64_t x) : type(t), u_u64(x) {}
    atom(rtype t, uint32_t x) : atom(t, static_cast<uint64_t>(x)) {}
    atom(rtype t, uint16_t x) : atom(t, static_cast<uint64_t>(x)) {}
    atom(rtype t, uint8_t x) : atom(t, static_cast<uint64_t>(x)) {}

    atom(rtype t, double x) : atom(t, 0) { u_f64 = x; }
    atom(rtype t, float x) : atom(t, 0) { u_f32 = x; }
    atom(rtype t, float16_t x) : atom(t, 0) { u_f16 = x; }
    atom(rtype t, bfloat16_t x) : atom(t, 0) { u_bf16 = x; }

    atom(rtype t, sym x) : atom(t, x.to_int()) {}
    atom(rtype t, nano x) : atom(t, x.to_int()) {}
    atom(rtype t, bit x) : atom(t, x.to_int()) {}

} pack_align(8);
static_assert(sizeof(atom) == 16);

struct vattr {
    uint8_t i;

    static constexpr uint8_t unique = 0x01;
    static constexpr uint8_t sorted = 0x02;
    static constexpr uint8_t parted = 0x04;
    static constexpr uint8_t nulls  = 0x80;

    static constexpr uint8_t strict = unique | sorted | parted;
    static constexpr uint8_t mtonic = sorted | parted;

    vattr(uint8_t x) : i(x) {}
    vattr(uint8_t u, uint8_t s, uint8_t p, uint8_t n) : i(n << 7 | p << 2 | s << 1 | u) {
        assert(u < 2 && s < 2 && p < 2 && n < 2);
    }
} pack_align(1);
static_assert(sizeof(vattr) == 1);

struct vec {
    rtype    type;
    vattr    attr;
    uint8_t  pad1_[2] = {};
    uint16_t ref;
    uint8_t  pad2_[2] = {};
    uint64_t cap;
    uint64_t len;
    void    *data;
} pack_align(8);
static_assert(sizeof(vec) == 32);

struct mattr {
    uint8_t i;

    static constexpr uint8_t trans = 0x01;
    static constexpr uint8_t diag  = 0x02;

    mattr(uint8_t x) : i(x) {}
    mattr(uint8_t t, uint8_t d) : i(d << 1 | d) { assert(t < 2 && d < 2); }
} pack_align(1);

struct mat {
    rtype    type;
    mattr    attr;
    uint8_t  pad1_[2];
    uint16_t ref;
    uint8_t  pad2_[2];
    uint64_t xdim;
    uint64_t ydim;
    void    *data;
} pack_align(8);
static_assert(sizeof(mat) == 32);

};   // namespace runik
