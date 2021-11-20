#include <string>
#include <set>
#include <map>
#include <vector>
#include <deque>
#include <algorithm>
#include <iterator>
#include <memory>
#include <iostream>

typedef std::string::const_iterator scanptr_type;
typedef std::set<int> re_set_type;
typedef std::shared_ptr<re_set_type> re_set_ptr;
struct re_ast_type;
typedef std::shared_ptr<re_ast_type> re_ast_ptr;

enum { EPSILON, SHARP, OR, CAT, STAR, PLUS, QUES, LETTER };

struct re_ast_type {
    int m_op;
    re_ast_ptr m_lhs;
    re_ast_ptr m_rhs;
    int m_code;
    int m_pos;
    bool m_nullable;
    std::set<int> m_firstpos;
    std::set<int> m_lastpos;
    std::set<int> m_followpos;

    explicit re_ast_type (int op, re_ast_ptr lhs, re_ast_ptr rhs)
        : m_op (op), m_lhs (lhs), m_rhs (rhs),
          m_code (0), m_pos (0),
          m_nullable (false), m_firstpos (), m_lastpos (), m_followpos () {}

    void set_letter_pos (std::vector<re_ast_ptr>& place);
    void reduce_followpos (std::vector<re_ast_ptr> const& place);
    std::string to_string (void) const;

private:
    void loop_followpos (std::vector<re_ast_ptr> const& place);
    void union_followpos (std::vector<re_ast_ptr> const& place,
        std::set<int>& u1, std::set<int>& u2);
    void union_nullable (void);
    void union_firstpos (void);
    void union_lastpos (void);
};

// parsing result for each productions.

struct re_match_type {
    re_ast_ptr m_re;
    scanptr_type m_derivs;
};

// dstate is the DFA state, the set of important places.

struct re_dstate_type {
    bool m_marked;
    bool m_sharped;
    int m_id;
    re_set_type m_set;
};

// the DFA construction context.

class re_dfa_type {
public:
    enum { FINCODE = -1 };
    re_dfa_type () {}
    bool compile (scanptr_type first, scanptr_type second);
    void minimize (void);
    std::string fold_string (void) const;

public:
    int m_sharp;                // finally, places of SHARP terminator
    scanptr_type m_end;
    std::vector<re_ast_ptr> m_place;
    re_match_type m_matched;
    re_set_type m_letter;
    std::map<int,std::map<int,int> > m_transition;
    re_set_type m_final;

    bool parse (scanptr_type first, scanptr_type second);
    void extend_sharp (re_match_type& matched);
    bool parse_expr (scanptr_type s, re_match_type& matched);
    bool parse_concat (scanptr_type s, re_match_type& matched);
    bool parse_piece (scanptr_type s, re_match_type& matched);
    bool parse_atom (scanptr_type s, re_match_type& matched);
    bool parse_paren (scanptr_type s, re_match_type& matched);
    bool parse_letter (scanptr_type s, re_match_type& matched);

    void dfa_construct ();

    void minimize_init_partition (std::vector<re_set_type>& partition);
    void minimize_to_trie (std::vector<std::vector<int> >& transition);

    std::deque<re_dstate_type>::iterator
    dstate_unmarked (std::deque<re_dstate_type>& dstate);
    std::deque<re_dstate_type>::iterator
    dstate_insert (std::deque<re_dstate_type>& dstate, int& seq, re_set_type& u);
};

struct re_dfa_hopcroft_impl {
    std::map<int,std::map<int,int> > const& m_transition;
    re_set_type const& m_letter;
    re_set_type const& m_final;
    std::map<int,std::map<int,re_set_type> > m_itran;
    std::deque<re_set_ptr> m_partition;
    std::deque<re_set_ptr> m_worklist;

    explicit re_dfa_hopcroft_impl (
        std::map<int,std::map<int,int> > const& transition,
        re_set_type const& letter, re_set_type const& final
    ) : m_transition (transition), m_letter (letter), m_final (final) {}
    void minimize (void);
    void reconstruct (std::map<int,std::map<int,int> >& transition,
        re_set_type& final);

