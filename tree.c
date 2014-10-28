#include "tree.h"
#include "parse.h"

#include <stdio.h>

static void walk_statements(node_s *stmt);
static void walk_expression(node_s *exp);

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
        if(child->type == TOKTYPE_EXPRESSION) {
            walk_expression(child);
        }
    }
}

void walk_expression(node_s *exp)
{
    printf("walk expression called\n");
}
