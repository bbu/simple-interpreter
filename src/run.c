#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"
#include "parse.h"

static void run_stmt(const struct node *const);
static void run_assn(const struct node *const);
static void run_prnt(const struct node *const);
static void run_ctrl(const struct node *const);
static int eval_atom(const struct node *const);
static int eval_expr(const struct node *const);
static int eval_pexp(const struct node *const);
static int eval_bexp(const struct node *const);
static int eval_uexp(const struct node *const);
static int eval_texp(const struct node *const);
static int eval_aexp(const struct node *const);

#define VARSTORE_CAPACITY 128

static struct {
    size_t size;

    struct {
        const uint8_t *beg;
        ptrdiff_t len;
        size_t array_size;
        int *values;
    } vars[VARSTORE_CAPACITY];
} varstore;

void run(const struct node *const unit)
{
    for (size_t stmt_idx = 1; stmt_idx < unit->nchildren - 1; ++stmt_idx) {
        run_stmt(unit->children[stmt_idx]);
    }
    
    for (size_t var_idx = 0; var_idx < varstore.size; ++var_idx) {
        free(varstore.vars[var_idx].values);
    }

    varstore.size = 0;
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
    const int lhs_is_aexp = assn->children[0]->nchildren;

    const int array_idx = lhs_is_aexp ? 
        eval_expr(assn->children[0]->children[2]) : 0;

    const uint8_t *const beg = lhs_is_aexp ? 
        assn->children[0]->children[0]->token->beg : 
        assn->children[0]->token->beg;

    const ptrdiff_t len = lhs_is_aexp ? 
        assn->children[0]->children[0]->token->end - beg : 
        assn->children[0]->token->end - beg;
    
    size_t var_idx;
    
    for (var_idx = 0; var_idx < varstore.size; ++var_idx) {
        if (varstore.vars[var_idx].len == len &&
            !memcmp(varstore.vars[var_idx].beg, beg, len)) {

            const size_t array_size = varstore.vars[var_idx].array_size;

            if (!array_size) {
                return fprintf(stderr, "warn: a previous reallocation has failed, "
                    "assignment has no effect\n"), (void) 0;
            }
            
            if (array_idx >= 0 && array_idx < array_size) {
                varstore.vars[var_idx].values[array_idx] = 
                    eval_expr(assn->children[2]);
                
                return;
            } else if (array_idx >= 0) {
                const size_t new_size = (array_idx + 1) * 2;
                
                int *const tmp = realloc(
                    varstore.vars[var_idx].values, new_size * sizeof(int));

                if (!tmp) {
                    free(varstore.vars[var_idx].values);                    
                    varstore.vars[var_idx].array_size = 0;
                    varstore.vars[var_idx].values = NULL;
                    return perror("realloc"), (void) 0;
                }

                varstore.vars[var_idx].values = tmp;
                varstore.vars[var_idx].array_size = new_size;
                varstore.vars[var_idx].values[array_idx] = 
                    eval_expr(assn->children[2]);
                
                return;
            } else {
                return fprintf(stderr, "warn: negative array offset\n"), (void) 0;
            }
        }
    }
    
    if (var_idx < VARSTORE_CAPACITY) {
        if (array_idx < 0) {
            return fprintf(stderr, "warn: negative array offset\n"), (void) 0;
        }
    
        varstore.vars[var_idx].beg = beg;
        varstore.vars[var_idx].len = len;
        varstore.vars[var_idx].values = malloc((array_idx + 1) * sizeof(int));
        varstore.vars[var_idx].array_size = 0;
        
        if (!varstore.vars[var_idx].values) {
            return perror("malloc"), (void) 0;
        }
        
        varstore.vars[var_idx].array_size = array_idx + 1;
        varstore.vars[var_idx].values[array_idx] = eval_expr(assn->children[2]);
        varstore.size++;
    } else {
        fprintf(stderr, "warn: varstore exhausted, assignment has no effect\n");
    }
}

