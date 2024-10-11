#pragma once

#include "input.h"
#include "error_reporter.h"
#include "tokenize.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>

class Preprocessor {
public:
    static void init();
    

private:
    static std::string processLine(std::string& line);
};

