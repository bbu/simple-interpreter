#include <stdio.h>
#include <stdlib.h>

#include "lex.h"
#include "parse.h"

#define RULE_RHS_LAST 8
#define STACK_MAX_DEPTH 256
#define GRAMMAR_SIZE (sizeof(grammar) / sizeof(*grammar))

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

struct term {
    union {
        const token_t tm;
        const non_terminal_t nt;
    };

    const uint8_t is_terminal: 1;
    const uint8_t is_multiplier: 1;
};

struct rule {
    const non_terminal_t lhs;
    const struct term rhs[RULE_RHS_LAST + 1];
};

#define n(_nt) { .nt = NT_##_nt, .is_terminal = 0, .is_multiplier = 0 }
#define m(_nt) { .nt = NT_##_nt, .is_terminal = 0, .is_multiplier = 1 }
#define t(_tm) { .tm = TK_##_tm, .is_terminal = 1, .is_multiplier = 0 }
#define no     { .tm = TK_COUNT, .is_terminal = 1, .is_multiplier = 0 }

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

static int term_eq_node(const struct term *term, const struct stree_node *node)
{
    int node_is_leaf = node->num_children == 0;

    if (term->is_terminal == node_is_leaf) {
        if (node_is_leaf) {
            return term->tm == node->tm->token;
        } else {
            return term->nt == node->nt;
        }
    }

    return 0;
}

static void print_stack(const struct stree_node *stack, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (stack[i].num_children == 0) {
            if (stack[i].tm->token == TK_FBEG) {
                printf(GREEN("^ "));
            } else if (stack[i].tm->token == TK_FEND) {
                printf(GREEN("$ "));
            } else {
                int len = stack[i].tm->end - stack[i].tm->beg;
                printf(GREEN("%.*s "), len, stack[i].tm->beg);
            }
        } else {
            printf(YELLOW("%s "), nts[stack[i].nt]);
        }
    }

    puts("");
}

struct stree_node parse(const struct token_range *ranges, size_t nranges)
{
    ssize_t st_size = 0;
    static struct stree_node stack[STACK_MAX_DEPTH];

    for (size_t range_idx = 0; range_idx < nranges; ++range_idx) {
        if (ranges[range_idx].token == TK_WSPC) {
            continue;
        }

        if (st_size > STACK_MAX_DEPTH - 1) {
            puts(RED("Stack depth exceeded!"));
            break;
        }

        stack[st_size++] = (struct stree_node) {
            .num_children = 0,
            .tm = &ranges[range_idx],
            .children = NULL
        };

        printf(CYAN("Shift: "));
        print_stack(stack, st_size);
        
        try_reduce_again:;
        const struct rule *rule = grammar;

        do {
            const struct term *prev_term = NULL;
            const struct term *term = &rule->rhs[RULE_RHS_LAST];
            ssize_t st_idx = st_size - 1;

            do {
                if (term_eq_node(term, &stack[st_idx])) {
                    prev_term = term->is_multiplier ? term : NULL;
                    --term, --st_idx;
                } else if (prev_term && term_eq_node(prev_term, &stack[st_idx])) {
                    --st_idx;
                } else if (term->is_multiplier) {
                    prev_term = NULL;
                    --term;
                } else {
                    term = NULL;
                    break;
                }
            } while (st_idx >= 0 && !(term->is_terminal && term->tm == TK_COUNT));

            int reached_eor = term && term->is_terminal && term->tm == TK_COUNT;
            size_t reduction_size = st_size - st_idx - 1;

            if (reached_eor && reduction_size) {
                size_t reduction_idx = ++st_idx;

                struct stree_node *child_nodes = malloc(
                    reduction_size * sizeof(struct stree_node));
                
                struct stree_node **old_children = stack[reduction_idx].children;
                
                stack[reduction_idx].children = malloc(
                    reduction_size * sizeof(struct stree_node *));

                for (size_t node_idx = 0; st_idx < st_size; ++st_idx, ++node_idx) {
                    child_nodes[node_idx] = stack[st_idx];
                    stack[reduction_idx].children[node_idx] = &child_nodes[node_idx];
                }

                child_nodes[0].children = old_children;
                stack[reduction_idx].num_children = reduction_size;
                stack[reduction_idx].nt = rule->lhs;
                st_size = reduction_idx + 1;

                printf(ORANGE("Red%02td: "), rule - grammar + 1);
                print_stack(stack, st_size);

                goto try_reduce_again;
            }
        } while (++rule != grammar + GRAMMAR_SIZE);
    }

    printf(st_size == 1 ? GREEN("ACCEPT ") : RED("REJECT "));
    print_stack(stack, st_size);
    puts("");
    
    static const struct token_range error = { .token = TK_COUNT };
    return st_size == 1 ? stack[0] : (struct stree_node) {
        .num_children = 0,
        .tm = &error,
    };
}
