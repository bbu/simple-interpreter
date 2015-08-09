#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

enum token {
    TK_NAME,
    TK_NMBR,
    TK_WSPC,
    TK_LPAR,
    TK_RPAR,
    TK_LBRC,
    TK_RBRC,
    TK_COND,
    TK_WHIL,
    TK_ASSN,
    TK_EQUL,
    TK_NEQL,
    TK_PLUS,
    TK_MINS,
    TK_PRNT,
    TK_SCOL,
    TK_COUNT,
    TK_FBEG,
    TK_FEND,
};

typedef uint8_t token_t;

struct token_range {
    const uint8_t *beg, *end;
    token_t token;
};

enum state_transition_status {
    STS_ACCEPT,
    STS_REJECT,
    STS_HUNGRY,
};

typedef uint8_t sts_t;

#define TR(st, tr) (state = (st), (STS_##tr))
#define REJECT TR(0, REJECT)

#define IS_ALPHA(c)  (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_DIGIT(c)  ((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c)  (IS_ALPHA(c) || IS_DIGIT(c))
#define IS_WSPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

static const char *nts[] = {
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

enum non_terminal {
    NT_Unit,
    NT_Stmt,
    NT_Prnt,
    NT_Ctrl,
    NT_Cond,
    NT_Loop,
    NT_Atom,
    NT_Expr,
    NT_Pexp,
    NT_Bexp,
};

typedef uint8_t non_terminal_t;

struct term {
    union {
        token_t tm;
        non_terminal_t nt;
    };

    uint8_t is_terminal: 1;
    uint8_t is_multiplier: 1;
};

#define RULE_RHS_LAST 8

struct rule {
    non_terminal_t lhs;
    struct term rhs[RULE_RHS_LAST + 1];
};

struct stree_node {
    uint32_t num_children;

    union {
        const struct token_range *tm;
        non_terminal_t nt;
    };

    struct stree_node **children;
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

void evaluate(struct stree_node *top)
{
}

static inline int term_equals_node(const struct term *term, const struct stree_node *node)
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
                printf("\033[1;32m^\033[0m ");
            } else if (stack[i].tm->token == TK_FEND) {
                printf("\033[1;32m$\033[0m ");
            } else {
                printf("\033[1;32m%.*s\033[0m ", (int)(stack[i].tm->end - stack[i].tm->beg), stack[i].tm->beg);
            }
        } else {
            printf("\033[1;33mNT_%s\033[0m ", nts[stack[i].nt]);
        }
    }

    printf("\n");
}

struct stree_node *parse(const struct token_range *ranges, size_t nranges)
{
    #define STACK_MAX_DEPTH 256
    ssize_t st_size = 0;
    static struct stree_node stack[STACK_MAX_DEPTH];

    for (size_t range_idx = 0; range_idx < nranges; ++range_idx) {
        if (ranges[range_idx].token == TK_WSPC) {
            continue;
        }

        if (st_size > STACK_MAX_DEPTH - 1) {
            printf("\033[1;31mStack depth exceeded!\033[0m\n");
            break;
        }

        stack[st_size++] = (struct stree_node) {
            .num_children = 0,
            .tm = &ranges[range_idx],
            .children = NULL
        };

        printf("\033[1;36mShift:\033[0m ");
        print_stack(stack, st_size);
        
        try_reduce_again:

        for (const struct rule *rule = grammar; 
            rule != grammar + sizeof(grammar) / sizeof(*grammar); 
            ++rule) {

            const struct term *prev_term = NULL, *term = &rule->rhs[RULE_RHS_LAST];
            ssize_t st_idx = st_size - 1;

            do {
                if (term_equals_node(term, &stack[st_idx])) {
                    prev_term = term->is_multiplier ? term : NULL;
                    --term, --st_idx;
                } else if (prev_term && term_equals_node(prev_term, &stack[st_idx])) {
                    --st_idx;
                } else if (term->is_multiplier) {
                    prev_term = NULL;
                    --term;
                } else {
                    term = NULL;
                    break;
                }
            } while (st_idx >= 0 && !(term->is_terminal && term->tm == TK_COUNT));

            if (term && term->is_terminal && term->tm == TK_COUNT && st_idx != st_size - 1) {
                size_t reduction_size = st_size - st_idx - 1;
                size_t reduction_idx = ++st_idx;

                struct stree_node *child_nodes = malloc(reduction_size * sizeof(struct stree_node));
                struct stree_node **old_children = stack[reduction_idx].children;
                stack[reduction_idx].children = malloc(reduction_size * sizeof(struct stree_node *));

                for (size_t node_idx = 0; st_idx < st_size; ++st_idx, ++node_idx) {
                    child_nodes[node_idx] = stack[st_idx];
                    stack[reduction_idx].children[node_idx] = &child_nodes[node_idx];
                }

                child_nodes[0].children = old_children;
                stack[reduction_idx].num_children = reduction_size;
                stack[reduction_idx].nt = rule->lhs;
                st_size = reduction_idx + 1;

                printf("\033[1;34mRed%02td:\033[0m ", rule - grammar + 1);
                print_stack(stack, st_size);

                goto try_reduce_again;
            }
        }
    }