    void inverse_transition (void);
    void init_partition (void);
    bool select_lead_states (int code, re_set_ptr a, re_set_type& x);
    void update_partition (re_set_type const& x);
    void split_group (re_set_ptr y, re_set_type const& x,
        re_set_ptr& reject_x, re_set_ptr& accept_x);
    void update_worklist (re_set_ptr reject_x, re_set_ptr accept_x, re_set_ptr y);
    int intern_group (int state, std::vector<re_set_ptr>& group);
};

void re_set_insert (re_set_type& u, int element);
void re_set_union (re_set_type& u, re_set_type const& u1, re_set_type const& u2);
bool re_set_member (re_set_type const& u, int v);
std::string re_set_to_string (re_set_type const& u);
std::string slurp (int argc, char* argv[]);

// int main ()
// {
//     bool ok = false;
//         std ::string str;
//         std :: cin >> str ; 
//         //std::string src = slurp (argc, argv);
//         re_dfa_type dfa;
//         ok = dfa.compile (str.cbegin (), str.cend ());
        
//         for( auto k : dfa.m_transition) {
//             std :: cout << k.first << ": \n";
//             for(auto j : k.second ) std :: cout << j.first << " " << j.second << '\n'; 
//         }

//     return 0;
// }

std::string
slurp (int argc, char* argv[])
{
    std::string src;
    if (argc == 2) {
        src = argv[1];
    }
    else {
        for (std::string line; std::getline (std::cin, line); )
            src += line;
    }
    return src;
}

// Algorithm 3.36

bool
re_dfa_type::compile (scanptr_type first, scanptr_type second)
{
    bool const ok = parse (first, second);
    if (ok) {
        extend_sharp (m_matched);
        m_place.clear ();
        m_matched.m_re->set_letter_pos (m_place);
        m_sharp = m_matched.m_re->m_rhs->m_pos;
        m_matched.m_re->reduce_followpos (m_place);
        dfa_construct ();
        //std::cout << fold_string () << "---\n";
        minimize ();
    }
    return ok;
}

// front end
//
// parse regular expression input, and construct abstract syntax tree, AST.
// We may get the root node of AST in the member ctx.m_matched.re.
// On success, ctx.m_pos holds the place number of sharp terminator.
// ctx.m_path is the head of list for important places such as letters
// and sharp termination.
// ctx.m_letter has the set of letter in the regular expression.

bool
re_dfa_type::parse (scanptr_type first, scanptr_type second)
{
    m_end = second;
    bool ok = parse_expr (first, m_matched);
    if (ok && m_matched.m_derivs != m_end) {
        std::cerr << "error: syntax error." << std::endl;
        ok = false;
    }
    return ok;
}

// construct extended AST
// <root> => (CAT <root> SHARP)

void
re_dfa_type::extend_sharp (re_match_type& matched)
{
    matched.m_re = std::make_shared<re_ast_type> (CAT, matched.m_re, nullptr);
    matched.m_re->m_rhs = std::make_shared<re_ast_type> (SHARP, nullptr, nullptr);
}

// expression : expression '|' concat   { (OR $1 $3) }
//            | concat
//            ;

bool
re_dfa_type::parse_expr (scanptr_type s, re_match_type& matched)
{
    re_match_type lhs;
    re_match_type rhs;

    bool ok = parse_concat (s, lhs);
    if (ok) {
        s = lhs.m_derivs;
        while (ok && m_end != s && '|' == *s) {
            ++s;
            ok = parse_concat (s, rhs);
            if (ok) {
                lhs.m_re = std::make_shared<re_ast_type> (OR, lhs.m_re, rhs.m_re);
                s = rhs.m_derivs;
            }
        }
    }
    matched.m_re = lhs.m_re;
    matched.m_derivs = s;
    return ok;
}

// concat : concat piece    { (CAT $1 $2) }
//        |                 { EPSILON }
//        ;

bool
re_dfa_type::parse_concat (scanptr_type s, re_match_type& matched)
{
    re_match_type lhs;
    re_match_type rhs;

    lhs.m_re = nullptr;
    bool ok = true;
    if (m_end == s || '|' == *s || ')' == *s) {
        lhs.m_re = std::make_shared<re_ast_type> (EPSILON, nullptr, nullptr);
    }
    else {
        ok = parse_piece (s, lhs);
        if (ok) {
            s = lhs.m_derivs;
            while (ok && m_end != s && '|' != *s && ')' != *s) {
                ok = parse_piece (s, rhs);
                if (ok) {
                    lhs.m_re = std::make_shared<re_ast_type> (CAT, lhs.m_re, rhs.m_re);
                    s = rhs.m_derivs;
                }
            }
        }
    }
    matched.m_re = lhs.m_re;
    matched.m_derivs = s;
    return ok;
}

