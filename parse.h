#ifndef PARSE_H_
#define PARSE_H_

typedef struct errlist_s errlist_s;

struct errlist_s
{
    errlist_s *next;
    char *msg;
};

extern errlist_s *parse(char *src);

extern void printerrs(errlist_s *err);

#endif
