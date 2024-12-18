#ifndef _DUSTSCRIPT_H_
#define _DUSTSCRIPT_H_

#include "mathParser.h"
#include "trim.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

class DustScript {
public:
    struct Variable {
        string key;
        string value;
    };

protected:
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
        for (auto variable : props.variables) {
            int pos;
            std::string k(variable.key);
            while ((pos = target.find(k)) != std::string::npos) {
                // printf("Replacing (pos %d, len %ld) %s with %s\n", pos, k.length(), k.c_str(), variable.value.c_str());
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
        if (key == NULL || value[0] == '\0') {
            throw std::runtime_error("Invalid line: " + string(line));
        }

        value = ltrim(value, ' ');
        string val = applyMathInString(applyMath(applyVariable(value)));
        strcpy(value, val.c_str());

        return { key, value };
    }

    string applyMathInString(string value)
    {
        size_t start = value.find('(');
        if (start != string::npos && value[start - 1] == '\\') {
            // remove the escape character
            return value.substr(0, start - 1) + value.substr(start);
        }
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

    void sortVariable()
    {
        // sort variable from longer to shorter to avoid that a variable is overwritten by another
        std::sort(props.variables.begin(), props.variables.end(), [](const Variable& a, const Variable& b) {
            return a.key.length() > b.key.length();
        });
    }

    bool evalIf(string params)
    {
        for (auto op : operators) {
            size_t pos = params.find(op.sign);
            if (pos != std::string::npos) {
                string left = rtrim(params.substr(0, pos), ' ');
                string right = ltrim(params.substr(pos + strlen(op.sign)), ' ');
                return op.test(left, right);
            }
        }

        return false;
    }

    void setVariable(char* lineStr)
    {
        Line line = splitLine(lineStr, "=");
        setVariable(line.key, line.value);
    }

    void getFullpath(char* newPath, const char* parentFilename, char* fullpathBuffer)
    {
        char* lastSlash;
        strcpy(fullpathBuffer, parentFilename);
        if (newPath[0] == '/' || (lastSlash = strrchr(fullpathBuffer, '/')) == NULL) {
            fullpathBuffer[0] = '\0';
        } else {
            *lastSlash = '\0';
            strcat(fullpathBuffer, "/");
        }
        strcat(fullpathBuffer, newPath);
    }

    ResultTypes defaultCallback(char* command, char* params, const char* filename, uint8_t indentation, std::function<void(char* key, char* value, const char* filename, uint8_t indentation, DustScript& instance)> callback)
    {
        if (strcmp(command, "if") == 0) {
            return evalIf(params) ? ResultTypes::DEFAULT : ResultTypes::IF_FALSE;
        }
        if (strcmp(command, "while") == 0) {
            return evalIf(params) ? ResultTypes::LOOP : ResultTypes::LOOP_FALSE;
        }
        if (strcmp(command, "include") == 0) {
            char fullpath[512];
            getFullpath(params, filename, fullpath);
            DustScript::load(fullpath, callback);
            return ResultTypes::DEFAULT;
        }
        callback(command, params, filename, indentation, *this);

        return ResultTypes::DEFAULT;
    }

    static DustScript* instance;

public:
    ResultTypes parseScriptLine(char* lineStr, const char* filename, uint8_t indentation, std::function<void(char* key, char* value, const char* filename, uint8_t indentation, DustScript& instance)> callback)
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

    void run(const char* filename, std::function<void(char* key, char* value, const char* filename, uint8_t indentation, DustScript& instance)> callback)
    {
        uint lineCount = 0;
        string lineStack = "";
        try {
            FILE* file = fopen(filename, "r");
            if (file == NULL) {
                throw std::runtime_error("Failed to load script: " + string(filename));
            }

            uint8_t skipTo = -1;
            long loopStartPos;
            uint8_t loopIndent = -1;
            char line[512];
            while (fgets(line, sizeof(line), file)) {
                lineCount++;
                lineStack = line;
                long pos = ftell(file) - strlen(line);
                uint8_t indentation = countLeadingChar(line, ' ');
                if (indentation > skipTo) {
                    continue;
                }
                if (line[indentation] == '\n' || line[indentation] == '\0') {
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
        } catch (const std::exception& e) {
            throw std::runtime_error("DustScript Error (" + string(filename) + ": "
                + std::to_string(lineCount) + "): " + lineStack + "\t" + std::string(e.what()));
        }
    }

    void setVariable(char* key, char* value)
    {
        // search first if variable already exists
        for (int i = 0; i < props.variables.size(); i++) {
            if (props.variables[i].key == key) {
                props.variables[i].value = value;
                return;
            }
        }
        Variable variable;
        variable.key = key;
        variable.value = value;
        props.variables.push_back(variable);

        sortVariable();
    }

    struct Props {
        std::vector<Variable> variables;
    } props;

    DustScript(Props& props)
        : props(props)
    {
        sortVariable();
    }

    static DustScript& load(
        const char* filename,
        std::function<void(char* key, char* value, const char* filename, uint8_t indentation, DustScript& instance)> callback,
        Props props = {})
    {
        if (!instance) {
            instance = new DustScript(props);
        }
        instance->run(filename, callback);
        return *instance;
    }
};

DustScript* DustScript::instance = NULL;

#endif