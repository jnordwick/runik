#include "reader.hpp"

#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "runik.hpp"
#include "vec.hpp"

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

constexpr std::array<std::string_view, 4> adverbs = {"':", "/", "\\", "'"};

node::~node() {
    using enum asv::ntype;

    switch (type) {
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
    case t_vec: v->dref(); break;
    case t_term:
    case t_number:
    case t_adverb:
    case t_oper: break;
    }
}

std::ostream &operator<<(std::ostream &os, const asv::ntype t) {
    using enum asv::ntype;

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
    case t_number: os << "Number"; break;
    case t_str: os << "Str"; break;
    case t_term: os << "Term"; break;
    case t_sym: os << "Sym"; break;
    case t_vec: os << "Vec"; break;
    }
    return os;
}

static void print_tnumber(std::ostream &os, const node &n) {
    assert(n.type == asv::ntype::t_number);
    switch (n.a.type.v) {
    case rtype::a_i8: os << n.a.a_i8; break;
    case rtype::a_i16: os << n.a.a_i16; break;
    case rtype::a_i32: os << n.a.a_i32; break;
    case rtype::a_i64: os << n.a.a_i64; break;
    case rtype::a_f16: os << static_cast<float>(n.a.a_f16); break;
    case rtype::a_bf16: os << static_cast<float>(n.a.a_bf16); break;
    case rtype::a_f32: os << n.a.a_f32; break;
    case rtype::a_f64: os << n.a.a_f64; break;
    default: assert(false);
    }
}

std::ostream &operator<<(std::ostream &os, const node &n) {
    using enum asv::ntype;

    os << "Node<" << n.type << ">";
    os << "(" << n.pos << ")[";
    switch (n.type) {
    case t_ident:
    case t_sym:
    case t_comment:
    case t_str: os << *n.s; break;
    case t_number: print_tnumber(os, n); break;
    case t_adverb: os << adverbs[n.a.a_char]; break;
    case t_parlist:
    case t_bracketlist:
    case t_bracelist:
    case t_toplevel:
    case t_expr: {
        os << '#' << n.n->size();
        break;
    }
    case t_vec: {
        os << *n.v;
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
    assert(pos < sv.size());

    unsigned     p         = pos;
    int          dot_count = 0;
    vector<char> clean;

    if (sv[p] == '-') {
        clean.push_back('-');
        p += 1;
    }

    if (p >= sv.size() || !is_num(sv[p])) return no_parse;

    while (p < sv.size()) {
        char c = sv[p];
        if (is_num(c)) {
            clean.push_back(c);
            p++;
        }
        else if (c == '.') {
            dot_count++;
            clean.push_back(c);
            p++;
        }
        else if (c == '_') {
            p++;   // skip; don't store
        }
        else {
            break;
        }
    }

    if (dot_count > 1) throw parse_error("too many dots in number", pos);
    if (*clean.end() == '.') throw parse_error("bad number format", pos);

    // Default type inferred from shape of literal
    rtype::t typ = dot_count == 1 ? rtype::a_f64 : rtype::a_i64;

    std::string_view trail = sv.substr(p);
    if (p < sv.size() && is_alpha(sv[p])) {
        struct Suf {
            std::string_view s;
            rtype::t         t;
        };
        // Longer entries first so "bf16" beats a hypothetical "b" prefix.
        static constexpr Suf suffixes[] = {
            {"bf16", rtype::a_bf16},
            { "f64",  rtype::a_f64},
            { "f32",  rtype::a_f32},
            { "f16",  rtype::a_f16},
            { "i64",  rtype::a_i64},
            { "i32",  rtype::a_i32},
            { "i16",  rtype::a_i16},
            {  "i8",   rtype::a_i8},
        };

        for (auto &[s, t] : suffixes) {
            if (!trail.starts_with(s)) continue;
            typ  = t;
            p   += s.size();
            break;
        }
        if (p < sv.size() && is_identchar(sv[p]))
            throw parse_error("unknown number type suffix", pos);
    }

    // Catch e.g. 12.5_i32 — truncation vs. error is a design call
    bool is_float_type =
        (typ == rtype::a_f16 || typ == rtype::a_f32 || typ == rtype::a_f64 || typ == rtype::a_bf16);
    if (dot_count == 1 && !is_float_type)
        throw parse_error("decimal literal with integer suffix", pos);

    // Wrap from_chars error codes into throws
    auto chk = [&](std::errc ec, const char *name) {
        if (ec == std::errc::invalid_argument)
            throw parse_error(std::string("bad ") + name + " literal", pos);
        if (ec == std::errc::result_out_of_range)
            throw parse_error(std::string(name) + " overflow", pos);
    };

    switch (typ) {
    case rtype::a_i8: {
        int val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "i8");
        if (val > 127 || val < -128) throw parse_error("i8 overflow", pos);
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_i8, static_cast<int8_t>(val));
        break;
    }
    case rtype::a_i16: {
        int16_t val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "i16");
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_i16, val);
        break;
    }
    case rtype::a_i32: {
        int32_t val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "i32");
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_i32, val);
        break;
    }
    case rtype::a_i64: {
        int64_t val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "i64");
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_i64, val);
        break;
    }
    case rtype::a_f32: {
        float val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "f32");
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_f32, val);
        break;
    }
    case rtype::a_f64: {
        double val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "f64");
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_f64, val);
        break;
    }
    case rtype::a_f16: {
        // No from_chars overload for _Float16; parse as double then narrow.
        double val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "f16");
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_f16, static_cast<float16_t>(val));
        break;
    }
    case rtype::a_bf16: {
        double val;
        chk(std::from_chars(clean.data(), clean.data() + clean.size(), val).ec, "bf16");
        n.type = ntype::t_number;
        n.pos  = pos;
        n.a    = atom(rtype::a_bf16, static_cast<bfloat16_t>(val));
        break;
    }
    default: assert(false);   // unreachable: typ is always set to a valid atom rtype above
    }

    return p;
}

