#pragma once

#include <stdint.h>
#include <stddef.h>

#define COLOURED(s, b, c) "\033[" #b ";" #c "m" s "\033[0m"
#define GRAY(s)           COLOURED(s, 0, 37)
#define RED(s)            COLOURED(s, 1, 31)
#define GREEN(s)          COLOURED(s, 1, 32)
#define YELLOW(s)         COLOURED(s, 1, 33)
#define ORANGE(s)         COLOURED(s, 1, 34)
#define CYAN(s)           COLOURED(s, 1, 36)
#define WHITE(s)          COLOURED(s, 1, 37)

enum {
    TK_NAME,
    TK_NMBR,
    TK_STRL,
    TK_WSPC,
    TK_LCOM,
    TK_BCOM,
    TK_LPAR,
    TK_RPAR,
    TK_LBRA,
    TK_RBRA,
    TK_LBRC,
    TK_RBRC,
    TK_COND,
    TK_ELIF,
    TK_ELSE,
    TK_DOWH,
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
    TK_MULT,
    TK_DIVI,
    TK_MODU,
    TK_NEGA,
    TK_PRNT,
    TK_SCOL,
    TK_QUES,
    TK_COLN,
    TK_COUNT,
    TK_FBEG,
    TK_FEND,
};

typedef uint8_t tk_t;

struct token {
    const uint8_t *beg, *end;
    tk_t tk;
};

int lex(const uint8_t *, struct token **, size_t *);

enum {
    LEX_OK,
    LEX_NOMEM,
    LEX_UNKNOWN_TOKEN,
};
