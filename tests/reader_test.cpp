#include "reader.hpp"

#include "../other/ut/include/boost/ut.hpp"

int main() {
    using namespace boost::ut;
    using namespace asv;

    "int"_test = [] {
        node n;
        int  p = reader("123--").parse_number(0, n);
        expect(p == 3);
        expect(n.type == type::t_int);
        expect(n.i == 123);
    };

    "float"_test = [] {
        Node n;
        int  p = reader("12.5 ").parse_number(0, n);
        expect(p == 4);
        expect(n.type == type::t_float);
        expect(n.d == 12.5);
    };

    "str"_test = [] {
        Node n;
        int  p = reader("\"hi\"").parse_str(0, n);
        expect(p == 4);
        expect(n.type == type::t_str);
        expect(n.s->compare("hi") == 0);
    };

    "ident"_test = [] {
        Node n;
        int  p = reader("h11s+").parse_ident(0, n);
        expect(p == 4);
        expect(n.type == type::t_ident);
        expect(n.s->compare("h11s") == 0);
    };

    "sym"_test = [] {
        Node n;
        int  p = reader("`a1`a2").parse_sym(0, n);
        expect(p == 3);
        expect(n.type == type::t_sym);
        expect(n.s->compare("a1") == 0);
    };

    "oper"_test = [] {
        Node n;
        int  p = reader("+").parse_oper(0, n);
        expect(p == 1);
        expect(n.type == type::t_oper);
        expect(n.c == '#');
    };

    "expr"_test = [] {
        Node n;
        int  p = reader("1 #x").parse_expr(0, n);
        expect(p == 4);
        expect(n.type == type::t_expr);
        expect(n.n->size() == 3);
    };

    return 0;
}
