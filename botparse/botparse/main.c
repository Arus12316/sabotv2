#include "parse.h"
#include "calc.h"
#include <stdio.h>
#include "general.h"
#include <assert.h>

char testcase1f[] = "/Users/jhamm/sabotv2/botparse/botparse/testcase1";


char *readsrc(char *file);

int main(int argc, const char *argv[])
{
    char *src;
    errlist_s *err;
    calcres_s res;
    
    res = eval("3x^y^z^2^3-(x - 4 - 3 - y)+sqrt(4)^4 plus x");
    printf("result: %s\n", res.val);
    return 0;
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
