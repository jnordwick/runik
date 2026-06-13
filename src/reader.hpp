#pragma once

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace asv {

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

enum class type {
    t_term,
    t_expr,
    t_parlist,
    t_bracketlist,
    t_bracelist,
    t_toplevel,
    t_int,
    t_float,
    t_str,
    t_ident,
    t_oper,
    t_func,
    t_sym,
};

struct node {
    type     typ;
    unsigned pos;
    union {
        double             d;
        int64_t            i;
        std::string       *s;
        std::vector<node> *n;
        func              *f;
        char               c;
    };

    explicit node() : typ(type::t_term), pos(no_parse), c(0) {};
    explicit node(enum type t, unsigned p, double x) : typ(t), pos(p), d(x) {}
    explicit node(enum type t, unsigned p, int64_t x) : typ(t), pos(p), i(x) {}
    explicit node(enum type t, unsigned p, std::string *x) : typ(t), pos(p), s(x) {}
    explicit node(enum type t, unsigned p, std::vector<node> *x) : typ(t), pos(p), n(x) {}
    explicit node(enum type t, unsigned p, func *x) : typ(t), pos(p), f(x) {}
    explicit node(enum type t, unsigned p, char x) : typ(t), pos(p), c(x) {}

    node(const node &)            = delete;
    node &operator=(const node &) = delete;

    node(node &&other) noexcept {
        this->typ = other.typ;
        this->pos = other.pos;
        this->f   = other.f;
        other.typ = type::t_term;
        other.f   = nullptr;
    }

    node &operator=(node &&other) noexcept {
        if (this != &other) {
            std::swap(this->typ, other.typ);
            std::swap(this->pos, other.pos);
            std::swap(this->f, other.f);
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
    unsigned parse_group(unsigned pos, node &n, asv::type type, char open, char close);

    unsigned parse_number(unsigned pos, node &n);
    unsigned parse_str(unsigned pos, node &n);
    unsigned parse_ident(unsigned pos, node &n);
    unsigned parse_sym(unsigned pos, node &n);
    unsigned parse_oper(unsigned pos, node &n);
    unsigned parse_term(unsigned pos, node &n);
    unsigned parse_func(unsigned pos, node &n);
    unsigned parse_funcparams(unsigned pos, node &n);
    unsigned parse_funcbody(unsigned pos, node &n);
    unsigned parse_parlist(unsigned pos, node &n);
    unsigned parse_bracelist(unsigned pos, node &n);
    unsigned parse_bracketlist(unsigned pos, node &n);
    unsigned parse_expr(unsigned pos, node &n);
    unsigned parse_toplevel(unsigned pos, node &n);
};

std::ostream &operator<<(std::ostream &os, const asv::type t);
std::ostream &operator<<(std::ostream &os, const node &n);

void pretty_print(node &n, int level = 0, char const *prefix = "");

}   // namespace asv
