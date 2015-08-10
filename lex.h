#pragma once

#include <stdint.h>
#include <stddef.h>

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

int lex(const uint8_t *input, struct token_range *ranges, size_t *nranges);
