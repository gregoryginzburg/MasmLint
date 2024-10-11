#include "error_reporter.h"
#include <iostream>

std::vector<ErrorReporter::Error> ErrorReporter::errors;

void ErrorReporter::init() {}

void ErrorReporter::reportError(const std::string &message, int line, int column, const std::string &lineContent,
                                const std::string &fileName)
{
    errors.push_back({Error::Level::Error, message, line, column, lineContent, fileName});
}

void ErrorReporter::reportWarning(const std::string &message, int line, int column, const std::string &lineContent,
                                  const std::string &fileName)
{
    errors.push_back({Error::Level::Warning, message, line, column, lineContent, fileName});
}

bool ErrorReporter::hasErrors()
{
    for (const auto &error : errors) {
        if (error.level == Error::Level::Error) {
            return true;
        }
    }
    return false;
}

void ErrorReporter::displayErrors()
{
    for (const auto &error : errors) {
        std::string levelStr = (error.level == Error::Level::Error) ? "Error" : "Warning";
        std::cout << levelStr << " [File: " << error.fileName << ", Line " << error.line << ", Column " << error.column
                  << "]: " << error.message << "\n";
        if (!error.lineContent.empty()) {
            std::cout << "    " << error.lineContent << "\n";
            std::cout << "    ";
            for (int i = 0; i < error.column; ++i) {
                std::cout << " ";
            }
            std::cout << "^\n";
        }
    }
}
