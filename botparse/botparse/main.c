#include "parse.h"
#include <stdio.h>
#include "general.h";

char testcase1[] = "23.23 hello 23.1 _234_23 2332 -> => @ 2323 *= 234 ^= 23 for x <- {10 => 20} do ";

char testcase2[] = "var bob := @(){1 + 1;}; bob(1, 2, 3, 4);";

int main(int argc, const char *argv[])
{
    parse(testcase2);
    return 0;
}
