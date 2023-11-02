#ifndef _DUSTSCRIPT_H_
#define _DUSTSCRIPT_H_

#include "mathParser.h"
#include "trim.h"

#include <stdexcept>
#include <vector>

class DustScript {
protected:
    struct Variable {
        string key;
        string value;
    };
    std::vector<Variable> variables;

    enum ResultTypes {
        DEFAULT = 0,
        IF_FALSE,
        LOOP,
        LOOP_FALSE,
    };

    struct Operator {
        const char* sign;
        bool (*test)(string left, string right);
    };
    static const uint8_t operatorCount = 6;
    Operator operators[operatorCount] = {
        { "==", [](string left, string right) { return left == right; } },
        { "!=", [](string left, string right) { return left != right; } },
        { ">", [](string left, string right) { return stod(left) > stod(right); } },
        { "<", [](string left, string right) { return stod(left) < stod(right); } },
        { "<=", [](string left, string right) { return stod(left) <= stod(right); } },
        { ">=", [](string left, string right) { return stod(left) >= stod(right); } },
    };

    string applyVariable(char* str)
    {
        std::string target(str);
        for (auto variable : variables) {
            int pos;
            std::string k(variable.key);
            while ((pos = target.find(k)) != std::string::npos) {
                target.replace(pos, k.length(), variable.value);
            }
        }
        return target;
    }

    struct Line {
        char* key;
        char* value;
    };

    Line splitLine(char* line, const char* token)
    {
        char* key = strtok(line, token);
        char* value = line + strlen(key) + 1; // Do not use strtok to split only in 2 element, in case there is more token
        if (key == NULL || value == NULL) {
            throw std::runtime_error("Invalid line " + string(line));
        }

        value = ltrim(value, ' ');
        string val = applyMathInString(applyMath(applyVariable(value)));
        strcpy(value, val.c_str());

        return { key, value };
    }

    string applyMathInString(string value)
    {
        size_t start = value.find('(');
        string prefix = value.substr(0, start);
        string mathExpr = value.substr(start + 1);
        uint8_t open = 1;
        int end = 0;
        for (; end < mathExpr.length(); end++) {
            if (mathExpr[end] == '(') {
                open++;
            } else if (mathExpr[end] == ')') {
                open--;
                if (open == 0) {
                    string rest = mathExpr.substr(end + 1);
                    value = prefix + applyMath(mathExpr.substr(0, end)) + rest;
                    value = applyMathInString(value);
                    break;
                }
            }
        }
        return value;
    }

    string applyMath(string value)
    {
        try {
            double val = MathParser::eval((char*)value.c_str());
            return rtrim(std::to_string(val), "0.");
        } catch (const std::exception& e) {
            // do nothing, it's just not a math expression
            // printf("!!! error: %s\n", e.what());
        }
        return value;
    }

    void setVariable(char* lineStr)
    {
        Line line = splitLine(lineStr, "=");
        // search first if variable already exists
        for (int i = 0; i < variables.size(); i++) {
            if (variables[i].key == line.key) {
                variables[i].value = line.value;
                return;
            }
        }
        Variable variable;
        variable.key = line.key;
        variable.value = line.value;
        variables.push_back(variable);
    }

    bool evalIf(string params)
    {
        for (auto op : operators) {
            size_t pos = params.find(op.sign);
            if (pos != std::string::npos) {
                string left = params.substr(0, pos);
                string right = params.substr(pos + strlen(op.sign));
                return op.test(left, right);
            }
        }

        return false;
    }

    ResultTypes defaultCallback(char* command, char* params, const char* filename, uint8_t indentation, void (*callback)(char* command, char* params, const char* filename, uint8_t indentation))
    {
        if (strcmp(command, "if") == 0) {
            return evalIf(params) ? ResultTypes::DEFAULT : ResultTypes::IF_FALSE;
        }
        if (strcmp(command, "while") == 0) {
            return evalIf(params) ? ResultTypes::LOOP : ResultTypes::LOOP_FALSE;
        }
        callback(command, params, filename, indentation);

        return ResultTypes::DEFAULT;
    }

    static DustScript* instance;

public:
    ResultTypes parseScriptLine(char* lineStr, const char* filename, uint8_t indentation, void (*callback)(char* command, char* params, const char* filename, uint8_t indentation))
    {
        lineStr = ltrim(lineStr, ' ');

        // ignore comments and empty lines
        if (lineStr[0] == '#' || lineStr[0] == '\n') {
            return ResultTypes::DEFAULT;
        }

        lineStr = rtrim(lineStr, '\n');

        if (lineStr[0] == '$') {
            setVariable(lineStr);
            return ResultTypes::DEFAULT;
        }

        Line line = splitLine(lineStr, ":");
        return defaultCallback(line.key, line.value, filename, indentation, callback);
    }

    void run(const char* filename, void (*callback)(char* command, char* params, const char* filename, uint8_t indentation))
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
            uint8_t indentation = countLeadingChar(line, ' ');
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

    static DustScript& load(const char* filename, void (*callback)(char* command, char* params, const char* filename, uint8_t indentation))
    {
        if (!instance) {
            instance = new DustScript();
        }
        instance->run(filename, callback);
        return *instance;
    }
};

DustScript* DustScript::instance = NULL;

#endif