// piece : atom '*'   { (STAR $1) }
//       | atom '+'   { (PLUS $1) }
//       | atom '?'   { (QUES $1) }
//       | atom
//       ;

bool
re_dfa_type::parse_piece (scanptr_type s, re_match_type& matched)
{
    re_match_type lhs;

    bool ok = parse_atom (s, lhs);
    if (ok) {
        s = lhs.m_derivs;
        if (m_end != s && '*' == *s) {
            ++s;
            lhs.m_re = std::make_shared<re_ast_type> (STAR, lhs.m_re, nullptr);
        }
        else if (m_end != s && '+' == *s) {
            ++s;
            lhs.m_re = std::make_shared<re_ast_type> (PLUS, lhs.m_re, nullptr);
        }
        else if (m_end != s && '?' == *s) {
            ++s;
            lhs.m_re = std::make_shared<re_ast_type> (QUES, lhs.m_re, nullptr);
        }
    }
    matched.m_re = lhs.m_re;
    matched.m_derivs = s;
    return ok;
}

// atom : '(' expression ')'    { $2 }
//      | letter                { (LETTER $1) }
//      ;

bool
re_dfa_type::parse_atom (scanptr_type s, re_match_type& matched)
{
    bool ok = false;
    int const ch = static_cast<unsigned char> (*s);
    if ('(' == ch) {
        ok = parse_paren (s, matched);
    }
    else if (' ' <= ch && 0x7f != ch && '*' != ch && '?' != ch && '+' != ch) {
        ok = parse_letter (s, matched);
    }
    else {
        std::cerr << "regexp: illegal letter" << std::endl;
        ++s;
        matched.m_re = nullptr;
        matched.m_derivs = s;
    }
    return ok;
}

// atom : '(' expression ')'    { $2 }

bool
re_dfa_type::parse_paren (scanptr_type s, re_match_type& matched)
{
    ++s; // skip '('
    bool ok = parse_expr (s, matched);
    if (ok) {
        s = matched.m_derivs;
        if (m_end != s && ')' == *s) {
            ++s;
            matched.m_derivs = s;
        }
        else {
            std::cerr << "regexp: lost right paren" << std::endl;
            ok = false;
        }
    }
    return ok;
}

// LETTERs are the important places for construction of DFA.
// We give them the unique `m_pos` identifiers.

// atom : letter                { (LETTER $1) }
//      | '\\' letter           { (LETTER $2) }

bool
re_dfa_type::parse_letter (scanptr_type s, re_match_type& matched)
{
    int ch = static_cast<unsigned char> (*s);
    bool ok = '\\' != ch;
    if ('\\' == ch && m_end != s + 1) {
        ++s;
        ch = static_cast<unsigned char> (*s);
        ok = ' ' <= ch && ch != 0x7f;
    }
    if (ok) {
        re_set_insert (m_letter, ch);
        ++s;
        matched.m_re = std::make_shared<re_ast_type> (LETTER, nullptr, nullptr);
        matched.m_re->m_code = ch;
        matched.m_derivs = s;
    }
    return ok;
}

// stringify AST

std::string
re_ast_type::to_string (void) const
{
    std::string out;
    if (EPSILON == m_op) {
        out = "epsilon";
    }
    else if (SHARP == m_op) {
        out = "#.";
        out += std::to_string (m_pos);
    }
    else if (LETTER == m_op) {
        out = "'";
        out.push_back (m_code);
        out += "'.";
        out += std::to_string (m_pos);
    }
    else if (OR == m_op || CAT == m_op) {
        out += OR == m_op ? "(or " : "(cat ";
        if (nullptr != m_lhs) out += m_lhs->to_string ();
        out += " ";
        if (nullptr != m_rhs) out += m_rhs->to_string ();
        out += ")";
    }
    else if (STAR == m_op || PLUS == m_op || QUES == m_op) {
        out += STAR == m_op ? "(star " : PLUS == m_op ? "(plus " : "(ques ";
        if (nullptr != m_lhs) out += m_lhs->to_string ();
        out += ")";
    }
    return out;
}