unsigned reader::parse_str(unsigned pos, node &n) {
    if (sv[pos] != '"') return no_parse;

    unsigned p = pos + 1;
    while (has_more(p)) {
        if (sv[p] == '"' && sv[p - 1] != '\\') {
            n = node(asv::ntype::t_str, pos, new std::string(sv, pos + 1, p - pos - 1));
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
    n = node(asv::ntype::t_ident, pos, new std::string(sv, pos, p - pos));
    return p;
}

unsigned reader::parse_comment(unsigned pos, node &n) {
    if (sv[pos] != '/') return no_parse;
    char prev = pos == 0 ? 0 : sv[pos - 1];
    if (!is_ws(prev) && !is_term(prev)) return no_parse;
    int p = pos + 1;
    while (has_more(p) && p != '\n')
        p += 1;
    n = node(asv::ntype::t_comment, pos, new std::string(sv, pos, p - pos));
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
    n.type = asv::ntype::t_sym;
    return p;
}

unsigned reader::parse_oper(unsigned pos, node &n) {
    char c = sv[pos];
    if (!oper_table.at(static_cast<unsigned>(c))) return no_parse;
    n = node(asv::ntype::t_oper, pos, c);
    return pos + 1;
}

unsigned reader::parse_adverb(unsigned pos, node &n) {
    auto sv_pos = sv.substr(pos);
    for (unsigned i = 0; i < adverbs.size(); ++i) {
        if (!sv_pos.starts_with(adverbs[i])) continue;
        n = node(asv::ntype::t_adverb, pos, static_cast<int64_t>(i));
        return pos + adverbs[i].size();
    }
    return no_parse;
}

unsigned reader::parse_term(unsigned pos, node &n) {
    char c = pos >= sv.size() ? 0 : sv[pos];
    if (!(is_term(c) || is_rightgroup(c))) return no_parse;
    n = node(asv::ntype::t_term, pos, c);
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

    n = node(asv::ntype::t_func, pos, fn);
    return p;
}

unsigned reader::parse_funcparams(unsigned pos, node &n) {
    using enum asv::ntype;

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
            if (params[i].type != t_ident && params[i].type != t_term)
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

unsigned reader::parse_group(unsigned pos, node &n, asv::ntype type, char open, char close) {
    unsigned p = pos;
    if (sv[p] != open) return no_parse;
    p += 1;

    bool          done = false;
    vector<node> *list = new vector<node>();
    while (has_more(p) && !done) {
        node child;
        p = skip_ws(p);
        p = parse_expr(p, child);
        assert(child.n->back().type == asv::ntype::t_term);
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
    return parse_group(pos, n, asv::ntype::t_parlist, '(', ')');
}
unsigned reader::parse_bracketlist(unsigned pos, node &n) {
    return parse_group(pos, n, asv::ntype::t_bracketlist, '[', ']');
}
unsigned reader::parse_bracelist(unsigned pos, node &n) {
    return parse_group(pos, n, asv::ntype::t_bracelist, '{', '}');
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
    n = node(asv::ntype::t_toplevel, start, v);
    return p;
}

unsigned reader::parse_expr(unsigned pos, node &n) {
    unsigned p = skip_ws(pos);
    if (!has_more(p)) return no_parse;

    unsigned      start = p;
    vector<node> *v     = new vector<node>();
    bool          done  = false;

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
            if (child.f->name.type == asv::ntype::t_ident)
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
        ret = parse_adverb(p, child);
        if (ret != no_parse) goto bottom;
        ret = parse_oper(p, child);
        if (ret != no_parse) goto bottom;
        throw parse_error("unknown token", p);
    bottom:
        v->emplace_back(std::move(child));
        p = ret;
        p = skip_ws(p);
    }
    n = node(asv::ntype::t_expr, start, v);
    return p;
}

void pretty_print(node &n, int level, char const *prefix) {
    using enum asv::ntype;

    for (int i = 0; i < level; ++i)
        std::cout << "  ";

    std::cout << prefix << n << std::endl;

    switch (n.type) {
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

template <typename Dest, typename Src>
static Dest numeric_cast(Src v) {
    if constexpr (std::is_same_v<Dest, Src>) {
        return v;
    }
    else if constexpr ((std::is_same_v<Src, float16_t> || std::is_same_v<Src, bfloat16_t>) ||
                       (std::is_same_v<Dest, float16_t> || std::is_same_v<Dest, bfloat16_t>))
    {
        return static_cast<Dest>(static_cast<float>(v));
    }
    else {
        return static_cast<Dest>(v);
    }
}

static void convert_numeric(rtype dest_t, void *dest_p, rtype src_t, void const *src_p) {
    with_numeric_type(dest_t, [&]<typename Dest>(std::type_identity<Dest>) {
        with_numeric_type(src_t, [&]<typename Src>(std::type_identity<Src>) {
            *static_cast<Dest *>(dest_p) = numeric_cast<Dest>(*static_cast<Src const *>(src_p));
        });
    });
}

static void nums_to_vec(vector<node> &ns, unsigned i) {
    if (i == ns.size() - 1 || ns[i + 1].type != asv::ntype::t_number) return;
    rtype    largest = rtype::a_i8;
    unsigned end     = i;
    while (end < ns.size() && ns[end].type == asv::ntype::t_number) {
        largest  = ns[end].a.type.to_int() > largest.to_int() ? ns[end].a.type : largest;
        end     += 1;
    }
    vec *v = vec::make(largest.to_vec(), end - i);
    for (unsigned j = i; j < end; ++j) {
        atom &a = ns[j].a;
        convert_numeric(largest, v->head() + (j - i) * largest.size_class(), a.type, &a.data_);
    }
    v->len = end - i;
    ns.erase(ns.begin() + i + 1, ns.begin() + end);
    ns[i].type = ntype::t_vec;
    ns[i].v    = v;

    return;
}

void optpass_veclit(vector<node> &ns) {
    for (unsigned i = 0; i < ns.size(); ++i) {
        if (ns[i].type == ntype::t_number) {
            nums_to_vec(ns, i);
        }
    }
}

}   // namespace asv
