#include "tree.h"
#include "parse.h"

#include <stdio.h>

static void walk_statements(node_s *stmt);

static void walk_expression(node_s *exp);
static void walk_while(node_s *w);
static void walk_do(node_s *d);
static void walk_for(node_s *f);
static void walk_varlet(node_s *vl);
static void walk_class(node_s *c);
static void walk_enum(node_s *c);
static void walk_struct(node_s *s);
static void walk_return(node_s *r);
static void walk_break(node_s *b);
static void walk_continue(node_s *c);


void walk_tree(node_s *root)
{
    walk_statements(root);
}

void walk_statements(node_s *stmt)
{
    int i;
    node_s *child;
    
    for(i = 0; i < stmt->nchildren; i++) {
        child = stmt->children[i];
        switch(child->type) {
            case TOKTYPE_EXPRESSION:
                walk_expression(child);
                break;
            case TOKTYPE_WHILE:
                walk_while(child);
                break;
            case TOKTYPE_DO:
                walk_do(child);
                break;
            case TOKTYPE_FOR:
                walk_for(child);
                break;
            case TOKTYPE_VAR:
            case TOKTYPE_LET:
                walk_varlet(child);
                break;
            case TOKTYPE_CLASS:
                walk_class(child);
                break;
            case TOKTYPE_ENUM:
                walk_enum(child);
                break;
            case TOKTYPE_STRUCTTYPE:
                walk_struct(child);
                break;
            case TOKTYPE_RETURN:
                walk_return(child);
                break;
            case TOKTYPE_SEMICOLON:
                break;
            case TOKTYPE_BREAK:
                walk_break(child);
                break;
            case TOKTYPE_CONTINUE:
                walk_continue(child);
                break;
            default:
                break;
        }
    }
}

void walk_expression(node_s *exp)
{
    printf("in: %s\n", __func__);
}

void walk_while(node_s *w)
{
    printf("in: %s\n", __func__);
}

void walk_do(node_s *d)
{
    printf("in: %s\n", __func__);
}

void walk_for(node_s *f)
{
    printf("in: %s\n", __func__);
}

void walk_varlet(node_s *vl)
{
    printf("in: %s\n", __func__);

}

void walk_class(node_s *c)
{
    printf("in: %s\n", __func__);

}

void walk_enum(node_s *c)
{
    printf("in: %s\n", __func__);
}

void walk_struct(node_s *s)
{
    printf("in: %s\n", __func__);

}

void walk_return(node_s *r)
{
    printf("in: %s\n", __func__);

}

void walk_break(node_s *b)
{
    printf("in: %s\n", __func__);
}

void walk_continue(node_s *c)
{
    printf("in: %s\n", __func__);
}