// backend
//
// set position for all LETTERs and the SHARP node.

void
re_ast_type::set_letter_pos (std::vector<re_ast_ptr>& place)
{
    if (nullptr != m_lhs) {
        if (LETTER == m_lhs->m_op || SHARP == m_lhs->m_op) {
            m_lhs->m_pos = place.size ();
            place.push_back (m_lhs);
        }
        else {
            m_lhs->set_letter_pos (place);
        }
    }
    if (nullptr != m_rhs) {
        if (LETTER == m_rhs->m_op || SHARP == m_rhs->m_op) {
            m_rhs->m_pos = place.size ();
            place.push_back (m_rhs);
        }
        else {
            m_rhs->set_letter_pos (place);
        }
    }
}

// Calculate the m_followpos set of positions at letters or sharp. 

void
re_ast_type::reduce_followpos (std::vector<re_ast_ptr> const& place)
{
    union_nullable ();
    union_firstpos ();
    union_lastpos ();
    loop_followpos (place);
}

// from CAT, STAR or PLUS nodes,
// build followpos at the LETTER or the SHARP nodes.
// see dragon book section 3.9.4
//
//     loop_followpos(e1 e2)
//       for x in lastpos(e1) do followpos(x) U= firstpos(e2)
//     loop_followpos(e*)
//       for x in lastpos(e) do followpos(x) U= firstpos(e)
//     loop_followpos(e+)
//       for x in lastpos(e) do followpos(x) U= firstpos(e)

void
re_ast_type::loop_followpos (std::vector<re_ast_ptr> const& place)
{
    if (nullptr != m_lhs) m_lhs->loop_followpos (place);
    if (nullptr != m_rhs) m_rhs->loop_followpos (place);
    if (CAT == m_op)
        union_followpos (place, m_lhs->m_lastpos, m_rhs->m_firstpos);
    else if (STAR == m_op || PLUS == m_op)
        union_followpos (place, m_lastpos, m_firstpos);
}

void
re_ast_type::union_followpos (std::vector<re_ast_ptr> const& place,
    std::set<int>& u1, std::set<int>& u2)
{
    for (int const u1pos : u1) {
        re_ast_ptr re = place[u1pos];
        re_set_union (re->m_followpos, re->m_followpos, u2); // followpos U= u2;
    }
}

// memorize nullable function's results for each AST nodes.
// see dragon book section 3.9.3
//
//     nullable(a)  == false
//     nullable(#)  == false
//     nullable(e1|e2) == nullable(e1) || nullable(e2)
//     nullable(e1 e2) == nullable(e1) && nullable(e2)
//     nullable(e*) == true
//     nullable(e+) == nullable(e)
//     nullable(e?) == true

void
re_ast_type::union_nullable ()
{
    if (nullptr != m_lhs) m_lhs->union_nullable ();
    if (nullptr != m_rhs) m_rhs->union_nullable ();
    switch (m_op) {
    case LETTER:
    case SHARP:
        m_nullable = false;
        break;
    case OR:
        m_nullable = m_lhs->m_nullable || m_rhs->m_nullable;
        break;
    case CAT:
        m_nullable = m_lhs->m_nullable && m_rhs->m_nullable;
        break;
    case PLUS:
        m_nullable = m_lhs->m_nullable;
        break;
    default:
        m_nullable = true;
        break;
    }
}

// memorize firstpos function's results for each AST nodes.
// see dragon book section 3.9.3
//
//     firstpos() == {}
//     firstpos(a.i) == {p_i}
//     firstpos(#.i) == {p_i}
//     firstpos(e1|e2) == firstpos(e1) U firstpos(e2)
//     firstpos(e1 e2) == nullable(e1) ? firstpos(e1) U firstpos(e2) : firstpos(e1)
//     firstpos(e*) == firstpos(e)
//     firstpos(e+) == firstpos(e)
//     firstpos(e?) == firstpos(e)

