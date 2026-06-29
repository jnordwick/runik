#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>   // IWYU pragma: keep
#include <stdfloat>

#define pack_align(x) __attribute__((__packed__, __aligned__(x)))
#define vec_align(x)  std::assume_aligned<data_align>(x)

namespace runik {

static constexpr size_t data_align = 32;
static constexpr size_t block_size = 32;

using float16_t  = _Float16;
using bfloat16_t = __bf16;

struct rune;
struct atom;
struct vec;

struct rtype {
    // clang-format off
    enum t : uint8_t {
        a_gen   = 0, v_gen  = 0 | 0x80,
        a_sym   = 1, v_sym  = 1 | 0x80,
        a_nano  = 2, v_nano = 2 | 0x80,
        a_bit   = 3, v_bit  = 3 | 0x80,
        a_char  = 4, v_char = 4 | 0x80,
        a_i8    = 5, v_i8   = 5 | 0x80,
        a_i16   = 6, v_i16  = 6 | 0x80,
        a_i32   = 7, v_i32  = 7 | 0x80,
        a_i64   = 8, v_i64  = 8 | 0x80,
        a_f16   = 9, v_f16  = 9 | 0x80,
        a_bf16  = 10, v_bf16 = 10 | 0x80,
        a_f32   = 11, v_f32  = 11 | 0x80,
        a_f64   = 12, v_f64  = 12 | 0x80,
    };
    // clang-format on

    rtype::t v;

    rtype() {}
    rtype(rtype::t x) : v(x) {}
    rtype(uint8_t x) : v(static_cast<rtype::t>(x)) {}
    uint32_t to_int() const { return static_cast<uint32_t>(v); }

    auto operator<=>(const rtype &x) const = default;

    rtype to_atom() const { return v & ~0xc0; }
    rtype to_vec() const { return to_atom().to_int() | 0x80; }
    rtype as_mat() const { return to_atom().to_int() | 0x40; }
    rtype as_ten() const { return to_int() | 0xc0; }
    bool  is_atom() const { return 0 == (to_int() & 0xc0); }
    bool  is_vec() const { return 0x80 == (to_int() & 0xc0); }
    bool  is_mat() const { return 0x40 == (to_int() & 0xc0); }
    bool  is_ten() const { return 0xc0 == (to_int() & 0xc0); }

    unsigned size_class() {
        switch (to_atom().to_int()) {
        case a_char:
        case a_i8:
        case a_bit: return 1;
        case a_bf16:
        case a_f16:
        case a_i16: return 2;
        case a_f32:
        case a_i32:
        case a_sym: return 4;
        case a_f64:
        case a_i64:
        case a_nano: return 8;
        case a_gen: return 16;
        default: assert(false);
        }
    }
} pack_align(1);
static_assert(sizeof(rtype) == 1);

inline size_t block_round_up(rtype t, size_t n) {
    size_t per_mask = block_size / t.size_class() - 1;
    return (per_mask + n) & ~per_mask;
}

inline size_t block_round_down(rtype t, size_t n) {
    size_t per_mask = block_size / t.size_class() - 1;
    return n & ~per_mask;
}

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
        char       data_[8];
        rune      *a_rune;
        sym        a_sym;
        nano       a_nano;
        bit        a_bit;
        int8_t     a_i8;
        int16_t    a_i16;
        int32_t    a_i32;
        int64_t    a_i64;
        uint8_t    a_char;
        uint8_t    a_u8;
        uint16_t   a_u16;
        uint32_t   a_u32;
        uint64_t   a_u64;
        bfloat16_t a_bf16;
        float16_t  a_f16;
        float      a_f32;
        double     a_f64;
    } pack_align(8);

    // sometimes atoms can have non-atomic values when in general lists
    // this so each atom only needs 16 bytes per entry, not sizeof(vec)
    atom(rtype t, rune *x) : type(t), a_rune(x) {}

    atom(rtype t, int64_t x) : type(t), a_i64(x) {}
    atom(rtype t, int32_t x) : atom(t, static_cast<int64_t>(x)) {}
    atom(rtype t, int16_t x) : atom(t, static_cast<int64_t>(x)) {}
    atom(rtype t, int8_t x) : atom(t, static_cast<int64_t>(x)) {}

    atom(rtype t, uint64_t x) : type(t), a_u64(x) {}
    atom(rtype t, uint32_t x) : atom(t, static_cast<uint64_t>(x)) {}
    atom(rtype t, uint16_t x) : atom(t, static_cast<uint64_t>(x)) {}
    atom(rtype t, uint8_t x) : atom(t, static_cast<uint64_t>(x)) {}

    atom(rtype t, double x) : atom(t, 0) { a_f64 = x; }
    atom(rtype t, float x) : atom(t, 0) { a_f32 = x; }
    atom(rtype t, float16_t x) : atom(t, 0) { a_f16 = x; }
    atom(rtype t, bfloat16_t x) : atom(t, 0) { a_bf16 = x; }

    atom(rtype t, sym x) : atom(t, x.to_int()) {}
    atom(rtype t, nano x) : atom(t, x.to_int()) {}
    atom(rtype t, bit x) : atom(t, x.to_int()) {}

} pack_align(8);
static_assert(sizeof(atom) == 16);

