#pragma once

#include <stack>

#include "tokenize.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "preprocessor.h"
#include "session.h"
#include "ast.h"

class Parser {
public:
    Parser(std::shared_ptr<ParseSession> parseSession, const std::vector<Token> &tokens);
    ASTExpressionPtr parse();

private:
    std::shared_ptr<ParseSession> parseSess;

    int currentIndex;
    Token currentToken;
    const std::vector<Token> &tokens;
    bool panicLine = false;

    std::stack<Token> expressionDelimitersStack;

    void advance();
    bool match(TokenType type);
    bool match(TokenType type, const std::string &value);
    std::optional<Token> consume(TokenType type);
    std::optional<Token> consume(TokenType type, const std::string &value);

    ASTExpressionPtr parseLine();
    ASTExpressionPtr parseExpression();
    ASTExpressionPtr parseExpressionHelper();
    ASTExpressionPtr parseMultiplicativeExpression();
    ASTExpressionPtr parseUnaryExpression();
    ASTExpressionPtr parsePostfixExpression();
    ASTExpressionPtr parseMemberAccessExpression();
    ASTExpressionPtr parseHighPrecedenceUnaryExpression();
    ASTExpressionPtr parseIndexSequence();
    ASTExpressionPtr parsePrimaryExpression();

    std::shared_ptr<Diagnostic> reportUnclosedDelimiterError(const Token &closingDelimiter);
    std::shared_ptr<Diagnostic> reportExpectedExpression(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedOperatorOrClosingDelimiter(const Token &token);
};