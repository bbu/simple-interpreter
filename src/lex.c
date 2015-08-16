#include <stdio.h>
#include <stdlib.h>

#include "lex.h"

enum {
    STS_ACCEPT,
    STS_REJECT,
    STS_HUNGRY,
};

typedef uint8_t sts_t;

#define TR(st, tr) (*s = (st), (STS_##tr))
#define REJECT TR(0, REJECT)

#define IS_ALPHA(c)  (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define IS_DIGIT(c)  ((c) >= '0' && (c) <= '9')
#define IS_ALNUM(c)  (IS_ALPHA(c) || IS_DIGIT(c))
#define IS_WSPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')

#define TOKEN_DEFINE_1(token, str) \
static sts_t token(const uint8_t c, uint8_t *const s) \
{ \
    switch (*s) { \
        case 0: return c == (str)[0] ? TR(1, ACCEPT) : REJECT; \
        case 1: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_2(token, str) \
static sts_t token(const uint8_t c, uint8_t *const s) \
{ \
    switch (*s) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, ACCEPT) : REJECT; \
        case 2: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_3(token, str) \
static sts_t token(const uint8_t c, uint8_t *const s) \
{ \
    switch (*s) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, HUNGRY) : REJECT; \
        case 2: return c == (str)[2] ? TR(3, ACCEPT) : REJECT; \
        case 3: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_4(token, str) \
static sts_t token(const uint8_t c, uint8_t *const s) \
{ \
    switch (*s) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, HUNGRY) : REJECT; \
        case 2: return c == (str)[2] ? TR(3, HUNGRY) : REJECT; \
        case 3: return c == (str)[3] ? TR(4, ACCEPT) : REJECT; \
        case 4: return REJECT; \
        default: return -1; \
    } \
}

#define TOKEN_DEFINE_5(token, str) \
static sts_t token(const uint8_t c, uint8_t *const s) \
{ \
    switch (*s) { \
        case 0: return c == (str)[0] ? TR(1, HUNGRY) : REJECT; \
        case 1: return c == (str)[1] ? TR(2, HUNGRY) : REJECT; \
        case 2: return c == (str)[2] ? TR(3, HUNGRY) : REJECT; \
        case 3: return c == (str)[3] ? TR(4, HUNGRY) : REJECT; \
        case 4: return c == (str)[4] ? TR(5, ACCEPT) : REJECT; \
        case 5: return REJECT; \
        default: return -1; \
    } \
}

static sts_t tk_name(const uint8_t c, uint8_t *const s)
{
    enum {
        tk_name_begin,
        tk_name_accum,
    };

    switch (*s) {
    case tk_name_begin:
        return IS_ALPHA(c) || (c == '_') ? TR(tk_name_accum, ACCEPT) : REJECT;

    case tk_name_accum:
        return IS_ALNUM(c) || (c == '_') ? STS_ACCEPT : REJECT;
    }

    /* unreachable, but keeps the compiler happy */
    return 0;
}

static sts_t tk_nmbr(const uint8_t c, uint8_t *const s)
{
    return IS_DIGIT(c) ? STS_ACCEPT : STS_REJECT;
}

static sts_t tk_strl(const uint8_t c, uint8_t *const s)
{
    enum {
        tk_strl_begin,
        tk_strl_accum,
        tk_strl_end,
    };

    switch (*s) {
    case tk_strl_begin:
        return c == '"' ? TR(tk_strl_accum, HUNGRY) : REJECT;

    case tk_strl_accum:
        return c != '"' ? STS_HUNGRY : TR(tk_strl_end, ACCEPT);
        
    case tk_strl_end:
        return REJECT;
    }

    /* unreachable, but keeps the compiler happy */
    return 0;
}

static sts_t tk_wspc(const uint8_t c, uint8_t *const s)
{
    enum {
        tk_wspc_begin,
        tk_wspc_accum,
    };

    switch (*s) {
    case tk_wspc_begin:
        return IS_WSPACE(c) ? TR(tk_wspc_accum, ACCEPT) : REJECT;

    case tk_wspc_accum:
        return IS_WSPACE(c) ? STS_ACCEPT : REJECT;
    }

    /* unreachable, but keeps the compiler happy */
    return 0;
}

static sts_t tk_lcom(const uint8_t c, uint8_t *const s)
{
    enum {
        tk_lcom_begin,
        tk_lcom_first_slash,
        tk_lcom_accum,
        tk_lcom_end
    };

    switch (*s) {
    case tk_lcom_begin:
        return c == '/' ? TR(tk_lcom_first_slash, HUNGRY) : REJECT;

    case tk_lcom_first_slash:
        return c == '/' ? TR(tk_lcom_accum, HUNGRY) : REJECT;

    case tk_lcom_accum:
        return c == '\n' || c == '\r' ? TR(tk_lcom_end, ACCEPT) : STS_HUNGRY;

    case tk_lcom_end:
        return REJECT;
    }

    /* unreachable, but keeps the compiler happy */
    return 0;
}

static sts_t tk_bcom(const uint8_t c, uint8_t *const s)
{
    enum {
        tk_bcom_begin,
        tk_bcom_open_slash,
        tk_bcom_accum,
        tk_bcom_close_star,
        tk_bcom_end
    };

    switch (*s) {
    case tk_bcom_begin:
        return c == '/' ? TR(tk_bcom_open_slash, HUNGRY) : REJECT;

    case tk_bcom_open_slash:
        return c == '*' ? TR(tk_bcom_accum, HUNGRY) : REJECT;

    case tk_bcom_accum:
        return c != '*' ? STS_HUNGRY : TR(tk_bcom_close_star, HUNGRY);

    case tk_bcom_close_star:
        return c == '/' ? TR(tk_bcom_end, ACCEPT) : TR(tk_bcom_accum, HUNGRY);
        
    case tk_bcom_end:
        return REJECT;
    }

    /* unreachable, but keeps the compiler happy */
    return 0;
}

