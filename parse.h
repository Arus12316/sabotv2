#ifndef PARSE_H_
#define PARSE_H_

typedef struct errlist_s errlist_s;

struct errlist_s
{
    errlist_s *next;
    char msg[];
};

extern void parse(char *src);

#endif