    printf("\033[1;%sm%s\033[0m ", st_size == 1 ? "32" : "31", st_size == 1 ? "ACCEPT" : "REJECT");
    print_stack(stack, st_size);

    return st_size == 1 ? stack : NULL;

    #undef STACK_MAX_DEPTH
}

#define TOKEN_DEFINE_1(token, str) \
static sts_t token(uint8_t c) \
{ \
    static char state; \
    \
    switch (state) { \
        case 0: return c == (str)[0] ? TR(1, ACCEPT) : REJECT; \
        case 1: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_2(token, str) \
static sts_t token(uint8_t c) \
{ \
    static char state; \
    \
    switch (state) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, ACCEPT) : REJECT; \
        case 2: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_3(token, str) \
static sts_t token(uint8_t c) \
{ \
    static char state; \
    \
    switch (state) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, HUNGRY) : REJECT; \
        case 2: return c == (str)[2] ? TR(3, ACCEPT) : REJECT; \
        case 3: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_4(token, str) \
static sts_t token(uint8_t c) \
{ \
    static char state; \
    \
    switch (state) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, HUNGRY) : REJECT; \
        case 2: return c == (str)[2] ? TR(3, HUNGRY) : REJECT; \
        case 3: return c == (str)[3] ? TR(4, ACCEPT) : REJECT; \
        case 4: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_5(token, str) \
static sts_t token(uint8_t c) \
{ \
    static char state; \
    \
    switch (state) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, HUNGRY) : REJECT; \
        case 2: return c == (str)[2] ? TR(3, HUNGRY) : REJECT; \
        case 3: return c == (str)[3] ? TR(4, HUNGRY) : REJECT; \
        case 4: return c == (str)[4] ? TR(5, ACCEPT) : REJECT; \
        case 5: return REJECT; \
        default: return -1; \
    } \
}

static sts_t tk_name(uint8_t c)
{
    static enum {
        tk_name_begin,
        tk_name_accum,
    } state;

    switch (state) {
    case tk_name_begin:
        return IS_ALPHA(c) || (c == '_') ? TR(tk_name_accum, ACCEPT) : REJECT;

    case tk_name_accum:
        return IS_ALNUM(c) || (c == '_') ? STS_ACCEPT : REJECT;
    }

    /* unreachable, but keeps the compiler happy */
    return 0;
}

static sts_t tk_number(uint8_t c)
{
    return IS_DIGIT(c) ? STS_ACCEPT : STS_REJECT;
}

static sts_t tk_wspace(uint8_t c)
{
    static enum {
        tk_wspace_begin,
        tk_wspace_accum,
    } state;

    switch (state) {
    case tk_wspace_begin:
        return IS_WSPACE(c) ? TR(tk_wspace_accum, ACCEPT) : REJECT;

    case tk_wspace_accum:
        return IS_WSPACE(c) ? STS_ACCEPT : REJECT;
    }

    /* unreachable, but keeps the compiler happy */
    return 0;
}

TOKEN_DEFINE_1(tk_lparen, "(");
TOKEN_DEFINE_1(tk_rparen, ")");
TOKEN_DEFINE_1(tk_lbrace, "{");
TOKEN_DEFINE_1(tk_rbrace, "}");
TOKEN_DEFINE_2(tk_if,     "if");
TOKEN_DEFINE_5(tk_while,  "while");
TOKEN_DEFINE_1(tk_assign, "=");
TOKEN_DEFINE_2(tk_equal,  "==");
TOKEN_DEFINE_2(tk_nequal, "!=");
TOKEN_DEFINE_1(tk_plus,   "+");
TOKEN_DEFINE_1(tk_minus,  "-");
TOKEN_DEFINE_5(tk_print,  "print");
TOKEN_DEFINE_1(tk_scolon, ";");

