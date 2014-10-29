#include "parse.h"
#include "calc.h"
#include <stdio.h>
#include "general.h"
#include <assert.h>

char testcase1f[] = "/Users/jhamm/sabotv2/botparse/botparse/testcase1.bot";


char *calctestcase[] = {
    "1 plus 1 plus 32",
    "33^33",
    ",..,.,..,.,.,.,.,..,,..,,..,,.()))())))()",
    "(((((((((((33^(32)))"
    ")33+32",
    "!!!,,,,..230987409237480",
    "..././/..//***",
    "(1 + 1)^(1+1)*(33^33^3^44^(1^4 plus 5^3^2^(8+1)))*32*(1-cos(cos(cos(tanh(32*e*pi))*pi)pi))e",
    "cos(pi)e*cos(pi)*cos(23*32|3"
};

char *readsrc(char *file);

int main(int argc, const char *argv[])
{
    int i;
    char *src;
    errlist_s *err;
    calcres_s res;
    
    
   
   /* for(i = 0; i < sizeof(calctestcase)/sizeof(*calctestcase); i++) {
        res = eval(calctestcase[i]);
        printf("result: %s\n\n", res.val);
    }
    
    return 0;*/
    src = readsrc(testcase1f);
    
    err = parse(src);
    
    printerrs(err);
    
    free(src);
    
    return 0;
}


char *readsrc(char *file) 
{
    int c;
    int size = 0;
    char *src;
    FILE *f;
    
    f = fopen(file, "r");
    if(!f) {        
        perror("file io");
        exit(EXIT_FAILURE);
    }
    
    src = alloc(1);
    
    while((c = fgetc(f)) != EOF) {
        src[size++] = c;
        src = realloc(src, size + 1);
        if(!src) {
            perror("realloc failure");
            exit(EXIT_FAILURE);
        }
    }
    src[size] = '\0';
    return src;
}
