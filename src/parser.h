#pragma once

#include "tokenize.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "preprocessor.h"
#include "session.h"
#include "ast.h"

class Parser {
public:
    Parser(std::shared_ptr<ParseSession> parseSession, const std::vector<Token> &tokens);
    ASTPtr parse();

private:
    std::shared_ptr<ParseSession> parseSess;

    int currentIndex;
    Token currentToken;
    const std::vector<Token> &tokens;

    void advance();
    bool match(TokenType type);
    bool match(TokenType type, const std::string &value);
    Token consume(TokenType type);
    Token consume(TokenType type, const std::string &value);

    ASTPtr parseLine();
    ASTPtr parseExpression();
    ASTPtr parseMultiplicativeExpression();
    ASTPtr parseUnaryExpression();
    ASTPtr parsePostfixExpression();
    ASTPtr parseMemberAccessExpression();
    ASTPtr parseHighPrecedenceUnaryExpression();
    ASTPtr parseIndexSequence();
    ASTPtr parsePrimaryExpression();
};