static sts_t (*tokens[TK_COUNT])(uint8_t) = {
    tk_name,
    tk_number,
    tk_wspace,
    tk_lparen,
    tk_rparen,
    tk_lbrace,
    tk_rbrace,
    tk_if,
    tk_while,
    tk_assign,
    tk_equal,
    tk_nequal,
    tk_plus,
    tk_minus,
    tk_print,
    tk_scolon,
};

int lex(const uint8_t *input, struct token_range *ranges, size_t *nranges)
{
    static struct {
        sts_t prev, curr;
    } statuses[TK_COUNT] = {
        [0 ... TK_COUNT - 1] = { STS_HUNGRY, STS_REJECT }
    };

    #define foreach_token \
        for (token_t token = 0; token < TK_COUNT; ++token)

    const uint8_t *prefix_beg = input, *prefix_end = input;
    token_t accepted_token;
    ranges[*nranges = 1, 0].token = TK_FBEG;

    while (*prefix_end) {
        int did_accept = 0;
        
        foreach_token {
            if (statuses[token].prev != STS_REJECT) {
                statuses[token].curr = tokens[token](*prefix_end);
            }

            if (statuses[token].curr != STS_REJECT) {
                did_accept = 1;
            }
        }

        if (!did_accept) {
            accepted_token = TK_COUNT;

            foreach_token {
                if (statuses[token].prev == STS_ACCEPT) {
                    accepted_token = token;
                }

                statuses[token].prev = STS_HUNGRY;
                statuses[token].curr = STS_REJECT;
            }

            ranges[(*nranges)++] = (struct token_range) {
                .beg = prefix_beg, 
                .end = prefix_end, 
                .token = accepted_token
            };

            if (accepted_token == TK_COUNT) {
                return -1;
            } else {
                prefix_beg = prefix_end;
            }
        } else {
            prefix_end++;

            foreach_token {
                statuses[token].prev = statuses[token].curr;
            }
        }
    }

    accepted_token = TK_COUNT;

    foreach_token {
        if (statuses[token].curr == STS_ACCEPT) {
            accepted_token = token;
        }
        
        statuses[token].prev = STS_HUNGRY;
        statuses[token].curr = STS_REJECT;
    }

    ranges[(*nranges)++] = (struct token_range) {
        .beg = prefix_beg, 
        .end = prefix_end, 
        .token = accepted_token
    };

    if (accepted_token == TK_COUNT) {
        return -1;
    }

    ranges[(*nranges)++].token = TK_FEND;
    return 0;
    #undef foreach_token
}

struct token_range ranges[200];
size_t nranges;

int main (int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 127;
    }

    int fd = open(argv[1], O_RDONLY);

    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct stat s;
    int status = fstat(fd, &s);

    if (status < 0) {
        perror("fstat");
        return -1;
    }

    size_t size = s.st_size;
    const uint8_t *mapped = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (mapped == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    int lexresult = lex(mapped, ranges, &nranges);
    printf("\033[1;37m*** Lexing ***\033[0m\n");

    for (size_t i = 0, alternate = 0; i < nranges; ++i) {
        struct token_range range = ranges[i];

        if (range.token == TK_FBEG || range.token == TK_FEND) {
            continue;
        }

        if (range.token != TK_WSPC) {
            alternate++;
        }

        int length = range.end - range.beg;

        if (i == nranges - 1 && lexresult) {
            printf("\033[1;31m%.*s\033[0m\033[1;36m < Unknown token\033[0m\n", length ?: 1, range.beg);
        } else {
            if (alternate % 2) {
                printf("\033[1;33m%.*s\033[0m", length, range.beg);
            } else {
                printf("\033[1;32m%.*s\033[0m", length, range.beg);
            }
        }
    }

    printf("\n");
    printf("\033[1;37m*** Parsing ***\033[0m\n");
    struct stree_node *stree = parse(ranges, nranges);

    if (stree) {
    }

    return 0;
}
