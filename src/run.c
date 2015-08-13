#include <stdio.h>

#include "lex.h"
#include "parse.h"

static void run_stmt(const struct node *);
static void run_assn(const struct node *);
static void run_read(const struct node *);
static void run_prnt(const struct node *);
static void run_ctrl(const struct node *);
static int eval_atom(const struct node *);
static int eval_expr(const struct node *);
static int eval_pexp(const struct node *);
static int eval_bexp(const struct node *);
static int eval_uexp(const struct node *);
static int eval_texp(const struct node *);

void run(const struct node *unit)
{
    for (size_t stmt_idx = 1; stmt_idx < unit->nchildren - 1; ++stmt_idx) {
        run_stmt(unit->children[stmt_idx]);
    }
}

static void run_stmt(const struct node *stmt)
{
    switch (stmt->children[0]->nt) {
    case NT_Assn:
        run_assn(stmt->children[0]);
        break;

    case NT_Read:
        run_read(stmt->children[0]);
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

static void run_assn(const struct node *assn)
{
    printf("assign\n");
}

static void run_read(const struct node *read)
{
    int input;

    printf("read: ");
    scanf("%d", &input);
}

static void run_prnt(const struct node *prnt)
{
    printf("print: %d\n", eval_expr(prnt->children[1]));
}

static void run_ctrl(const struct node *ctrl)
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

static int eval_atom(const struct node *atom)
{
    switch (atom->children[0]->token->tk) {
    case TK_NAME:
        return 0;

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

static int eval_expr(const struct node *expr)
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

static int eval_pexp(const struct node *pexp)
{
    return eval_expr(pexp->children[1]);
}

static int eval_bexp(const struct node *bexp)
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

static int eval_uexp(const struct node *uexp)
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

static int eval_texp(const struct node *texp)
{
    return eval_expr(texp->children[0]) ? 
        eval_expr(texp->children[2]) : eval_expr(texp->children[4]);
}
