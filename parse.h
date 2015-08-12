#pragma once

#include <stdint.h>
#include <stddef.h>

enum {
    NT_Unit,
    NT_Stmt,
    NT_Assn,
    NT_Read,
    NT_Prnt,
    NT_Ctrl,
    NT_Cond,
    NT_Elif,
    NT_Else,
    NT_Loop,
    NT_Atom,
    NT_Expr,
    NT_Pexp,
    NT_Bexp,
    NT_Uexp,
    NT_Texp,
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
void destroy_tree(struct node root);

#define parse_success(root) ((root).nchildren)
