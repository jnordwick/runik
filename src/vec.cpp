#include "mem.hpp"
#include "runik.hpp"

namespace runik {

vec *vec::vec_make(rtype t, uint64_t n, uint32_t r) {
    assert(t.is_vec());
    assert(n > 0);
    uint64_t u = block_round_up(t, n);
    vec     *v = mem::allocv(u, t.size_class());
    v->type    = t;
    v->attr    = vattr::none;
    v->ref     = r;
    v->cap     = u;
    v->len     = n;
    return v;
}

void vec::vec_free(vec *v) {
    if (v->type == rtype::v_gen) {
        for (uint64_t i = 0; i < v->len; ++i) {
            atom &a = v->get<atom>(i);
            if (a.type == rtype::a_gen) a.u_rune->dref();
        }
    }
}

}   // namespace runik
