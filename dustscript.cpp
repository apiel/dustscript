#include <stdio.h>

#include "dustscript.h"

void scriptCallback(char *key, std::vector<string> params, const char *filename, uint16_t indentation)
{
    if (strcmp(key, "print") == 0)
    {
        printf(">> LOG: %s\n", params[0].c_str());
    }
    else
    {
        printf("(%s, %d) key: %s params:", filename, indentation, key);
        for (auto param : params)
        {
            printf("'%s' ", param.c_str());
        }
        printf("\n");
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