void
re_ast_type::union_firstpos ()
{
    if (nullptr != m_lhs) m_lhs->union_firstpos ();
    if (nullptr != m_rhs) m_rhs->union_firstpos ();
    switch (m_op) {
    case EPSILON:
        m_firstpos.clear ();
        break;
    case LETTER:
    case SHARP:
        re_set_insert (m_firstpos, m_pos);
        break;
    case OR:
        re_set_union (m_firstpos, m_lhs->m_firstpos, m_rhs->m_firstpos);
        break;
    case CAT:
        if (m_lhs->m_nullable)
            re_set_union (m_firstpos, m_lhs->m_firstpos, m_rhs->m_firstpos);
        else
            m_firstpos = m_lhs->m_firstpos;
        break;
    case STAR:
    case PLUS:
    case QUES:
        m_firstpos = m_lhs->m_firstpos;
        break;
    }
}

// memorize lastpos function's results for each AST nodes.
// see dragon book section 3.9.3
//
//     lastpos() == {}
//     lastpos(a.i) == {p_i}
//     lastpos(#.i) == {p_i}
//     lastpos(e1|e2) == lastpos(e1) U lastpos(e2)
//     lastpos(e1 e2) == nullable(e2) ? lastpos(e1) U lastpos(e2) : lastpos(e2)
//     lastpos(e*) == lastpos(e)
//     lastpos(e+) == lastpos(e)
//     lastpos(e?) == lastpos(e)

void
re_ast_type::union_lastpos ()
{
    if (nullptr != m_lhs) m_lhs->union_lastpos ();
    if (nullptr != m_rhs) m_rhs->union_lastpos ();
    switch (m_op) {
    case EPSILON:
        m_lastpos.clear ();
        break;
    case LETTER:
    case SHARP:
        re_set_insert (m_lastpos, m_pos);
        break;
    case OR:
        re_set_union (m_lastpos, m_lhs->m_lastpos, m_rhs->m_lastpos);
        break;
    case CAT:
        if (m_rhs->m_nullable)
            re_set_union (m_lastpos, m_lhs->m_lastpos, m_rhs->m_lastpos);
        else
            m_lastpos = m_rhs->m_lastpos;
        break;
    case STAR:
    case PLUS:
    case QUES:
        m_lastpos = m_lhs->m_lastpos;
        break;
    }
}

// Construct DFA from followpos sets in the AST
// see dragon book section 3.9.5

void
re_dfa_type::dfa_construct (void)
{
    std::deque<re_dstate_type> dstate;
    std::deque<re_dstate_type>::iterator from;
    std::deque<re_dstate_type>::iterator to;

    int seq = 0;
    dstate_insert (dstate, seq, m_matched.m_re->m_firstpos);
    while ((from = dstate_unmarked (dstate)) != dstate.cend ()) {
        from->m_marked = true;
        if (from->m_sharped)
            m_final.insert (from->m_id);
        for (int code : m_letter) {
            re_set_type u;
            for (int pos : from->m_set) {
                re_ast_ptr re = m_place[pos];
                if (code == re->m_code && LETTER == re->m_op)
                    re_set_union (u, u, re->m_followpos); // u U= followpos
            }
            if (! u.empty ()) {
                to = dstate_insert (dstate, seq, u);
                if (to->m_sharped)
                    m_final.insert (to->m_id);
                m_transition[from->m_id][code] = to->m_id;
            }
        }
    }
    for (int s : m_final) {
        m_transition[s][FINCODE] = -1;
    }
}

// If the set u already stores in the m_dstate table, use it.
// Otherwise we add new state data into the m_dstate table.

std::deque<re_dstate_type>::iterator
re_dfa_type::dstate_insert (std::deque<re_dstate_type>& dstate, int& seq, std::set<int>& u)
{
    std::deque<re_dstate_type>::iterator it = dstate.begin ();
    std::deque<re_dstate_type>::iterator found = dstate.end ();
    while (found == dstate.end () && it != dstate.end ()) {
        if (it->m_set == u)
            found = it;
        ++it;
    }
    if (found == dstate.end ()) {
        bool const sharped = re_set_member (u, m_sharp);
        dstate.push_back ({false, sharped, seq, u});
        ++seq;
        found = dstate.end () - 1;
    }
    return found;
}

