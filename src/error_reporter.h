#pragma once

#include <string>
#include <vector>
#include "context.h"

class ErrorReporter {
public:
    static void init(); 
    static void reportError(const std::string &message, int line, int column, const std::string &lineContent,
                     const std::string &fileName);
    static void reportWarning(const std::string &message, int line, int column, const std::string &lineContent,
                       const std::string &fileName);
    static bool hasErrors();
    static void displayErrors();

private:
    struct Error {
        enum class Level { Error, Warning };
        Level level;
        std::string message;
        int line;
        int column;
        std::string lineContent;
        std::string fileName;
    };

    static std::vector<Error> errors;
};
