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

    string parseValue(char* param)
    {
        try {
            double val = MathParser::eval(param);
            return rtrim(std::to_string(val), "0.");
        } catch (const std::exception& e) {
            // do nothing, it's just not a math expression
        }
        return param;
    }

    // char* parseValue2(char* param)
    // {
    //     param = ltrim(param, ' ');
    //     try {
    //         double val = MathParser::eval(param);
    //         printf(">>>>>>>>>>>>>> %s val: %f\n", param, val);
    //         return (char *)rtrim(std::to_string(val), "0.").c_str();
    //     } catch (const std::exception& e) {
    //         // do nothing, it's just not a math expression
    //     }
    //     return param;
    // }

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
    ResultTypes parseScriptLine(char* line, const char* filename, uint8_t indentation, void (*callback)(char* command, char* params, const char* filename, uint8_t indentation))
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

        char* command = strtok(line, ":");
        if (command == NULL) {
            return ResultTypes::DEFAULT;
        }

        char* params = line + strlen(command) + 1;
        params = ltrim(params, ' ');
        applyVariable(params);
        // params = parseValue2(params);

        return defaultCallback(command, params, filename, indentation, callback);
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