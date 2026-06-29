#pragma once

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "runik.hpp"
#include "vec.hpp"

namespace asv {

using namespace runik;

constexpr unsigned no_parse = std::numeric_limits<unsigned>::max();

struct func;
struct reader;
struct node;

class parse_error : public std::runtime_error {
    std::string msg;

   public:
    unsigned pos;
    parse_error(std::string m, unsigned p);

    char const *what() const noexcept override { return msg.c_str(); }
};

enum class ntype {
    t_term,
    t_expr,
    t_parlist,
    t_bracketlist,
    t_bracelist,
    t_toplevel,
    t_number,
    t_vec,
    t_str,
    t_ident,
    t_oper,
    t_adverb,
    t_func,
    t_sym,
    t_comment,
};

struct node {
    ntype    type;
    unsigned pos;
    union {
        atom               a;
        vec               *v;
        std::string       *s;
        std::vector<node> *n;
        func              *f;
    };

    explicit node(enum ntype t, unsigned p, double x, rtype rt) : type(t), pos(p), a(rt, x) {}
    explicit node(enum ntype t, unsigned p, int64_t x, rtype rt) : type(t), pos(p), a(rt, x) {}
    explicit node(enum ntype t, unsigned p, double x) : type(t), pos(p), a(rtype::a_f64, x) {}
    explicit node(enum ntype t, unsigned p, int64_t x) : type(t), pos(p), a(rtype::a_i64, x) {}
    explicit node(enum ntype t, unsigned p, char x) : type(t), pos(p), a(rtype::a_char, x) {}
    explicit node(enum ntype t, unsigned p, vec *x) : type(t), pos(p), v(x) {}

    explicit node(enum ntype t, unsigned p, std::string *x) : type(t), pos(p), s(x) {}
    explicit node(enum ntype t, unsigned p, std::vector<node> *x) : type(t), pos(p), n(x) {}
    explicit node(enum ntype t, unsigned p, func *x) : type(t), pos(p), f(x) {}

    explicit node() : node(ntype::t_term, no_parse, '\0') {}

    node(const node &)            = delete;
    node &operator=(const node &) = delete;

    node(node &&other) noexcept {
        this->type = other.type;
        this->pos  = other.pos;
        this->a    = other.a;
        other.type = ntype::t_term;
    }

    node &operator=(node &&other) noexcept {
        if (this != &other) {
            std::swap(this->type, other.type);
            std::swap(this->pos, other.pos);
            std::swap(this->a, other.a);
        }
        return *this;
    }

    ~node();
};

struct func {
    node name;
    node params;
    node body;
    bool implicit;
};

struct reader {
    std::string_view sv;

    explicit reader(std::string_view s) : sv(s) {}
    explicit reader(char const *s) : sv(s) {}

    bool     has_more(unsigned p);
    unsigned skip_ws(unsigned p);
    unsigned match_next(unsigned pos, std::string_view x);
    unsigned parse_group(unsigned pos, node &n, asv::ntype type, char open, char close);

    unsigned parse_number(unsigned pos, node &n);
    unsigned parse_str(unsigned pos, node &n);
    unsigned parse_ident(unsigned pos, node &n);
    unsigned parse_sym(unsigned pos, node &n);
    unsigned parse_oper(unsigned pos, node &n);
    unsigned parse_adverb(unsigned pos, node &n);
    unsigned parse_term(unsigned pos, node &n);
    unsigned parse_func(unsigned pos, node &n);
    unsigned parse_funcparams(unsigned pos, node &n);
    unsigned parse_funcbody(unsigned pos, node &n);
    unsigned parse_parlist(unsigned pos, node &n);
    unsigned parse_bracelist(unsigned pos, node &n);
    unsigned parse_bracketlist(unsigned pos, node &n);
    unsigned parse_expr(unsigned pos, node &n);
    unsigned parse_toplevel(unsigned pos, node &n);
    unsigned parse_comment(unsigned pos, node &n);
};

std::ostream &operator<<(std::ostream &os, const asv::ntype t);
std::ostream &operator<<(std::ostream &os, const node &n);

void pretty_print(node &n, int level = 0, char const *prefix = "");

}   // namespace asv
