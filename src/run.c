#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"
#include "parse.h"

static void run_stmt(const struct node *);
static void run_assn(const struct node *);
static void run_prnt(const struct node *);
static void run_ctrl(const struct node *);
static int eval_atom(const struct node *);
static int eval_expr(const struct node *);
static int eval_pexp(const struct node *);
static int eval_bexp(const struct node *);
static int eval_uexp(const struct node *);
static int eval_texp(const struct node *);

#define VARSTORE_CAPACITY 128

static struct {
    size_t size;

    struct {
        const uint8_t *beg;
        ptrdiff_t len;
        int val;
    } values[VARSTORE_CAPACITY];
} varstore;

void run(const struct node *const unit)
{
    varstore.size = 0;

    for (size_t stmt_idx = 1; stmt_idx < unit->nchildren - 1; ++stmt_idx) {
        run_stmt(unit->children[stmt_idx]);
    }
}

static void run_stmt(const struct node *const stmt)
{
    switch (stmt->children[0]->nt) {
    case NT_Assn:
        run_assn(stmt->children[0]);
        break;

    case NT_Prnt:
        run_prnt(stmt->children[0]);
        break;

    case NT_Ctrl:
        run_ctrl(stmt->children[0]);
        break;

    default:
        break;
    }
}

static void run_assn(const struct node *const assn)
{
    const uint8_t *beg = assn->children[0]->token->beg;
    ptrdiff_t len = assn->children[0]->token->end - beg;
    size_t idx;
    
    for (idx = 0; idx < varstore.size; ++idx) {
        if (varstore.values[idx].len == len) {
            if (!memcmp(varstore.values[idx].beg, beg, len)) {
                varstore.values[idx].val = eval_expr(assn->children[2]);
                return;
            }
        }
    }
    
    if (idx < VARSTORE_CAPACITY) {
        varstore.values[idx].val = eval_expr(assn->children[2]);
        varstore.values[idx].beg = beg;
        varstore.values[idx].len = len;
        
        varstore.size++;
    } else {
        fprintf(stderr, "exception: varstore exhausted\n");
    }
}

static void run_prnt(const struct node *const prnt)
{
    printf("print: %d\n", eval_expr(prnt->children[1]));
}

static void run_ctrl(const struct node *const ctrl)
{
    switch (ctrl->children[0]->nt) {
    case NT_Cond: {
        const struct node *cond = ctrl->children[0];
    
        if (eval_expr(cond->children[1])) {
            const struct node *stmt = cond->children[3];

            while (stmt->nchildren) {
                run_stmt(stmt++);
            }
        } else if (ctrl->nchildren >= 2) {
            size_t child_idx = 1;

            do {
                if (ctrl->children[child_idx]->nt == NT_Elif) {
                    const struct node *elif = ctrl->children[child_idx];
                    
                    if (eval_expr(elif->children[1])) {
                        const struct node *stmt = elif->children[3];
                    
                        while (stmt->nchildren) {
                            run_stmt(stmt++);
                        }
                        
                        break;
                    }
                } else {
                    const struct node *els = ctrl->children[child_idx];
                    const struct node *stmt = els->children[2];
                    
                    while (stmt->nchildren) {
                        run_stmt(stmt++);
                    }                    
                }
            } while (++child_idx < ctrl->nchildren);
        }
    } break;

    case NT_Loop: {
        const struct node *loop = ctrl->children[0];

        while (eval_expr(loop->children[1])) {
            const struct node *stmt = loop->children[3];

            while (stmt->nchildren) {
                run_stmt(stmt++);
            }
        }
    } break;

    default:
        break;
    }
}

static int eval_atom(const struct node *const atom)
{
    switch (atom->children[0]->token->tk) {
    case TK_NAME: {
        const uint8_t *beg = atom->children[0]->token->beg;
        ptrdiff_t len = atom->children[0]->token->end - beg;

        for (size_t idx = 0; idx < varstore.size; ++idx) {
            if (varstore.values[idx].len == len) {
                if (!memcmp(varstore.values[idx].beg, beg, len)) {
                    return varstore.values[idx].val;
                }
            }
        }

        return 0;
    }
    
    case TK_NMBR: {
        const uint8_t *beg = atom->children[0]->token->beg;
        const uint8_t *end = atom->children[0]->token->end;
        int result = 0, mult = 1;

        for (ssize_t idx = end - beg - 1; idx >= 0; --idx, mult *= 10) {
            result += mult * (beg[idx] - '0');
        }

        return result;
    }

    default:
        return 0;
    }
}

static int eval_expr(const struct node *const expr)
{
    switch (expr->children[0]->nt) {
    case NT_Atom:
        return eval_atom(expr->children[0]);

    case NT_Pexp:
        return eval_pexp(expr->children[0]);

    case NT_Bexp:
        return eval_bexp(expr->children[0]);

    case NT_Uexp:
        return eval_uexp(expr->children[0]);
        
    case NT_Texp:
        return eval_texp(expr->children[0]);

    default:
        return 0;
    }
}

static int eval_pexp(const struct node *const pexp)
{
    return eval_expr(pexp->children[1]);
}

static int eval_bexp(const struct node *const bexp)
{
    switch (bexp->children[1]->token->tk) {
    case TK_PLUS:
        return eval_expr(bexp->children[0]) + eval_expr(bexp->children[2]);

    case TK_MINS:
        return eval_expr(bexp->children[0]) - eval_expr(bexp->children[2]);

    case TK_MULT:
        return eval_expr(bexp->children[0]) * eval_expr(bexp->children[2]);

    case TK_DIVD:
        return eval_expr(bexp->children[0]) / eval_expr(bexp->children[2]);

    case TK_EQUL:
        return eval_expr(bexp->children[0]) == eval_expr(bexp->children[2]);

    case TK_NEQL:
        return eval_expr(bexp->children[0]) != eval_expr(bexp->children[2]);

    case TK_LTHN:
        return eval_expr(bexp->children[0]) < eval_expr(bexp->children[2]);

    case TK_GTHN:
        return eval_expr(bexp->children[0]) > eval_expr(bexp->children[2]);

    case TK_LTEQ:
        return eval_expr(bexp->children[0]) <= eval_expr(bexp->children[2]);

    case TK_GTEQ:
        return eval_expr(bexp->children[0]) >= eval_expr(bexp->children[2]);

    case TK_CONJ:
        return eval_expr(bexp->children[0]) && eval_expr(bexp->children[2]);

    case TK_DISJ:
        return eval_expr(bexp->children[0]) || eval_expr(bexp->children[2]);

    default:
        return 0;
    }
}

static int eval_uexp(const struct node *const uexp)
{
    switch (uexp->children[0]->token->tk) {
    case TK_PLUS:
        return eval_expr(uexp->children[1]);

    case TK_MINS:
        return -eval_expr(uexp->children[1]);

    case TK_NEGA:
        return !eval_expr(uexp->children[1]);

    default:
        return 0;
    }
}

static int eval_texp(const struct node *const texp)
{
    return eval_expr(texp->children[0]) ? 
        eval_expr(texp->children[2]) : eval_expr(texp->children[4]);
}
