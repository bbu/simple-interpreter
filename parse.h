#pragma once

#include <stdint.h>
#include <stddef.h>

#define COLOURED(s, c) "\033[1;" #c "m" s "\033[0m"
#define RED(s)         COLOURED(s, 31)
#define GREEN(s)       COLOURED(s, 32)
#define YELLOW(s)      COLOURED(s, 33)
#define ORANGE(s)      COLOURED(s, 34)
#define CYAN(s)        COLOURED(s, 36)
#define WHITE(s)       COLOURED(s, 37)

enum {
    NT_Unit,
    NT_Stmt,
    NT_Assn,
    NT_Read,
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

typedef uint8_t nt_t;

struct token;
struct node {
    /* use "token" if nchildren == 0, "nt" and "children" otherwise */
    uint32_t nchildren;

    union {
        const struct token *token;

        struct {
            nt_t nt;
            struct node **children;
        };
    };
};

struct node parse(const struct token *ranges, const size_t nranges);

#define parse_success(root) ((root).nchildren)
