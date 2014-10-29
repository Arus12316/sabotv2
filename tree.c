#include "tree.h"
#include "parse.h"
#include "general.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LEFT(n) n->children[0]
#define RIGHT(n) n->children[1]
#define FIRST(n) LEFT(n)

typedef struct state_s state_s;

struct state_s
{
    scope_s *scope;
};

static void walk_statements(state_s *state, node_s *stmt);

static void walk_expression(state_s *state, node_s *exp);
static void walk_shiftop(state_s *state, node_s *exp);
static void walk_simplexp(state_s *state, node_s *exp);
static void walk_term(state_s *state, node_s *term);
static void walk_factor(state_s *state, node_s *factor);
static type_s *walk_factor_(state_s *state, node_s *fact_);

static void walk_while(state_s *state, node_s *w);
static void walk_do(state_s *state, node_s *d);
static void walk_for(state_s *state, node_s *f);
static void walk_varlet(state_s *state, node_s *vl);
static void walk_class(state_s *state, node_s *c);
static void walk_enum(state_s *state, node_s *c);
static void walk_struct(state_s *state, node_s *s);
static void walk_return(state_s *state, node_s *r);
static void walk_break(state_s *state, node_s *b);
static void walk_continue(state_s *state, node_s *c);

static void emit(buf_s *b, char *code, ...);
static void makelabel(unsigned *labelcount, char *buf);

void walk_tree(void *scoperoot, void *root)
{
    state_s state;
    
    state.scope = scoperoot;
    
    walk_statements(&state, root);
}

void walk_statements(state_s *state, node_s *stmt)
{
    int i;
    node_s *child;
    
    for(i = 0; i < stmt->nchildren; i++) {
        child = stmt->children[i];
        switch(child->type) {
            case TOKTYPE_EXPRESSION:
                walk_expression(state, child);
                break;
            case TOKTYPE_WHILE:
                walk_while(state, child);
                break;
            case TOKTYPE_DO:
                walk_do(state, child);
                break;
            case TOKTYPE_FOR:
                walk_for(state, child);
                break;
            case TOKTYPE_VAR:
            case TOKTYPE_LET:
                walk_varlet(state, child);
                break;
            case TOKTYPE_CLASS:
                walk_class(state, child);
                break;
            case TOKTYPE_ENUM:
                walk_enum(state, child);
                break;
            case TOKTYPE_STRUCTTYPE:
                walk_struct(state, child);
                break;
            case TOKTYPE_RETURN:
                walk_return(state, child);
                break;
            case TOKTYPE_SEMICOLON:
                break;
            case TOKTYPE_BREAK:
                walk_break(state, child);
                break;
            case TOKTYPE_CONTINUE:
                walk_continue(state, child);
                break;
            default:
                break;
        }
    }
}

void walk_expression(state_s *state, node_s *exp)
{
    node_s *first = FIRST(exp);
    
    if(first->type == TOKTYPE_RELOP) {
        walk_expression(state, LEFT(first));
        walk_expression(state, RIGHT(first));
    }
    else {
        walk_shiftop(state, first);
    }
}

void walk_shiftop(state_s *state, node_s *exp)
{
    if(exp->type == TOKTYPE_SHIFT) {
        walk_shiftop(state, LEFT(exp));
        walk_shiftop(state, RIGHT(exp));
    }
    else {
        walk_simplexp(state, exp);
    }
}

void walk_simplexp(state_s *state, node_s *exp)
{
    if(exp->type == TOKTYPE_ADDOP) {
        walk_simplexp(state, LEFT(exp));
        walk_simplexp(state, RIGHT(exp));
    }
    else {
        walk_term(state, exp);
    }
}

void walk_term(state_s *state, node_s *term)
{
    if(term->type == TOKTYPE_MULOP) {
        walk_term(state, LEFT(term));
        walk_term(state, RIGHT(term));
    }
    else {
        walk_factor(state, term);
    }
}

void walk_factor(state_s *state, node_s *factor)
{
    if(factor->type == TOKTYPE_EXPOP) {
        walk_factor_(state, LEFT(factor));
        walk_factor(state, RIGHT(factor));
    }
    else {
        walk_factor_(state, factor);
    }
}


type_s *walk_factor_(state_s *state, node_s *fact_)
{
    switch(fact_->type) {
        case TOKTYPE_NUM:
            printf("number: %s\n", fact_->tok->lex);
            break;
        default:
           // printf("other %s", fact_->tok->lex);
            break;
    }
    
    return NULL;
}


void walk_while(state_s *state, node_s *w)
{
    printf("in: %s\n", __func__);
}

void walk_do(state_s *state, node_s *d)
{
    printf("in: %s\n", __func__);
}

void walk_for(state_s *state, node_s *f)
{
    printf("in: %s\n", __func__);
}

void walk_varlet(state_s *state, node_s *vl)
{
    printf("in: %s\n", __func__);

}

void walk_class(state_s *state, node_s *c)
{
    printf("in: %s\n", __func__);

}

void walk_enum(state_s *state, node_s *c)
{
    printf("in: %s\n", __func__);
}

void walk_struct(state_s *state, node_s *s)
{
    printf("in: %s\n", __func__);

}

void walk_return(state_s *state, node_s *r)
{
    printf("in: %s\n", __func__);

}

void walk_break(state_s *state, node_s *b)
{
    printf("in: %s\n", __func__);
}

void walk_continue(state_s *state, node_s *c)
{
    printf("in: %s\n", __func__);
}

void emit(buf_s *b, char *str, ...)
{
    char *s;
    va_list argp;
    
    va_start(argp, str);
    
    bufaddstr(b, str, strlen(str));
    
    while((s = va_arg(argp, char *))) {
        bufaddstr(b, s, strlen(s));
    }
    va_end(argp);
}

void makelabel(unsigned *labelcount, char *buf)
{
    sprintf(buf, "_l%u:\n", *labelcount);
    ++*labelcount;
}