TOKEN_DEFINE_1(tk_lpar, "(");
TOKEN_DEFINE_1(tk_rpar, ")");
TOKEN_DEFINE_1(tk_lbra, "[");
TOKEN_DEFINE_1(tk_rbra, "]");
TOKEN_DEFINE_1(tk_lbrc, "{");
TOKEN_DEFINE_1(tk_rbrc, "}");
TOKEN_DEFINE_2(tk_cond, "if");
TOKEN_DEFINE_4(tk_elif, "elif");
TOKEN_DEFINE_4(tk_else, "else");
TOKEN_DEFINE_2(tk_dowh, "do");
TOKEN_DEFINE_5(tk_whil, "while");
TOKEN_DEFINE_1(tk_assn, "=");
TOKEN_DEFINE_2(tk_equl, "==");
TOKEN_DEFINE_2(tk_neql, "!=");
TOKEN_DEFINE_1(tk_lthn, "<");
TOKEN_DEFINE_1(tk_gthn, ">");
TOKEN_DEFINE_2(tk_lteq, "<=");
TOKEN_DEFINE_2(tk_gteq, ">=");
TOKEN_DEFINE_2(tk_conj, "&&");
TOKEN_DEFINE_2(tk_disj, "||");
TOKEN_DEFINE_1(tk_plus, "+");
TOKEN_DEFINE_1(tk_mins, "-");
TOKEN_DEFINE_1(tk_mult, "*");
TOKEN_DEFINE_1(tk_divi, "/");
TOKEN_DEFINE_1(tk_modu, "%");
TOKEN_DEFINE_1(tk_nega, "!");
TOKEN_DEFINE_5(tk_prnt, "print");
TOKEN_DEFINE_1(tk_scol, ";");
TOKEN_DEFINE_1(tk_ques, "?");
TOKEN_DEFINE_1(tk_coln, ":");

static sts_t (*const tokens[TK_COUNT])(const uint8_t, uint8_t *const) = {
    tk_name,
    tk_nmbr,
    tk_strl,
    tk_wspc,
    tk_lcom,
    tk_bcom,
    tk_lpar,
    tk_rpar,
    tk_lbra,
    tk_rbra,
    tk_lbrc,
    tk_rbrc,
    tk_cond,
    tk_elif,
    tk_else,
    tk_dowh,
    tk_whil,
    tk_assn,
    tk_equl,
    tk_neql,
    tk_lthn,
    tk_gthn,
    tk_lteq,
    tk_gteq,
    tk_conj,
    tk_disj,
    tk_plus,
    tk_mins,
    tk_mult,
    tk_divi,
    tk_modu,
    tk_nega,
    tk_prnt,
    tk_scol,
    tk_ques,
    tk_coln,
};

static inline int push_token(
    struct token **const ranges, 
    size_t *const nranges,
    size_t *const allocated,
    const tk_t token, 
    const uint8_t *const beg, 
    const uint8_t *const end)
{
    if (*nranges + 1 > *allocated) {
        *allocated = (*allocated ?: 1) * 8;

        struct token *const tmp = 
            realloc(*ranges, *allocated * sizeof(struct token));

        if (tmp == NULL) {
            free(*ranges);
            *ranges = NULL;
            return -1;
        } else {
            *ranges = tmp;                
        }
    }

    (*ranges)[(*nranges)++] = (struct token) {
        .beg = beg, 
        .end = end, 
        .tk = token
    };

    return 0;
}

int lex(const uint8_t *const input, 
    struct token **const ranges, size_t *const nranges)
{
    static struct {
        sts_t prev, curr;
    } statuses[TK_COUNT] = {
        [0 ... TK_COUNT - 1] = { STS_HUNGRY, STS_REJECT }
    };
    
    uint8_t states[TK_COUNT] = {0};

    const uint8_t *prefix_beg = input, *prefix_end = input;
    tk_t accepted_token;
    size_t allocated = 0;
    *ranges = NULL, *nranges = 0;
    
    #define PUSH_OR_NOMEM(tk, beg, end) \
        if (push_token(ranges, nranges, &allocated, (tk), (beg), (end))) { \
            return LEX_NOMEM; \
        }

    #define foreach_token \
        for (tk_t token = 0; token < TK_COUNT; ++token)

    PUSH_OR_NOMEM(TK_FBEG, NULL, NULL);

    while (*prefix_end) {
        int did_accept = 0;
        
        foreach_token {
            if (statuses[token].prev != STS_REJECT) {
                statuses[token].curr = tokens[token](*prefix_end, &states[token]);
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

            PUSH_OR_NOMEM(accepted_token, prefix_beg, prefix_end);

            if (accepted_token == TK_COUNT) {
                return LEX_UNKNOWN_TOKEN;
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

    PUSH_OR_NOMEM(accepted_token, prefix_beg, prefix_end);

    if (accepted_token == TK_COUNT) {
        return LEX_UNKNOWN_TOKEN;
    }

    PUSH_OR_NOMEM(TK_FEND, NULL, NULL);
    return LEX_OK;
    
    #undef PUSH_OR_NOMEM
    #undef foreach_token
}
