#ifndef _DUSTSCRIPT_H_
#define _DUSTSCRIPT_H_

#include "mathParser.h"
#include "trim.h"

#include <stdexcept>
#include <vector>

namespace DustScript {
struct Variable {
    string key;
    string value;
};
std::vector<Variable> variables;

string parseValue(char* param)
{
    param = ltrim(param, ' ');
    try {
        double val = MathParser::eval(param);
        return rtrim(std::to_string(val), "0.");
    } catch (const std::exception& e) {
        // do nothing, it's just not a math expression
    }
    return param;
}

char* applyVariable(char* str)
{
    std::string target(str);

    for (auto variable : variables) {
        int pos;
        std::string k(variable.key);
        while ((pos = target.find(k)) != std::string::npos) {
            target.replace(pos, k.length(), variable.value);
        }
    }
    strcpy(str, target.c_str());
    return str;
}

void setVariable(char* line)
{
    char* key = strtok(line, "=");
    char* value = strtok(NULL, "=");
    if (key == NULL || value == NULL) {
        throw std::runtime_error("Invalid variable line " + string(line));
    }
    value = applyVariable(value);

    // search first if variable already exists
    for (int i = 0; i < variables.size(); i++) {
        if (variables[i].key == key) {
            variables[i].value = parseValue(value);
            return;
        }
    }
    Variable variable;
    variable.key = key;
    variable.value = parseValue(value);
    variables.push_back(variable);
}

std::vector<string> getParams(char* paramsStr)
{
    std::vector<string> params;
    if (paramsStr != NULL) {
        applyVariable(paramsStr);
        char* param = strtok(paramsStr, ",");
        while (param != NULL) {
            params.push_back(parseValue(param));
            param = strtok(NULL, ",");
        }
    }
    return params;
}

enum ResultTypes {
    DEFAULT = 0,
    IF_FALSE,
    LOOP,
    LOOP_FALSE,
};

bool evalIf(std::vector<string> params)
{
    if (params.size() != 3) {
        throw std::runtime_error("Invalid if statement, required 3 paremeters: if: $var1, =, $var3");
    }

    bool result = false;
    if (params[1] == "==") {
        result = params[0] == params[2];
    } else if (params[1] == "!=") {
        result = params[0] != params[2];
    } else if (params[1] == ">") {
        result = stod(params[0]) > stod(params[2]);
    } else if (params[1] == "<") {
        result = stod(params[0]) < stod(params[2]);
    } else if (params[1] == ">=") {
        result = stod(params[0]) >= stod(params[2]);
    } else if (params[1] == "<=") {
        result = stod(params[0]) <= stod(params[2]);
    } else {
        throw std::runtime_error("Invalid if statement operator: " + params[1]);
    }

    return result;
}

ResultTypes defaultCallback(char* key, std::vector<string> params, const char* filename, uint16_t indentation, void (*callback)(char* key, std::vector<string> params, const char* filename, uint16_t indentation))
{
    if (strcmp(key, "if") == 0) {
        return evalIf(params) ? ResultTypes::DEFAULT : ResultTypes::IF_FALSE;
    }
    if (strcmp(key, "while") == 0) {
        return evalIf(params) ? ResultTypes::LOOP : ResultTypes::LOOP_FALSE;
    }
    callback(key, params, filename, indentation);

    return ResultTypes::DEFAULT;
}

ResultTypes parseScriptLine(char* line, const char* filename, uint16_t indentation, void (*callback)(char* key, std::vector<string> params, const char* filename, uint16_t indentation))
{
    line = ltrim(line, ' ');

    // ignore comments and empty lines
    if (line[0] == '#' || line[0] == '\n') {
        return ResultTypes::DEFAULT;
    }

    line = rtrim(line, '\n');

    if (line[0] == '$') {
        setVariable(line);
        return ResultTypes::DEFAULT;
    }

    char* key = strtok(line, ":");
    if (key == NULL) {
        return ResultTypes::DEFAULT;
    }

    char* paramsStr = strtok(NULL, ":");
    std::vector<string> params = getParams(paramsStr);
    return defaultCallback(key, params, filename, indentation, callback);
}


void load(const char* filename, void (*callback)(char* key, std::vector<string> params, const char* filename, uint16_t indentation))
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        throw std::runtime_error("Failed to load script: " + string(filename));
    }

    char line[512];

    uint8_t skipTo = -1;
    long loopStartPos;
    uint8_t loopIndent = -1;
    while (fgets(line, sizeof(line), file)) {
        long pos = ftell(file) - strlen(line);
        uint16_t indentation = countLeadingChar(line, ' ');
        if (indentation > skipTo) {
            continue;
        }
        skipTo = -1; // will set to max value
        if (loopIndent != (uint8_t)-1 && indentation <= loopIndent && loopStartPos != pos) {
            fseek(file, loopStartPos, SEEK_SET);
            continue;
        }
        ResultTypes result = parseScriptLine(line, filename, indentation, callback);
        if (result == ResultTypes::IF_FALSE) {
            skipTo = indentation;
        } else if (result == ResultTypes::LOOP_FALSE) {
            skipTo = indentation;
            loopIndent = -1;
        } else if (result == ResultTypes::LOOP) {
            loopStartPos = pos;
            loopIndent = indentation;
        }
    }
    fclose(file);
}
}

#endif