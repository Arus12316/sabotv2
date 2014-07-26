/*
 Implementation of Regex parser via NFA using Thompson's Construction
 Possible Goal: Implement option to convert to DFA
 */

#include "regex.h"
#include "general.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INIT_BLOCKSIZE 4
#define MAX_REPITITION_BUF 15

#define rp_error(a) rp_error_(a,sizeof(a))

enum regx_e {
    REGX_STARTANCHOR  = 0x80,
    REGX_ENDANCHOR,
    REGX_WILDCARD,
    REGX_EPSILON,
    REGX_BEGINGROUP,
    REGX_ENDGROUP,
};

/*
 <start> -> <anchor_start> <expressions> <anchor_end>
 <anchor_start> -> ^ | E
 <anchor_end> -> $ | E
 
 <expressions> -> <expresion> <closure> <expressions'>
 <expressions'> -> <union> <exression> <closure> <expressions'> | E
 <expression> -> char | . | <class> | ( <expressions> )
 <class> -> [ <chars> ]
 <chars> -> char <chars'>
 <chars'> -> char <chars'> | E
 <closure> -> * | + | ? | { <digits> } | E
 <union> -> \| | E
 <digits> -> digit <digits'>
 <digits'> -> digit <digits'> | E
 
 first(<expressions>) = first(<expression>) = char,
 
 */

typedef struct ptrstk_s ptrstk_s;

struct ptrstk_s
{
    void *ptr;
    ptrstk_s *next;
} *stack;

static regex_s *mach;
static const char *start;
static const char *c;
static regerr_s *errptr;

static nfa_s *rp_start(void);
static nfa_s *rp_expressions();
static void rp_expressions_(nfa_s *nfa);
static nfa_s *rp_expression(void);
static nfa_s *rp_class(nfa_s *nfa);
static bool rp_closure(void);
static void rp_union(void);

static fsmnode_s *rp_makenode(fsmnode_s *parent, regx_val_s val);
static inline fsmnode_s *fsmnode_s_(void);
static void rp_bridge(fsmnode_s *parent, fsmnode_s *child, regx_val_s val);
static void rp_add_edge(fsmnode_s *parent, fsmedge_s *edge);

static void rp_error_(const char *e, size_t len);

static void print_node(fsmnode_s *node);
static void push(fsmnode_s *n);
static bool contains(fsmnode_s *n);

regex_s *compile_regex(const char *src)
{
    start = c = src;
    mach = allocz(sizeof(*mach));
    mach->nfa = rp_start();
    return mach;
}

nfa_s *rp_start(void)
{
    nfa_s *nfa;
    
    if(*c == '^') {
        c++;
    }
    nfa = rp_expressions();
    if(*c == '$') {
        c++;
        if(*c) {
            //error
            rp_error("Inappropriate placement of character end of line anchor");
        }
    }
    if(*c) {
        rp_error("Invalid placement of special character");
    }
    return nfa;
}

nfa_s *rp_expressions(void)
{
    nfa_s *nfa;
    
    nfa = rp_expression();
    rp_closure();
    rp_expressions_(nfa);
    
    return nfa;
}

void rp_expressions_(nfa_s *nfa)
{
    nfa_s *subexp;
    regx_val_s val;
    fsmnode_s *u1, *u2;
    const char *last;
    
    while(*c && *c != ')' && *c != '$') {
        if(*c == '|') {
            last = ++c;
        }
        else
            last = NULL;
        subexp = rp_expression();
        if(last) {
            if(c != last) {
                val.c = REGX_EPSILON;
                val.is_scalar = true;
                u1 = fsmnode_s_();
                u2 = fsmnode_s_();
                rp_bridge(u1, nfa->start, val);
                rp_bridge(u1, subexp->start, val);
                rp_bridge(nfa->final, u2, val);
                rp_bridge(subexp->final, u2, val);
                nfa->start = u1;
                nfa->final = u2;
                free(subexp);
            }
            else {
                rp_error("Inappropriate placement of special character following alternation");
            }
        }
    }
}

nfa_s *rp_expression(void)
{
    nfa_s vnfa;
    nfa_s *nfa, *subexp;
    regx_val_s val;
    repet_s *rep;
    char    r1[MAX_REPITITION_BUF + 1],
            r2[MAX_REPITITION_BUF + 1],
            *rptr;
    
    nfa = alloc(sizeof *nfa);
    vnfa.start = nfa->start = fsmnode_s_();
    vnfa.final = nfa->final = nfa->start;
    
    val.is_scalar = true;
    while(*c) {
        switch(*c) {
            case '.':
                val.c = REGX_WILDCARD;
                vnfa.start = nfa->final;
                nfa->final = rp_makenode(nfa->final, val);
                vnfa.final = nfa->final;
                break;
            case '[':
                rp_class(nfa);
                break;
            case '(':
                c++;
                subexp = rp_expressions();
                if(*c == ')') {
                    val.c = REGX_BEGINGROUP;
                    rp_bridge(nfa->final, subexp->start, val);
                    nfa->final = subexp->final;
                    val.c = REGX_ENDGROUP;
                    nfa->final = rp_makenode(nfa->final, val);
                    vnfa.final = nfa->final;
                }
                else {
                    rp_error("Unbalanced Parenthesis");
                }
                free(subexp);
                break;
            case '\\':
                c++;
                val.c = *c;
                nfa->final = rp_makenode(nfa->final, val);
                break;
            case '*':
                val.c = REGX_EPSILON;
                rp_bridge(vnfa.start, vnfa.final, val);
                rp_bridge(vnfa.final, vnfa.start, val);
                break;
            case '+':
                val.c = REGX_EPSILON;
                rp_bridge(vnfa.final, vnfa.start, val);
                break;
            case '?':
                val.c = REGX_EPSILON;
                rp_bridge(vnfa.start, vnfa.final, val);
                break;
            case '{':
                rptr = r1;
                val.is_scalar = false;
                
                rep = alloc(sizeof *rep);
                rep->count = 0;
                
                while(*++c != '}') {
                    
                    if(*c >= '0' && *c <= '9') {
                        *rptr++ = *c;
                    }
                    else if(*c == ',') {
                        if(rptr == r1) {
                        }
                        else {
                            if(rptr > r1 && rptr < r1 + MAX_REPITITION_BUF) {
                                *rptr = '\0';
                            }
                            else if(rptr == r2) {
                                
                            }
                            else if(rptr > r2 && rptr < r2 + MAX_REPITITION_BUF) {
                                rp_error("Only two values permitted in repetition clause.");
                            }
                        }
                        rptr = r2;
                    }
                    else if(
                            *c != ' ' &&
                            *c != '\t'&&
                            *c != '\v'&&
                            *c != '\n'&&
                            *c != '\r'
                            ) {
                        rp_error("Invalid character in repetition clause.");
                    }
                }
                *rptr = '\0';
                rep->low = atoi(r1);
                rep->high = atoi(r2);
                val.is_scalar = true;
                break;
            case ')':
            case '|':
            case '$':
                return nfa;
            default:
                val.c = *c;
                vnfa.start = vnfa.final;
                vnfa.final = rp_makenode(vnfa.final, val);
                break;
        }
        c++;
    }
    return nfa;
}

