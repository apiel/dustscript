#include <stdio.h>

#include "dustscript.h"

void scriptCallback(char *key, std::vector<string> params, const char *filename)
{
    printf("(%s) fn: %s\n", filename, key);
    for (auto param : params)
    {
        printf("    - '%s'\n", param.c_str());
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <script>\n", argv[0]);
        return 1;
    }
    DustScript::load(argv[1], scriptCallback);
    return 0;
}
