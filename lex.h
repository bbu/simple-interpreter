#pragma once

#include <stdint.h>
#include <stddef.h>

enum {
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
    TK_LTHN,
    TK_GTHN,
    TK_LTEQ,
    TK_GTEQ,
    TK_CONJ,
    TK_DISJ,
    TK_PLUS,
    TK_MINS,
    TK_NEGA,
    TK_READ,
    TK_PRNT,
    TK_SCOL,
    TK_COUNT,
    TK_FBEG,
    TK_FEND,
};

typedef uint8_t tk_t;

struct token {
    const uint8_t *beg, *end;
    tk_t tk;
};

int lex(const uint8_t *input, struct token *ranges, size_t *nranges);
