#pragma once

#include <stdint.h>
#include <stddef.h>

#define COLOURED(s, c) "\033[1;" #c "m" s "\033[0m"
#define RED(s) COLOURED(s, 31)
#define GREEN(s) COLOURED(s, 32)
#define YELLOW(s) COLOURED(s, 33)
#define ORANGE(s) COLOURED(s, 34)
#define CYAN(s) COLOURED(s, 36)
#define WHITE(s) COLOURED(s, 37)

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
    NT_COUNT
};

typedef uint8_t non_terminal_t;

struct token_range;

struct stree_node {
    uint32_t num_children;

    union {
        const struct token_range *tm;
        non_terminal_t nt;
    };

    struct stree_node **children;
};

struct stree_node parse(const struct token_range *ranges, size_t nranges);
