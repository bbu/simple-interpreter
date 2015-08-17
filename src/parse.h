#pragma once

#include <stdint.h>
#include <stddef.h>

enum {
    NT_Unit,
    NT_Stmt,
    NT_Assn,
    NT_Prnt,
    NT_Ctrl,
    NT_Cond,
    NT_Elif,
    NT_Else,
    NT_Dowh,
    NT_Whil,
    NT_Atom,
    NT_Expr,
    NT_Pexp,
    NT_Bexp,
    NT_Uexp,
    NT_Texp,
    NT_Aexp,
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

struct node parse(const struct token *, size_t);

enum {
    PARSE_OK,
    PARSE_REJECT,
    PARSE_NOMEM,
    PARSE_OVERFLOW,
};

#define parse_error(root) ({ \
    struct node root_once = (root); \
    root_once.nchildren ? PARSE_OK : root_once.token->tk; \
})

void destroy_tree(struct node);
