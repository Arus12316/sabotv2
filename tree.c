#include "tree.h"
#include "parse.h"
#include "general.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LEFT(n) n->children[0]
#define RIGHT(n) n->children[1]
#define FIRST(n) LEFT(n)
#define SCOPEDOWN() state->scope = state->scope->children[state->scope->i]
#define SCOPEUP() state->scope = state->scope->parent; state->scope->i++

typedef struct state_s state_s;
typedef struct val_s val_s;

struct state_s
{
    int labelcount;
    int loccount;
    buf_s *code;
    scope_s *scope;
};

struct val_s
{
    bool isconst;
    tok_s *tok;
    node_s *val;
    type_s type;
};

static void walk_statements(state_s *state, node_s *stmt);

static void walk_expression(state_s *state, node_s *exp);
static void walk_shiftop(state_s *state, node_s *exp);
static void walk_simplexp(state_s *state, node_s *exp);
static void walk_term(state_s *state, node_s *term);
static void walk_factor(state_s *state, node_s *factor);
static val_s walk_factor_(state_s *state, node_s *fact_);

static void walk_while(state_s *state, node_s *w);
static void walk_do(state_s *state, node_s *d);
static void walk_for(state_s *state, node_s *f);
static val_s walk_varlet(state_s *state, node_s *vl);
static void walk_class(state_s *state, node_s *c);
static void walk_enum(state_s *state, node_s *c);
static void walk_struct(state_s *state, node_s *s);
static void walk_return(state_s *state, node_s *r);
static void walk_break(state_s *state, node_s *b);
static void walk_continue(state_s *state, node_s *c);

static type_s resolve_ident(state_s *state, node_s *ident);
static type_s *totype(node_s *texp);

static void emit(buf_s *b, char *code, ...);
static void makelabel(unsigned *labelcount, char *buf);
static void makeloc(unsigned *loccount, char *buf);

void walk_tree(void *scoperoot, void *root)
{
    state_s state;
    
    state.scope = scoperoot;
    state.labelcount = 0;
    state.loccount = 0;
    state.code = bufinit();
    
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

val_s walk_factor_(state_s *state, node_s *fact_)
{
    val_s val, v1, v2;

    switch(fact_->type) {
        case TOKTYPE_IDENT:
            val.type = resolve_ident(state, fact_);
            val.tok = fact_->tok;
            val.val = fact_;
            return val;
        case TOKTYPE_NUM:
            val.type.cat = CLASS_SCALAR;
            switch(fact_->att) {
                case TOKATT_NUMINT:
                    val.type.prim.val = &int_type;
                    break;
                case TOKATT_NUMREAL:
                    val.type.prim.val = &double_type;
                    break;
                default:
                    break;
            }
            val.tok = fact_->tok;
            val.val = fact_;
            return val;
        case TOKTYPE_STRING:
            val.type.cat = CLASS_SCALAR;
            val.type.prim.val = &string_type;
            val.tok = fact_->tok;
            val.val = fact_;
            return val;
        case TOKTYPE_REGEX:
            val.type.cat = CLASS_SCALAR;
            val.type.prim.val = &regex_type;
            val.tok = fact_->tok;
            val.val = fact_;
            return val;
        case TOKTYPE_ASSIGN:
            v1 = walk_factor_(state, LEFT(fact_));
            v2 = walk_factor_(state, RIGHT(fact_));
            
            return val;
            break;
        case TOKTYPE_VAR:
        case TOKTYPE_LET:
            walk_varlet(state, fact_);
            break;
        default:
           // printf("other %s", fact_->tok->lex);
            break;
    }
    
    return val;
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

val_s walk_varlet(state_s *state, node_s *vl)
{
    val_s v;
    node_s  *ident = vl->children[0],
            *dectype = (vl->nchildren > 1 ? vl->children[1] : NULL);
   
    addident(state->scope->table, ident->tok->lex, (vl->type == TOKTYPE_LET));
    
    if(dectype) {
        bindtype(state->scope, ident->tok->lex, dectype);
    }

    printf("in: %s\n", __func__);
    return v;
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

type_s resolve_ident(state_s *state, node_s *ident)
{
    type_s t;
    node_s *n;
    char *identval = ident->tok->lex;
    
    printf("Resolving Identifier: %s\n", identval);
    n = identlookup(state->scope, identval);
    if(!n) {
        printf("Use of undeclared identifier: %s\n", identval);
    }
    return t;
}

type_s *totype(node_s *texp)
{
    type_s *t = alloc(sizeof *t);
    

    switch(texp->type) {
        case TOKTYPE_IDENT:
            if(texp->nchildren) {
                t->cat = CLASS_ARRAY;
                t->array.indeces = texp->children[0];
                
            }
            break;
        
    }
    return t;
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

void makeloc(unsigned *loccount, char *buf)
{
    sprintf(buf, "_T%u:\n", *loccount);
    ++*loccount;
}