nfa_s *rp_class(nfa_s *nfa)
{
    nfa_s *cl;
    fsmedge_s *e;
    fsmnode_s *start, *final;
    regx_val_s val;

    cl = alloc(sizeof *cl);
    cl->start = start = fsmnode_s_();
    cl->final = final = fsmnode_s_();

    while(*++c != ']') {
        switch(*c) {
            case '[':
                rp_class(nfa);
                break;
            case '-':
                e = start->edges[start->nedges - 1];
                e->val.is_scalar = false;
                e->val.low = *(c - 1);
                e->val.high = *(c + 1);
                if(*(c - 1) >= *(c + 1)) {
                    rp_error("Invalid character class range");

                }
                break;
            case '\0':
                rp_error("Unbalanced ']'");
                break;
            case '\\':
            default:
                val.c = *c;
                printf("Adding: %c\n", val.c);
                rp_bridge(start, final, val);
                break;
        }
    }
    return cl;
}

bool rp_closure(void)
{
    switch(*c) {
        case '*':
        case '+':
        case '?':
        case '{':
            return true;
        default:
            return false;
    }
}

fsmnode_s *rp_makenode(fsmnode_s *parent, regx_val_s val)
{
    fsmnode_s *n;

    n = fsmnode_s_();
    rp_bridge(parent, n, val);
    return n;
}

fsmnode_s *fsmnode_s_(void)
{
    fsmnode_s *n;

    n = alloc(sizeof *n);
    n->blocksize = INIT_BLOCKSIZE;
    n->edges = alloc(INIT_BLOCKSIZE * sizeof(*n->edges));
    n->rep = NULL;
    n->nedges = 0;
    return n;
}


void rp_bridge(fsmnode_s *parent, fsmnode_s *child, regx_val_s val)
{
    fsmedge_s *e;
    
    e = alloc(sizeof *e);
    e->val = val;
    e->parent = parent;
    e->child = child;
    rp_add_edge(parent, e);
}

void rp_add_edge(fsmnode_s *parent, fsmedge_s *edge)
{
    parent->nedges++;
    if(parent->nedges == parent->blocksize) {
        parent->blocksize *= 2;
        parent->edges = ralloc(parent->edges, parent->blocksize * sizeof(*parent->edges));
    }
    parent->edges[parent->nedges-1] = edge;
}

void rp_error_(const char *e, size_t len)
{
#define RP_ERROR_PREFIX "Error: "
#define RP_ERROR_SUFFIX " At char %c."
    
    regerr_s *err;
    
    err = alloc(sizeof(*err) + sizeof(RP_ERROR_PREFIX) + sizeof(RP_ERROR_SUFFIX) + len - 2);
    err->pos = e - start;
    sprintf(err->msg, RP_ERROR_PREFIX "%s" RP_ERROR_SUFFIX, e, *c);
    
    fprintf(stderr, "%s\n", err->msg);
    
    if(errptr)
        errptr->next = err;
    else
        mach->err = err;
    errptr = err;
    err->next = NULL;
    
    
#undef RP_ERROR_PREFIX
#undef RP_ERROR_SUFFIX
}

void print_nfa(nfa_s *nfa)
{

    print_node(nfa->start);

}

void print_node(fsmnode_s *node)
{
    unsigned i;
    push(node);

    printf("At %p\n", node);
    for(i = 0; i < node->nedges; i++) {
        printf("\t%p -> %c -> %p\n", node, node->edges[i]->val.c, node->edges[i]->child);
    }

    putchar('\n');

    for(i = 0; i < node->nedges; i++) {
        if(!contains(node->edges[i]->child))
            print_node(node->edges[i]->child);
    }

}

void push(fsmnode_s *n)
{
    ptrstk_s *sn = alloc(sizeof *sn);

    sn->ptr = n;
    sn->next = stack;
    stack = sn;
}

bool contains(fsmnode_s *n)
{
    ptrstk_s *i;

    for(i = stack; i; i = i->next) {
        if(i->ptr == n)
            return true;
    }
    return false;
}
