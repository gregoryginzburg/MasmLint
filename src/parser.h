#pragma once

#include "tokenize.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "preprocessor.h"
#include "session.h"

class Parser {
public:
    Parser(std::shared_ptr<ParseSession> parseSession, const std::vector<Token> &tokens);
    void parse();

private:
    std::shared_ptr<ParseSession> parseSess;

    Token currentToken;
    const std::vector<Token> &tokens;
    int currentIndex;

    void advance();
    void parseLine();
    void parseExpression();
};