#include <stdio.h>
#include <stdlib.h>

#include "lex.h"
#include "parse.h"

#define RULE_RHS_LAST 8
#define STACK_MAX_DEPTH 256
#define GRAMMAR_SIZE (sizeof(grammar) / sizeof(*grammar))

static struct node stack[STACK_MAX_DEPTH];
static size_t st_size;

struct term {
    /* a rule RHS term is either a terminal token or a non-terminal */
    union {
        const tk_t tk;
        const nt_t nt;
    };

    /* indicates which field of the above union to use */
    const uint8_t is_tk: 1;

    /* indicates that the non-terminal can be matched multiple times */
    const uint8_t is_mt: 1;
};

struct rule {
    /* left-hand side of rule */
    const nt_t lhs;

    /* right-hand side of rule */
    const struct term rhs[RULE_RHS_LAST + 1];
};

#define n(_nt) { .nt = NT_##_nt, .is_tk = 0, .is_mt = 0 }
#define m(_nt) { .nt = NT_##_nt, .is_tk = 0, .is_mt = 1 }
#define t(_tm) { .tk = TK_##_tm, .is_tk = 1, .is_mt = 0 }
#define no     { .tk = TK_COUNT, .is_tk = 1, .is_mt = 0 }

#define r1(_lhs, t1) \
    { .lhs = NT_##_lhs, .rhs = { no, no, no, no, no, no, no, no, t1, } },
#define r2(_lhs, t1, t2) \
    { .lhs = NT_##_lhs, .rhs = { no, no, no, no, no, no, no, t1, t2, } },
#define r3(_lhs, t1, t2, t3) \
    { .lhs = NT_##_lhs, .rhs = { no, no, no, no, no, no, t1, t2, t3, } },
#define r4(_lhs, t1, t2, t3, t4) \
    { .lhs = NT_##_lhs, .rhs = { no, no, no, no, no, t1, t2, t3, t4, } },
#define r5(_lhs, t1, t2, t3, t4, t5) \
    { .lhs = NT_##_lhs, .rhs = { no, no, no, no, t1, t2, t3, t4, t5, } },
#define r6(_lhs, t1, t2, t3, t4, t5, t6) \
    { .lhs = NT_##_lhs, .rhs = { no, no, no, t1, t2, t3, t4, t5, t6, } },
#define r7(_lhs, t1, t2, t3, t4, t5, t6, t7) \
    { .lhs = NT_##_lhs, .rhs = { no, no, t1, t2, t3, t4, t5, t6, t7, } },
#define r8(_lhs, t1, t2, t3, t4, t5, t6, t7, t8) \
    { .lhs = NT_##_lhs, .rhs = { no, t1, t2, t3, t4, t5, t6, t7, t8, } },

static const struct rule grammar[] = {
    r3(Unit, t(FBEG), m(Stmt), t(FEND)                   )
    r1(Stmt, n(Ctrl)                                     )
    r2(Stmt, n(Prnt), t(SCOL)                            )
    r2(Stmt, n(Expr), t(SCOL)                            )
    r2(Prnt, t(PRNT), n(Expr)                            )
    r1(Ctrl, n(Cond)                                     )
    r1(Ctrl, n(Loop)                                     )
    r5(Cond, t(COND), n(Expr), t(LBRC), m(Stmt), t(RBRC) )
    r5(Loop, t(WHIL), n(Expr), t(LBRC), m(Stmt), t(RBRC) )
    r1(Atom, t(NAME)                                     )
    r1(Atom, t(NMBR)                                     )
    r1(Expr, n(Atom)                                     )
    r1(Expr, n(Bexp)                                     )
    r1(Expr, n(Pexp)                                     )
    r3(Pexp, t(LPAR), n(Expr), t(RPAR)                   )
    r3(Bexp, n(Expr), t(ASSN), n(Expr)                   )
    r3(Bexp, n(Expr), t(PLUS), n(Expr)                   )
    r3(Bexp, n(Expr), t(MINS), n(Expr)                   )
    r3(Bexp, n(Expr), t(EQUL), n(Expr)                   )
    r3(Bexp, n(Expr), t(NEQL), n(Expr)                   )
};

#undef r1
#undef r2
#undef r3
#undef r4
#undef r5
#undef r6
#undef r7
#undef r8

#undef n
#undef m
#undef t
#undef no

static void print_stack(void)
{
    static const char *nts[NT_COUNT] = {
        "Unit",
        "Stmt",
        "Prnt",
        "Ctrl",
        "Cond",
        "Loop",
        "Atom",
        "Expr",
        "Pexp",
        "Bexp",
    };

    for (size_t i = 0; i < st_size; ++i) {
        if (stack[i].nchildren) {
            printf(YELLOW("%s "), nts[stack[i].nt]);
        } else if (stack[i].token->tk == TK_FBEG) {
            printf(GREEN("^ "));
        } else if (stack[i].token->tk == TK_FEND) {
            printf(GREEN("$ "));
        } else {
            int len = stack[i].token->end - stack[i].token->beg;
            printf(GREEN("%.*s "), len, stack[i].token->beg);
        }
    }

    puts("");
}

static void destroy_node(struct node *node)
{
    if (node->nchildren) {
        for (size_t i = 0; i < node->nchildren; ++i) {
            destroy_node(node->children[i]);
        }

        free(*&node->children[0]);
        free(node->children);
    }
}

static void destroy_stack(void)
{
    for (size_t i = 0; i < st_size; ++i) {
        destroy_node(&stack[i]);
    }

    st_size = 0;
}

static int term_eq_node(const struct term *term, const struct node *node)
{
    int node_is_leaf = node->nchildren == 0;

    if (term->is_tk == node_is_leaf) {
        if (node_is_leaf) {
            return term->tk == node->token->tk;
        } else {
            return term->nt == node->nt;
        }
    }

    return 0;
}

static size_t match_rule(const struct rule *rule, size_t *at)
{
    const struct term *prev = NULL;
    const struct term *term = &rule->rhs[RULE_RHS_LAST];
    ssize_t st_idx = st_size - 1;

    do {
        if (term_eq_node(term, &stack[st_idx])) {
            prev = term->is_mt ? term : NULL;
            --term, --st_idx;
        } else if (prev && term_eq_node(prev, &stack[st_idx])) {
            --st_idx;
        } else if (term->is_mt) {
            prev = NULL;
            --term;
        } else {
            term = NULL;
            break;
        }
    } while (st_idx >= 0 && !(term->is_tk && term->tk == TK_COUNT));

    int reached_eor = term && term->is_tk && term->tk == TK_COUNT;
    size_t reduction_size = st_size - st_idx - 1;

    return reached_eor && reduction_size ?
        (*at = st_idx + 1, reduction_size) : 0;
}

static inline void shift(const struct token *token)
{
    stack[st_size++] = (struct node) {
        .nchildren = 0,
        .token = token,
    };
}

static int reduce(const struct rule *rule, const size_t at, const size_t size)
{
    struct node *child_nodes = malloc(size * sizeof(struct node));

    if (!child_nodes) {
        return -1;
    }

    struct node *const reduce_at = &stack[at];
    struct node **const old_children = reduce_at->children;
    reduce_at->children = malloc(size * sizeof(struct node *)) ?: old_children;

    if (reduce_at->children == old_children) {
        return free(child_nodes), -1;
    }

    for (size_t child_idx = 0, st_idx = at; 
        st_idx < st_size;
        ++st_idx, ++child_idx) {

        child_nodes[child_idx] = stack[st_idx];
        reduce_at->children[child_idx] = &child_nodes[child_idx];
    }

    child_nodes[0].children = old_children;
    reduce_at->nchildren = size;
    reduce_at->nt = rule->lhs;
    st_size = at + 1;
    return 0;
}

struct node parse(const struct token *ranges, const size_t nranges)
{
    static const struct token reject = {
        .tk = TK_COUNT
    }, nomem = {
        .tk = TK_COUNT + 1
    };
    
    static const struct node err_reject = {
        .nchildren = 0,
        .token = &reject,
    }, err_nomem = {
        .nchildren = 0,
        .token = &nomem,
    };

    st_size = 0;

    for (size_t range_idx = 0; range_idx < nranges; ++range_idx) {
        if (ranges[range_idx].tk == TK_WSPC) {
            continue;
        }

        if (st_size > STACK_MAX_DEPTH - 1) {
            puts(RED("Stack depth exceeded!"));
            break;
        }

        shift(&ranges[range_idx]);
        printf(CYAN("Shift: ")), print_stack();
        
        try_reduce_again:;
        const struct rule *rule = grammar;

        do {
            size_t reduction_at;
            size_t reduction_size = match_rule(rule, &reduction_at);

            if (reduction_size) {
                if (reduce(rule, reduction_at, reduction_size)) {
                    return destroy_stack(), err_nomem;
                }

                ptrdiff_t rule_number = rule - grammar + 1;
                printf(ORANGE("Red%02td: "), rule_number), print_stack();
                goto try_reduce_again;
            }
        } while (++rule != grammar + GRAMMAR_SIZE);
    }

    int accepted = st_size == 1;

    printf(accepted ? GREEN("ACCEPT ") : RED("REJECT ")), print_stack();
    return accepted ? stack[0] : err_reject;
}