// find unmarked state in the m_dstate table.

std::deque<re_dstate_type>::iterator
re_dfa_type::dstate_unmarked (std::deque<re_dstate_type>& dstate)
{
    std::deque<re_dstate_type>::iterator it = dstate.begin ();
    std::deque<re_dstate_type>::iterator found = dstate.end ();
    while (found == dstate.end () && it != dstate.end ()) {
        if (! it->m_marked)
            found = it;
        ++it;
    }
    return found;
}

// Hopcroft's DFA minimization
// It was not described in dragon book.

void
re_dfa_type::minimize (void)
{
    re_dfa_hopcroft_impl hopcroft (m_transition, m_letter, m_final);
    hopcroft.minimize ();
    hopcroft.reconstruct (m_transition, m_final);
}

// patitioning to the m_partition from the set of states in the m_transition.
// The set `*a` must not be changed in the while loop scope.

void
re_dfa_hopcroft_impl::minimize (void)
{
    inverse_transition ();
    init_partition ();
    while (! m_worklist.empty ()) {
        re_set_ptr a = m_worklist.front ();
        m_worklist.pop_front ();
        for (int code : m_letter) {
            re_set_type x;
            if (select_lead_states (code, a, x))
                update_partition (x);
        }
    }
}

// Hopcroft performs partitioning in the reverse direction.
// To reduction loop repeatations, inverse transition table before partitioning.

void
re_dfa_hopcroft_impl::inverse_transition (void)
{
    for (auto from : m_transition) {
        for (auto to : from.second) {
            if (to.first >= 0)
                m_itran[to.second][to.first].insert (from.first);
        }
    }
}

// Set initial partitioning: {Q-F, F}, here Q is the set of all states,
// and F is the set of final states.
// Set work list {Q-F, F} too.
// The work list tracks unvisited sets in m_parition.
//
// Notes: the sample code of several papers in the World Wide Web
// and Wikipedia https://en.wikipedia.org/wiki/DFA_minimization
// may be incorrect initial settings to the work list.
//
//      W := {F};                       (* Wikipedia *)
//      W = {smallest of Q-F and F};    (* several papers *)
//
// They may work only in the case of strongly connected component
// or the single path to goal states.
//
//      -    m_worklist = m_partition;
//      +    if (u.size () < m_final.size ())
//      +        m_worklist.push_back (u);
//      +    else
//      +        m_worklist.push_back (m_final);
//
// Above substitution leads incomplete partitioning as below:
//
//     $  ./regexp-to-dfa '-?(0+(.0*)?|.0+)(e-?0+)?'
//     S0: '-' S1 | '.' S2 | '0' S3
//     S1: '.' S2 | '0' S3
//     S2: '-' S2 | '0' S4
//     S3: '.' S4 | '0' S3 | 'e' S2 | #
//     S4: '0' S4 | 'e' S2 | #

void
re_dfa_hopcroft_impl::init_partition (void)
{
    re_set_type u;
    for (auto tran : m_transition) {
        if (m_final.count (tran.first) == 0)
            u.insert (tran.first);
    }
    m_partition.push_back (std::make_shared<re_set_type> (u));
    m_partition.push_back (std::make_shared<re_set_type> (m_final));
    m_worklist = m_partition;
}

// Select states: X == {s| "s: code f" for f in A }
// X is the set of states to lead states in A by code.

bool
re_dfa_hopcroft_impl::select_lead_states (int code, re_set_ptr a, re_set_type& x)
{
    for (int state_a : *a) {
        if (m_itran.count (state_a) && m_itran.at (state_a).count (code))
            re_set_union (x, x, m_itran.at (state_a).at (code));
    }
    return ! x.empty ();
}

// Update group Y to reject set: Y-X and accept set: Y (intersection) X

void
re_dfa_hopcroft_impl::update_partition (re_set_type const& x)
{
    std::deque<re_set_ptr> result;
    for (auto it = m_partition.begin (); it != m_partition.end (); ++it) {
        re_set_ptr reject_x = std::make_shared<re_set_type> ();
        re_set_ptr accept_x = std::make_shared<re_set_type> ();
        split_group (*it, x, reject_x, accept_x);
        if (reject_x->empty () || accept_x->empty ()) {
            result.push_back (*it);
        }
        else {
            result.push_back (reject_x);
            result.push_back (accept_x);
            update_worklist (reject_x, accept_x, *it);
        }
    }
    std::swap (m_partition, result);
}

