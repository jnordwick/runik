#include "boost/ut.hpp"
#include "reader.hpp"

int main() {
    using namespace boost::ut;
    using namespace asv;

    "int"_test = [] {
        node n;
        int  p = reader("123--").parse_number(0, n);
        expect(p == 3);
        expect(n.typ == type::t_int);
        expect(n.i == 123);
    };

    "float"_test = [] {
        node n;
        int  p = reader("12.5 ").parse_number(0, n);
        expect(p == 4);
        expect(n.typ == type::t_float);
        expect(n.d == 12.5);
    };

    "str"_test = [] {
        node n;
        int  p = reader("\"hi\"").parse_str(0, n);
        expect(p == 4);
        expect(n.typ == type::t_str);
        expect(n.s->compare("hi") == 0);
    };

    "ident"_test = [] {
        node n;
        int  p = reader("h11s+").parse_ident(0, n);
        expect(p == 4);
        expect(n.typ == type::t_ident);
        expect(n.s->compare("h11s") == 0);
    };

    "sym"_test = [] {
        node n;
        int  p = reader("`a1`a2").parse_sym(0, n);
        expect(p == 3);
        expect(n.typ == type::t_sym);
        expect(n.s->compare("a1") == 0);
    };

    "oper"_test = [] {
        node n;
        int  p = reader("+").parse_oper(0, n);
        expect(p == 1);
        expect(n.typ == type::t_oper);
        expect(n.c == '+');
    };

    "expr"_test = [] {
        node n;
        int  p = reader("1 +x").parse_expr(0, n);
        expect(p == 4);
        expect(n.typ == type::t_expr);
        expect(n.n->size() == 3);
    };

    return 0;
}