struct vattr {
    uint8_t i;

    static constexpr uint8_t none   = 0x00;
    static constexpr uint8_t unique = 0x01;
    static constexpr uint8_t sorted = 0x02;
    static constexpr uint8_t parted = 0x04;
    static constexpr uint8_t nulls  = 0x80;

    static constexpr uint8_t strict = unique | sorted | parted;
    static constexpr uint8_t mtonic = sorted | parted;

    vattr() {}
    vattr(uint8_t x) : i(x) {}
    vattr(uint8_t u, uint8_t s, uint8_t p, uint8_t n) : i(n << 7 | p << 2 | s << 1 | u) {
        assert(u < 2 && s < 2 && p < 2 && n < 2);
    }
} pack_align(1);
static_assert(sizeof(vattr) == 1);

struct vec;
void vec_unmake(vec *v);

struct rune {
    rtype    t;
    uint16_t ref;
    uint8_t  pad1_[1];

    rtype type() { return t; }

    atom *as_atom() {
        assert(t.is_atom());
        return reinterpret_cast<atom *>(this);
    }

    vec *as_vec() {
        assert(t.is_vec());
        return reinterpret_cast<vec *>(this);
    }

    rune *uref() {
        assert(t.is_vec());
        ref += 1;
        return this;
    }

    void dref() {
        assert(t.is_vec());
        if (ref > 0) {
            if (--ref == 0) vec_unmake(reinterpret_cast<vec *>(this));
        }
    }
};

struct mattr {
    uint8_t i;

    static constexpr uint8_t trans = 0x01;
    static constexpr uint8_t diag  = 0x02;

    mattr(uint8_t x) : i(x) {}
    mattr(uint8_t t, uint8_t d) : i(d << 1 | d) { assert(t < 2 && d < 2); }
} pack_align(1);

struct mat {
    rtype    type;
    uint16_t ref;
    uint8_t  pad1_[1];
    mattr    attr;
    uint8_t  pad2_[3];
    uint64_t xdim;
    uint64_t ydim;
    union {
        void       *data;
        float16_t  *v_16f;
        bfloat16_t *v_b16f;
        float      *v_32f;
        double     *v_64f;
    } pack_align(8);
} pack_align(8);
static_assert(sizeof(mat) == 32);

template <typename F>
static auto with_numeric_type(rtype t, F &&f) {
    switch (t.v) {
    case rtype::a_i8: return f(std::type_identity<int8_t>{});
    case rtype::a_i16: return f(std::type_identity<int16_t>{});
    case rtype::a_i32: return f(std::type_identity<int32_t>{});
    case rtype::a_i64: return f(std::type_identity<int64_t>{});
    case rtype::a_f16: return f(std::type_identity<float16_t>{});
    case rtype::a_bf16: return f(std::type_identity<bfloat16_t>{});
    case rtype::a_f32: return f(std::type_identity<float>{});
    case rtype::a_f64: return f(std::type_identity<double>{});
    default:
        std::cerr << "type=" << t.v << std::endl;
        assert(false);
        __builtin_unreachable();
    }
}

};   // namespace runik
