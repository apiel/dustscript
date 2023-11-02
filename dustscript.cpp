#include <stdio.h>

#include "dustscript.h"

void scriptCallback(char *command, char*  params, const char *filename, uint8_t indentation)
{
    if (strcmp(command, "print") == 0)
    {
        printf(">> LOG: %s\n", params);
    }
    else
    {
        printf("(%s, %d) command: %s params: %s\n", filename, indentation, command, params);
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
