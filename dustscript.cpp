#include <filesystem>
#include <stdio.h>

#include "dustscript.h"

void scriptCallback(char* command, char* params, const char* filename, uint8_t indentation, DustScript& instance)
{
    if (strcmp(command, "print") == 0) {
        printf(">> LOG: %s\n", params);
    } else if (strcmp(command, "myFunc") == 0) {
        // Demo a custom C++ function assigning variable
        char myVar[512];
        sprintf(myVar, "my custom variable from C++ with params [%s]", params);
        instance.setVariable((char *)"$myVar", myVar);
    } else {
        printf("(%s, %d) command: %s params: %s\n", filename, indentation, command, params);
    }
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s <script>\n", argv[0]);
        return 1;
    }
    DustScript::load(argv[1], scriptCallback, {
                                                  .variables = { { "$CWD", std::filesystem::current_path() } },
                                              });
    return 0;
}
