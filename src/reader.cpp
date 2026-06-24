#include "reader.hpp"

#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace asv {

using std::vector;

parse_error::parse_error(std::string m, unsigned p) : runtime_error(m), pos(p) {
    msg  = "parse error:";
    msg += std::to_string(p);
    msg += ": ";
    msg += m;
}

constexpr std::array<bool, 256> oper_table = []() {
    constexpr char        ops[] = "`+-*%<>:";
    std::array<bool, 256> t{};
    for (auto c : ops)
        t[static_cast<unsigned>(c)] = true;
    t[0] = false;
    return t;
}();

constexpr std::array<char const *, 4> adverbs = {"':", "/", "\\", "'"};

node::~node() {
    using enum asv::type;

    switch (typ) {
    case t_ident:
    case t_str:
    case t_comment:
    case t_sym: delete s; break;
    case t_parlist:
    case t_bracketlist:
    case t_bracelist:
    case t_toplevel: delete n; break;
    case t_expr: delete n; break;
    case t_func: delete f; break;
    case t_term:
    case t_int:
    case t_float:
    case t_adverb:
    case t_oper: break;
    }
}

std::ostream &operator<<(std::ostream &os, const asv::type t) {
    using enum asv::type;

    switch (t) {
    case t_comment: os << "Comment"; break;
    case t_ident: os << "Ident"; break;
    case t_adverb: os << "Adverb"; break;
    case t_oper: os << "Oper"; break;
    case t_parlist: os << "ParList"; break;
    case t_bracketlist: os << "BracketList"; break;
    case t_bracelist: os << "BraceList"; break;
    case t_toplevel: os << "TopLevel"; break;
    case t_expr: os << "Expr"; break;
    case t_func: os << "Func"; break;
    case t_float: os << "Float"; break;
    case t_int: os << "Int"; break;
    case t_str: os << "Str"; break;
    case t_term: os << "Term"; break;
    case t_sym: os << "Sym"; break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const node &n) {
    using enum asv::type;

    os << "Node<" << n.typ << ">";
    os << "(" << n.pos << ")[";
    switch (n.typ) {
    case t_ident:
    case t_sym:
    case t_comment:
    case t_str: os << *n.s; break;
    case t_int: os << n.a.a_i64; break;
    case t_float: os << n.a.a_f32; break;
    case t_adverb: os << adverbs[n.a.a_char]; break;
    case t_parlist:
    case t_bracketlist:
    case t_bracelist:
    case t_toplevel:
    case t_expr: {
        os << '#' << n.n->size();
        break;
    }
    case t_func: {
        os << n.f->implicit;
        break;
    }
    case t_term:
    case t_oper: os << n.a.a_char; break;
    }
    os << "]";
    return os;
}

bool is_ws(char c) { return c == ' ' || c == '\t'; }
bool is_term(char c) { return c == ';' || c == '\n' || c == 0; }
bool is_leftgroup(char c) { return c == '(' || c == '[' || c == '{'; }
bool is_rightgroup(char c) { return c == '}' || c == ']' || c == ')'; }
bool is_num(char c) { return ((unsigned)c) - ((unsigned)'0') < 10; }
bool is_upper(char c) { return ((unsigned)c) - ((unsigned)'A') < 26; }
bool is_lower(char c) { return ((unsigned)c) - ((unsigned)'a') < 26; }
bool is_alpha(char c) { return is_upper(c) || is_lower(c); }
bool is_ident1char(char c) { return is_alpha(c) || c == '_' || c == '.'; }
bool is_identchar(char c) { return is_alpha(c) || is_num(c) || c == '_' || c == '.'; }
bool is_expr_left_boundary(char c) { return is_ws(c) || is_term(c) || is_leftgroup(c); }

bool inline reader::has_more(unsigned p) { return p < sv.length(); }

unsigned inline reader::skip_ws(unsigned p) {
    while (p < sv.size()) {
        if (is_ws(sv[p]))
            p += 1;
        else
            break;
    }
    return p;
}

unsigned reader::parse_number(unsigned pos, node &n) {
    unsigned p = pos;
    if (sv[p] == '-') {
        char prev = p > 0 ? sv[p - 1] : ' ';
        if (!is_expr_left_boundary(prev)) return no_parse;
        p += 1;
    }
    if (p >= sv.size() || !is_num(sv[p])) return no_parse;

    while (has_more(p) && (is_num(sv[p]) || sv[p] == '.'))
        p += 1;
    if (has_more(p) && is_alpha(sv[p])) {   // 123abc
        throw parse_error("bad number or ident", pos);
    }

    unsigned count_dots = 0;
    for (unsigned i = pos; i < p; ++i) {
        count_dots += sv[i] == '.';
    }
    if (count_dots > 1)   // 1.2.3
        throw parse_error("bad number format", pos);
    if (sv[p - 1] == '.')   // 12.
        throw parse_error("bad number format", pos);

    std::from_chars_result fcres;
    if (count_dots == 1) {
        double d;
        fcres = std::from_chars(sv.data() + pos, sv.data() + p, d);
        n     = node(asv::type::t_float, pos, d);
    }
    else {
        int64_t i;
        fcres = std::from_chars(sv.data() + pos, sv.data() + p, i);
        n     = node(asv::type::t_int, pos, i);
    }
    if (fcres.ec == std::errc::invalid_argument) {
        throw parse_error("bad number format", pos);
    }
    else if (fcres.ec == std::errc::result_out_of_range) {
        throw parse_error("number overflow", pos);
    }
    return p;
}

unsigned reader::parse_str(unsigned pos, node &n) {
    if (sv[pos] != '"') return no_parse;

    unsigned p = pos + 1;
    while (has_more(p)) {
        if (sv[p] == '"' && sv[p - 1] != '\\') {
            n = node(asv::type::t_str, pos, new std::string(sv, pos + 1, p - pos - 1));
            return p + 1;
        }
        p += 1;
    }
    throw parse_error("unterminated string", pos);
}

unsigned reader::parse_ident(unsigned pos, node &n) {
    unsigned p = pos;
    if (!is_ident1char(sv[p])) return no_parse;
    while (has_more(p) && is_identchar(sv[p]))
        p += 1;
    n = node(asv::type::t_ident, pos, new std::string(sv, pos, p - pos));
    return p;
}

unsigned reader::parse_comment(unsigned pos, node &n) {
    if (sv[pos] != '/') return no_parse;
    char prev = pos == 0 ? 0 : sv[pos - 1];
    if (!is_ws(prev) && !is_term(prev)) return no_parse;
    int p = pos + 1;
    while (has_more(p) && p != '\n')
        p += 1;
    n = node(asv::type::t_comment, pos, new std::string(sv, pos, p - pos));
    return p;
}

unsigned reader::parse_sym(unsigned pos, node &n) {
    if (sv[pos] != '`') return no_parse;
    unsigned p = pos + 1;

    if (!has_more(p)) return no_parse;

    if (sv[p] == '"')
        p = parse_str(p, n);
    else if (is_ident1char(sv[p]))
        p = parse_ident(p, n);
    else
        return no_parse;
    n.typ = asv::type::t_sym;
    return p;
}

unsigned reader::parse_oper(unsigned pos, node &n) {
    char c = sv[pos];
    if (!oper_table.at(static_cast<unsigned>(c))) return no_parse;
    n = node(asv::type::t_oper, pos, c);
    return pos + 1;
}

unsigned reader::parse_adverb(unsigned pos, node &n) {
    for (unsigned i = 0; i < adverbs.size(); ++i) {
        unsigned p = match_next(pos, adverbs[i]);
        if (p != no_parse) {
            n = node(asv::type::t_adverb, pos, static_cast<int64_t>(i));
            return p;
        }
    }
    return no_parse;
}

unsigned reader::parse_term(unsigned pos, node &n) {
    char c = pos >= sv.size() ? 0 : sv[pos];
    if (!(is_term(c) || is_rightgroup(c))) return no_parse;
    n = node(asv::type::t_term, pos, c);
    return pos + 1;
}

unsigned reader::parse_func(unsigned pos, node &n) {
    unsigned p = pos;
    p          = match_next(p, "fn");
    if (p == no_parse) return no_parse;

    func *fn = new func();

    p                = skip_ws(p);
    unsigned post_id = parse_ident(p, fn->name);
    // if no name, is anonymous function
    if (post_id != no_parse) p = post_id;

    p                = skip_ws(p);
    unsigned post_fp = parse_funcparams(p, fn->params);
    if (post_fp == no_parse)
        fn->implicit = true;
    else
        p = post_fp;

    p            = skip_ws(p);
    unsigned ret = parse_funcbody(p, fn->body);
    if (ret == no_parse) throw parse_error("expecting fn body", p);
    p = ret;

    n = node(asv::type::t_func, pos, fn);
    return p;
}

unsigned reader::parse_funcparams(unsigned pos, node &n) {
    using enum asv::type;

    vector<node> params;
    node         bn;
    unsigned     p = parse_bracketlist(pos, bn);
    if (p != no_parse) {
        for (unsigned i = 0; i < bn.n->size(); ++i) {
            switch (bn.n->at(i).n->size()) {
            case 0: throw parse_error("internal error: missing parse", -1);
            case 1: throw parse_error("missing parameter", bn.n->at(i).n->at(0).pos);
            case 2: params.emplace_back(std::move(bn.n->at(i).n->at(0))); break;
            default: throw parse_error("extra garbage in param list", bn.n->at(i).n->at(2).pos);
            }
        }
        for (unsigned i = 0; i < params.size(); ++i) {
            if (params[i].typ != t_ident && params[i].typ != t_term)
                throw parse_error("bad type in param list", params[i].pos);
        }
        n = node(t_bracketlist, pos, new vector<node>(std::move(params)));
    }
    return p;
}

unsigned reader::parse_funcbody(unsigned pos, node &n) { return parse_bracelist(pos, n); }

unsigned reader::match_next(unsigned pos, std::string_view x) {
    if (sv.size() - pos < x.size()) return no_parse;
    if (sv.substr(pos, x.size()) != x) return no_parse;
    unsigned end = pos + x.size();
    if (is_identchar(sv[end])) return no_parse;
    return end;
}

unsigned reader::parse_group(unsigned pos, node &n, asv::type type, char open, char close) {
    unsigned p = pos;
    if (sv[p] != open) return no_parse;
    p += 1;

    bool          done = false;
    vector<node> *list = new vector<node>();
    while (has_more(p) && !done) {
        node child;
        p = skip_ws(p);
        p = parse_expr(p, child);
        assert(child.n->back().typ == asv::type::t_term);
        char found = p == no_parse ? 0 : child.n->back().a.a_char;
        if (found == ';' || found == '\n' || found == close) {
            list->emplace_back(std::move(child));
            if (found == close) done = true;
        }
        else {
            std::string msg("expected '");
            msg += close;
            msg += "' but found '";
            msg += found ? std::string(1, found) : std::string("eof");
            msg += "'";
            throw parse_error(msg, pos);
        }
    }
    n = node(type, pos, list);
    return p;
}

unsigned reader::parse_parlist(unsigned pos, node &n) {
    return parse_group(pos, n, asv::type::t_parlist, '(', ')');
}
unsigned reader::parse_bracketlist(unsigned pos, node &n) {
    return parse_group(pos, n, asv::type::t_bracketlist, '[', ']');
}
unsigned reader::parse_bracelist(unsigned pos, node &n) {
    return parse_group(pos, n, asv::type::t_bracelist, '{', '}');
}

unsigned reader::parse_toplevel(unsigned pos, node &n) {
    unsigned      p     = skip_ws(pos);
    unsigned      start = p;
    vector<node> *v     = new vector<node>();

    bool done = false;
    while (has_more(p) && !done) {
        node     child;
        unsigned ret = no_parse;

        ret = parse_func(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_expr(p, child);
        if (ret != no_parse) goto bottom;
        break;

    bottom:
        v->emplace_back(std::move(child));
        p = ret;
        p = skip_ws(p);
    }
    n = node(asv::type::t_toplevel, start, v);
    return p;
}

unsigned reader::parse_expr(unsigned pos, node &n) {
    unsigned p = skip_ws(pos);
    if (!has_more(p)) return no_parse;

    unsigned      start  = p;
    vector<node> *v      = new vector<node>();
    bool          done   = false;
    type          last_t = type::t_term;

    while (has_more(p) && !done) {
        node     child;
        unsigned ret = no_parse;

        ret = parse_comment(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_term(p, child);
        if (ret != no_parse) {
            done = true;
            goto bottom;
        }
        ret = parse_parlist(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_bracketlist(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_bracelist(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_func(p, child);
        if (ret != no_parse) {
            if (child.f->name.typ == asv::type::t_ident)
                throw parse_error("Named functions only at top level", p);
            goto bottom;
        }
        ret = parse_sym(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_number(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_str(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_ident(p, child);
        if (ret != no_parse) goto bottom;
        if (last_t == type::t_oper || last_t == type::t_ident) {
            ret = parse_adverb(p, child);
            if (ret != no_parse) goto bottom;
        }
        ret = parse_oper(p, child);
        if (ret != no_parse) goto bottom;
        throw parse_error("unknown token", p);
    bottom:
        last_t = child.typ;
        v->emplace_back(std::move(child));
        p = ret;
        p = skip_ws(p);
    }
    n = node(asv::type::t_expr, start, v);
    return p;
}

void pretty_print(node &n, int level, char const *prefix) {
    using enum asv::type;

    for (int i = 0; i < level; ++i)
        std::cout << "  ";

    std::cout << prefix << n << std::endl;

    switch (n.typ) {
    case t_func: {
        pretty_print(n.f->name, level + 1, "name: ");
        pretty_print(n.f->params, level + 1, "params: ");
        pretty_print(n.f->body, level + 1, "body: ");
        break;
    }
    case t_expr:
    case t_parlist:
    case t_bracketlist:
    case t_bracelist:
    case t_toplevel: {
        for (unsigned i = 0; i < n.n->size(); ++i)
            pretty_print(n.n->at(i), level + 1);
        break;
    }
    default: break;
    }
}

}   // namespace asv