static void run_prnt(const struct node *const prnt)
{
    if (prnt->nchildren == 3) {
        printf("%d\n", eval_expr(prnt->children[1]));
    } else if (prnt->nchildren == 4) {
        struct node *strl = prnt->children[1];

        const uint8_t *const beg = strl->token->beg + 1;
        const uint8_t *const end = strl->token->end - 1;
        const ptrdiff_t len = end - beg;
        
        printf("%.*s%d\n", (int) len, beg, eval_expr(prnt->children[2]));
    }
}

static void run_ctrl(const struct node *const ctrl)
{
    switch (ctrl->children[0]->nt) {
    case NT_Cond: {
        const struct node *const cond = ctrl->children[0];
    
        if (eval_expr(cond->children[1])) {
            const struct node *stmt = cond->children[3];

            while (stmt->nchildren) {
                run_stmt(stmt++);
            }
        } else if (ctrl->nchildren >= 2) {
            size_t child_idx = 1;

            do {
                if (ctrl->children[child_idx]->nt == NT_Elif) {
                    const struct node *const elif = ctrl->children[child_idx];
                    
                    if (eval_expr(elif->children[1])) {
                        const struct node *stmt = elif->children[3];
                    
                        while (stmt->nchildren) {
                            run_stmt(stmt++);
                        }
                        
                        break;
                    }
                } else {
                    const struct node *const els = ctrl->children[child_idx];
                    const struct node *stmt = els->children[2];

                    while (stmt->nchildren) {
                        run_stmt(stmt++);
                    }                    
                }
            } while (++child_idx < ctrl->nchildren);
        }
    } break;

    case NT_Dowh: {
        const struct node *const dowh = ctrl->children[0];
        const struct node *const expr = dowh->children[dowh->nchildren - 2];

        do {
            const struct node *stmt = dowh->children[2];
            
            while (stmt->nchildren) {
                run_stmt(stmt++);
            }
        } while (eval_expr(expr));
    } break;

    case NT_Whil: {
        const struct node *const whil = ctrl->children[0];

        while (eval_expr(whil->children[1])) {
            const struct node *stmt = whil->children[3];

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
        const uint8_t *const beg = atom->children[0]->token->beg;
        const ptrdiff_t len = atom->children[0]->token->end - beg;

        for (size_t idx = 0; idx < varstore.size; ++idx) {
            if (varstore.vars[idx].len == len &&
                !memcmp(varstore.vars[idx].beg, beg, len)) {

                if (varstore.vars[idx].array_size) {
                    return varstore.vars[idx].values[0];
                } else {
                    return 0;
                }
            }
        }

        return fprintf(stderr, "warn: access to undefined variable\n"), 0;
    }
    
    case TK_NMBR: {
        const uint8_t *const beg = atom->children[0]->token->beg;
        const uint8_t *const end = atom->children[0]->token->end;
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

    case NT_Aexp:
        return eval_aexp(expr->children[0]);

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

    case TK_DIVI: {
        const int dividend = eval_expr(bexp->children[0]);
        const int divisor = eval_expr(bexp->children[2]);
        
        if (divisor) {
            return dividend / divisor;
        } else {
            fprintf(stderr, "warn: prevented attempt to divide by zero\n");
            return 0;
        }
    }
    
    case TK_MODU:
        return eval_expr(bexp->children[0]) % eval_expr(bexp->children[2]);

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

static int eval_aexp(const struct node *const aexp)
{
    const uint8_t *const beg = aexp->children[0]->token->beg;
    const ptrdiff_t len = aexp->children[0]->token->end - beg;
    const int array_idx = eval_expr(aexp->children[2]);

    if (array_idx < 0) {
        return fprintf(stderr, "warn: negative array offset\n"), 0;
    }

    for (size_t idx = 0; idx < varstore.size; ++idx) {
        if (varstore.vars[idx].len == len &&
            !memcmp(varstore.vars[idx].beg, beg, len)) {

            if (array_idx < varstore.vars[idx].array_size) {
                return varstore.vars[idx].values[array_idx];
            } else {
                return fprintf(stderr, "warn: out of bounds array access\n"), 0;
            }
        }
    }
    
    return fprintf(stderr, "warn: access to undefined array\n"), 0;
}
