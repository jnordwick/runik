#include "vec.hpp"

#include <cstring>

#include "mem.hpp"
#include "runik.hpp"

namespace runik {

std::ostream &operator<<(std::ostream &os, vec const &v) {
    for (unsigned i = 0; i < v.len; ++i) {
        switch (v.type.v) {
        case rtype::v_i8: os << v.get<int8_t>(i) << " "; break;
        case rtype::v_i16: os << v.get<int16_t>(i) << " "; break;
        case rtype::v_i32: os << v.get<int32_t>(i) << " "; break;
        case rtype::v_i64: os << v.get<int64_t>(i) << " "; break;
        case rtype::v_f16: os << static_cast<float>(v.get<float16_t>(i)) << " "; break;
        case rtype::v_bf16: os << static_cast<float>(v.get<bfloat16_t>(i)) << " "; break;
        case rtype::v_f32: os << v.get<float>(i) << " "; break;
        case rtype::v_f64: os << v.get<double>(i) << " "; break;
        default: assert(false);
        }
    }
    return os;
}

vec *vec::make(rtype t, uint64_t n, uint32_t ref) {
    assert(t.is_vec());
    assert(n > 0);
    uint64_t u = block_round_up(t, n);
    vec     *v = mem::allocv(u, t.size_class());
    v->type    = t;
    v->attr    = vattr::none;
    v->ref     = ref;
    v->cap     = u;
    v->len     = 0;
    return v;
}

vec *vec::make(vec *old, uint64_t extra) {
    vec *v = make(old->type, old->len + extra);
    std::memcpy(v->v_void, old->v_void, old->len * old->type.size_class());
    v->attr = old->attr;
    return v;
}

void vec_unmake(vec *v) {
    if (v->type == rtype::v_gen) {
        for (uint64_t i = 0; i < v->len; ++i) {
            atom *a = v->v_atom;
            if (a->type == rtype::a_gen) a->a_rune->dref();
        }
    }
}

__attribute__((__cold__)) void vec::grow(uint64_t new_cap) {
    assert(ref < 2);
    assert(new_cap > cap);
    assert(cap >= block_size);

    // XXX fix overflow
    uint64_t c = cap;
    while (c < new_cap)
        c = (c / 2) * 3;
    new_cap = block_round_up(type, c);
    void *d = mem::alloc_data(new_cap);
    std::memcpy(d, v_void, len * type.size_class());
    mem::free_raw(v_void);
    v_void = d;
    cap    = new_cap;
}

vec *vec::from(rtype t, void *p, size_t nmem) {
    vec *v = make(t, nmem);
    std::memcpy(v->v_void, p, nmem * v->type.size_class());
    v->len  = nmem;
    v->attr = nmem == 1 ? vattr::strict : vattr::none;
    return v;
}

}   // namespace runik
