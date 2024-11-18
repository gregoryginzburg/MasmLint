#include "parser.h"
#include "symbol_table.h"
#include "diag_ctxt.h"

// #include <fmt/core.h>

Parser::Parser(std::shared_ptr<ParseSession> parseSession, const std::vector<Token> &tokens)
    : parseSess(parseSession), tokens(tokens)
{
}

void Parser::parse()
{
    currentIndex = 0;
    while (currentIndex < tokens.size()) {
        advance();
        if (currentToken.type == TokenType::EndOfFile) {
            return; // End parsing when EOF is reached
        }
        parseLine();
    }
}

void Parser::advance()
{
    if (currentIndex < tokens.size()) {
        currentToken = tokens[currentIndex++];
    } else {
        currentToken = {TokenType::EndOfFile, "", Span(1, 2, nullptr)};
    }
}

void Parser::parseLine() 
{
    parseExpression();
}

void Parser::parseExpression()
{
    
}
