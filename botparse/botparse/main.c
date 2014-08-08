#include "parse.h"
#include <stdio.h>
#include "general.h";

char testcase1[] = "23.23 hello 23.1 _234_23 2332 -> => @ 2323 *= 234 ^= 23 for x <- {10 => 20} do ";

//char testcase2[] = "var bob : ()"

int main(int argc, const char *argv[])
{
    parse(testcase1);
    return 0;
}