// Split Y and X to reject set: Y-X and accept set: Y (intersection) X

void
re_dfa_hopcroft_impl::split_group (re_set_ptr y, re_set_type const& x,
    re_set_ptr& reject_x, re_set_ptr& accept_x)
{
    for (int ystate : *y) {
        if (x.count (ystate))
            accept_x->insert (ystate);
        else
            reject_x->insert (ystate);
    }
}

// Update m_work list. It contains unvisited sets in m_parition.

void
re_dfa_hopcroft_impl::update_worklist (re_set_ptr reject_x, re_set_ptr accept_x,
    re_set_ptr y)
{
    auto w = std::find (m_worklist.begin (), m_worklist.end (), y);
    if (w != m_worklist.end ()) {
        // With GNU libstdc++, it is important the order of substition
        // before insertion. Sometimes, insertion moves iterator.
        *w = accept_x;
        m_worklist.insert (w, reject_x);
    }
    else if (accept_x->size () < reject_x->size ()) {
        m_worklist.push_back (accept_x);
    }
    else {
        m_worklist.push_back (reject_x);
    }
}

// naive reconstrution of minimized DFA transition.

void
re_dfa_hopcroft_impl::reconstruct (std::map<int,std::map<int,int> >& transition,
    re_set_type& final)
{
    std::map<int,std::map<int,int> > tmp_transition;
    std::set<int> tmp_final;
    std::vector<re_set_ptr> group;

    for (auto from : m_transition) {
        for (auto to : from.second) {
            int group_from = intern_group (from.first, group);
            if (to.first >= 0) {
                int group_to = intern_group (to.second, group);
                tmp_transition[group_from][to.first] = group_to;
            }
            else {
                tmp_transition[group_from][to.first] = to.second;
                tmp_final.insert (group_from);
            }
        }
    }
    std::swap (transition, tmp_transition);
    std::swap (final, tmp_final);
}

// give the unique number to the set of states in the m_parition.

int
re_dfa_hopcroft_impl::intern_group (int state, std::vector<re_set_ptr>& group)
{
    int id = -1;
    for (int i = 0; -1 == id && i < group.size (); ++i) {
        if (group.at (i)->count (state))
            id = i;
    }
    if (-1 == id) {
        for (auto y = m_partition.begin (); -1 == id && y != m_partition.end (); ++y) {
            if ((*y)->count (state)) {
                id = group.size ();
                group.push_back (*y);
            }
        }
    }
    return id;
}

// insert a value into the sorted set

void
re_set_insert (re_set_type& u, int element)
{
    u.insert (element);
}

// union two sets.

void
re_set_union (re_set_type& u, re_set_type const& u1, re_set_type const& u2)
{
    re_set_type w;
    std::set_union (u1.begin (), u1.end (), u2.begin (), u2.end (),
                    std::inserter (w, w.end ()));
    std::swap (u, w);
}

// Is value v member of set u?

bool
re_set_member (re_set_type const& u, int v)
{
    return u.count (v) > 0;
}

// stringify set of int

std::string
re_set_to_string (re_set_type const& u)
{
    std::string out;
    out += "{";
    int i = 0;
    for (int x : u) {
        if (i++ > 0) out += " ";
        out += std::to_string (x);
    }
    out += "}";
    return out;
}

// stringify m_dtran table by fold style

std::string
re_dfa_type::fold_string (void) const
{
    std::string out;

    for (auto from : m_transition) {
        //out += "S";
        out += std::to_string (from.first);
        out += ": ";
        int col = 0;
        for (auto to : from.second) {
            if (to.first != FINCODE) {
                if (col++ > 0) out += " | ";
                out += "'";
                out.push_back (to.first);
                //out += "' S";
                out += std::to_string (to.second);
            }
        }
        if (from.second.count (FINCODE) > 0) {
            if (col++ > 0) out += " | ";
            out += "#";
        }
        out += "\n";
    }
    return out;
}