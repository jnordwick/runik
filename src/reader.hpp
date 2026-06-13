#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace asv {

constexpr int no_parse = -1;

struct func;
struct reader;
struct node;

class parse_error : public std::runtime_error {
    std::string msg;

   public:
    int pos;
    parse_error(std::string m, int p);

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
    type type;
    int  pos;
    union {
        double             d;
        int64_t            i;
        std::string       *s;
        std::vector<node> *n;
        func              *f;
        char               c;
    };

    explicit node() : type(type::t_term), pos(no_parse), c(0) {};
    explicit node(enum type t, int p, double x) : type(t), pos(p), d(x) {}
    explicit node(enum type t, int p, int64_t x) : type(t), pos(p), i(x) {}
    explicit node(enum type t, int p, std::string *x) : type(t), pos(p), s(x) {}
    explicit node(enum type t, int p, std::vector<node> *x) : type(t), pos(p), n(x) {}
    explicit node(enum type t, int p, func *x) : type(t), pos(p), f(x) {}
    explicit node(enum type t, int p, char x) : type(t), pos(p), c(x) {}

    node(const node &)            = delete;
    node &operator=(const node &) = delete;

    node(node &&other) noexcept {
        this->type = other.type;
        this->pos  = other.pos;
        this->f    = other.f;
        other.type = type::t_term;
        other.f    = nullptr;
    }

    node &operator=(node &&other) noexcept {
        if (this != &other) {
            std::swap(this->type, other.type);
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

    bool has_more(int p);
    int  skip_ws(int p);
    int  match_next(int pos, std::string_view x);
    int  parse_group(int pos, node &n, asv::type type, char open, char close);

    int parse_number(int pos, node &n);
    int parse_str(int pos, node &n);
    int parse_ident(int pos, node &n);
    int parse_sym(int pos, node &n);
    int parse_oper(int pos, node &n);
    int parse_term(int pos, node &n);
    int parse_func(int pos, node &n);
    int parse_funcparams(int pos, node &n);
    int parse_funcbody(int pos, node &n);
    int parse_parlist(int pos, node &n);
    int parse_bracelist(int pos, node &n);
    int parse_bracketlist(int pos, node &n);
    int parse_expr(int pos, node &n);
    int parse_toplevel(int pos, node &n);
};

std::ostream &operator<<(std::ostream &os, const asv::type t);
std::ostream &operator<<(std::ostream &os, const node &n);

}   // namespace asv
