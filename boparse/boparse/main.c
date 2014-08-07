#include "parse.h"
#include <stdio.h>

char testcase1[] = "23.23 hello 23.1 _234_23 2332 -> => @ 2323 *= 234 ^= 23";

int main(int argc, const char *argv[])
{
    parse(testcase1);
    return 0;
}

