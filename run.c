#include <stdio.h>

#include "lex.h"
#include "parse.h"

static void run_stmt(const struct stree_node *);
static void run_prnt(const struct stree_node *);
static void run_ctrl(const struct stree_node *);
static void run_cond(const struct stree_node *);
static void run_loop(const struct stree_node *);
static int eval_atom(const struct stree_node *);
static int eval_expr(const struct stree_node *);
static int eval_pexp(const struct stree_node *);
static int eval_bexp(const struct stree_node *);

void run(const struct stree_node *unit)
{
    for (size_t stmt_idx = 1; stmt_idx < unit->num_children - 1; ++stmt_idx) {
        run_stmt(unit->children[stmt_idx]);
    }
}

static void run_stmt(const struct stree_node *stmt)
{
    switch (stmt->children[0]->nt) {
    case NT_Ctrl:
        run_ctrl(stmt->children[0]);
        break;

    case NT_Prnt:
        run_prnt(stmt->children[0]);
        break;

    case NT_Expr:
        eval_expr(stmt->children[0]);
        break;

    default:
        break;
    }
}

static void run_prnt(const struct stree_node *prnt)
{
    printf("print %d\n", eval_expr(prnt->children[1]));
}

static void run_ctrl(const struct stree_node *ctrl)
{
    switch (ctrl->children[0]->nt) {
    case NT_Cond:
        run_cond(ctrl->children[0]);
        break;

    case NT_Loop:
        run_loop(ctrl->children[0]);
        break;

    default:
        break;
    }
}

static void run_cond(const struct stree_node *cond)
{
    if (eval_expr(cond->children[1])) {
        const struct stree_node *stmt = cond->children[3];

        while (stmt->num_children) {
            run_stmt(stmt++);
        }
    }
}

static void run_loop(const struct stree_node *loop)
{
    while (eval_expr(loop->children[1])) {
        const struct stree_node *stmt = loop->children[3];

        while (stmt->num_children) {
            run_stmt(stmt++);
        }
    }
}

static int eval_atom(const struct stree_node *atom)
{
    switch (atom->children[0]->tm->token) {
    case TK_NAME:
        return 0;

    case TK_NMBR: {
        const uint8_t *beg = atom->children[0]->tm->beg;
        const uint8_t *end = atom->children[0]->tm->end;
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

static int eval_expr(const struct stree_node *expr)
{
    switch (expr->children[0]->nt) {
    case NT_Atom:
        return eval_atom(expr->children[0]);

    case NT_Bexp:
        return eval_bexp(expr->children[0]);

    case NT_Pexp:
        return eval_pexp(expr->children[0]);

    default:
        return 0;
    }
}

static int eval_pexp(const struct stree_node *pexp)
{
    return eval_expr(pexp->children[1]);
}

static int eval_bexp(const struct stree_node *bexp)
{
    switch (bexp->children[1]->tm->token) {
    case TK_ASSN:
        return 0;

    case TK_PLUS:
        return eval_expr(bexp->children[0]) + eval_expr(bexp->children[2]);

    case TK_MINS:
        return eval_expr(bexp->children[0]) - eval_expr(bexp->children[2]);

    case TK_EQUL:
        return eval_expr(bexp->children[0]) == eval_expr(bexp->children[2]);

    case TK_NEQL:
        return eval_expr(bexp->children[0]) != eval_expr(bexp->children[2]);

    default:
        return 0;
    }
}
