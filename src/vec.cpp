#include <cassert>
#include <cstdint>
#include <stdfloat>

struct rtype {
    // clang-format off
    enum class t : uint8_t {
        a_gen   = 0, v_gen  = 0 | 128,
        a_sym   = 1, v_sym  = 1 | 128,
        a_nano  = 2, v_nano = 2 | 128,
        a_bool  = 3, v_bool = 3 | 128,
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
};
static_assert(sizeof(rtype) == 1);

using float16_t  = _Float16;
using bfloat16_t = float16_t;

struct sym {
    uint32_t s;
};

struct nano {
    uint64_t n;
};

struct rbool {
    uint8_t b;
};

struct atom {
    rtype   type;
    uint8_t pad_[7];

    union {
        void      *u_gen;
        sym        u_sym;
        nano       u_nano;
        rbool      u_rbool;
        uint8_t    u_char;
        int8_t     u_i8;
        int16_t    u_i16;
        int32_t    u_i32;
        int64_t    u_i64;
        float16_t  u_f16;
        float      u_f32;
        double     u_f64;
        bfloat16_t u_bf16;
    };
} __attribute((__packed__));
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
};
static_assert(sizeof(vattr) == 1);

struct vec {
    rtype    type;
    vattr    attr;
    uint8_t  pad1_[2];
    uint16_t ref;
    uint8_t  pad2_[2];
    uint64_t cap;
    uint64_t len;
    void    *data;
} __attribute((__packed__));
static_assert(sizeof(vec) == 32);

struct mattr {
    uint8_t i;

    static constexpr uint8_t trans = 0x01;
    static constexpr uint8_t diag  = 0x02;

    mattr(uint8_t x) : i(x) {}
    mattr(uint8_t t, uint8_t d) : i(d << 1 | d) { assert(t < 2 && d < 2); }
};

struct mat {
    rtype    type;
    mattr    attr;
    uint8_t  pad1_[2];
    uint16_t ref;
    uint8_t  pad2_[2];
    uint64_t xdim;
    uint64_t ydim;
    void    *data;
} __attribute__((__packed__));
static_assert(sizeof(mat) == 32);
