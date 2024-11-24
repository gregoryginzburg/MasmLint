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
    Parser(const std::shared_ptr<ParseSession> &parseSession, const std::vector<Token> &tokens);
    ASTPtr parse();

private:
    std::shared_ptr<ParseSession> parseSess;
    const std::vector<Token> &tokens;
    size_t currentIndex = 0;
    Token currentToken;

    bool panicLine = false;

    std::stack<Token> expressionDelimitersStack;

    void advance();
    bool match(TokenType type) const;
    bool match(const std::string &value) const;
    bool match(TokenType type, const std::string &value) const;
    std::optional<Token> consume(TokenType type);
    std::optional<Token> consume(TokenType type, const std::string &value);

    ASTExpressionPtr parseLine();
    ASTExpressionPtr parseExpression();
    ASTExpressionPtr parseExpressionHelper();
    ASTExpressionPtr parseMultiplicativeExpression();
    ASTExpressionPtr parseUnaryExpression();
    ASTExpressionPtr parsePtrExpression();
    ASTExpressionPtr parseMemberAccessAndIndexingExpression();
    ASTExpressionPtr parseHighPrecedenceUnaryExpression();
    ASTExpressionPtr parseIndexSequence();
    ASTExpressionPtr parsePrimaryExpression();

    std::shared_ptr<Diagnostic> reportUnclosedDelimiterError(const Token &closingDelimiter);
    std::shared_ptr<Diagnostic> reportExpectedExpression(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedOperatorOrClosingDelimiter(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifier(const Token &token);
};