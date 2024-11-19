#include "parser.h"
#include "symbol_table.h"
#include "diag_ctxt.h"

// #include <fmt/core.h>

Parser::Parser(std::shared_ptr<ParseSession> parseSession, const std::vector<Token> &tokens)
    : parseSess(parseSession), tokens(tokens)
{
}

ASTPtr Parser::parse()
{
    return parseExpression();
    // currentIndex = 0;
    // while (currentIndex < tokens.size()) {
    //     advance();
    //     if (currentToken.type == TokenType::EndOfFile) {
    //         return; // End parsing when EOF is reached
    //     }
    //     parseLine();
    // }
}

void Parser::advance()
{
    if (currentIndex < tokens.size()) {
        currentToken = tokens[currentIndex++];
    } else {
        currentToken = {TokenType::EndOfFile, "", Span(1, 2, nullptr)};
    }
}

ASTPtr Parser::parseLine() { return nullptr; }

bool Parser::match(TokenType type) { return currentToken.type == type; }

bool Parser::match(TokenType type, const std::string &value)
{
    return currentToken.type == type && currentToken.lexeme == value;
}

Token Parser::consume(TokenType type)
{
    if (currentToken.type == type) {
        Token token = currentToken;
        advance();
        return token;
    } else {
        // reportError(currentToken.span, ErrorCode::EXPECTED_TOKEN, "Expected token of type");
        Token token = currentToken;
        advance();
        return token;
    }
}

Token Parser::consume(TokenType type, const std::string &value)
{
    if (currentToken.type == type && currentToken.lexeme == value) {
        Token token = currentToken;
        advance();
        return token;
    } else {
        // reportError(currentToken.span, ErrorCode::EXPECTED_TOKEN, "Expected token '" + value + "'");
        Token token = currentToken;
        advance();
        return token;
    }
}

ASTPtr Parser::parseExpression()
{
    // ASTPtr multiplicativeExpression = parseMultiplicativeExpression();
    return nullptr;
}
