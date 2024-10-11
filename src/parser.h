#pragma once

#include "tokenize.h"
#include "symbol_table.h"
#include "error_reporter.h"
#include "context.h"
#include "preprocessor.h"

class Parser {
public:
    static void init();
    static void parse();

private:
    static std::vector<Token> tokens;
    static size_t currentIndex;
    static Token currentToken;

    static bool getNextLine(std::string& line);

    static void advance();
    static void parseLine();
    static void parseDirective();
    static void parseInstruction();
    static void parseOperandList();
    static void reportError(const std::string &message);
};