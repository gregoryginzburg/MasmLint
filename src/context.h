#pragma once

#include "input.h"
#include "error_reporter.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>

class Context {
public:
    static void init(const std::string &filename);
    static void pushInputStack(std::unique_ptr<InputSource> inputSrc);
    static std::unique_ptr<InputSource> &topInputStack();
    static bool emptyInputStack();
    static void popInputStack();
    static int getLineNumber();
    static std::string getFileName();

private:
    static std::string currentFileName;
    static int currentLineNumber;
    static std::stack<std::unique_ptr<InputSource>> inputStack;
};
