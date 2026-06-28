#include "boost/ut.hpp"
#include "runik.hpp"

int main() {
    using namespace boost::ut;
    using namespace runik;

    test("vec") = [] {
        vec *v = vec::make(rtype::v_i32, 16);
        int32_t val = 10;
        expect(v->len == 0);
        expect(v->cap == 16);
        v->append(&val);
        expect(v->len == 1);
        expect(v->cap == 16);
        int32_t arr[] = {11, 12};
        v->append(arr, 2);
        expect(v->len == 3);
        expect(v->cap == 16);
        expect(v->get<int32_t>(0) == 10);
        expect(v->get<int32_t>(1) == 11);
        expect(v->get<int32_t>(2) == 12);
    };
}
