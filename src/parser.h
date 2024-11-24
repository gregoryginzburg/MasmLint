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

    std::shared_ptr<Directive> parseSegDir();
    std::shared_ptr<Directive> parseDataDir();
    std::shared_ptr<Directive> parseStructDir();
    std::shared_ptr<Directive> parseRecordDir();
    std::shared_ptr<Directive> parseEquDir();
    std::shared_ptr<Directive> parseEqualDir();
    std::shared_ptr<Directive> parseProcDir();

    std::shared_ptr<Instruction> parseInstruction();
    std::shared_ptr<LabelDef> parseLabelDef();

    ExpressionPtr parseExpression();
    ExpressionPtr parseExpressionHelper();
    ExpressionPtr parseMultiplicativeExpression();
    ExpressionPtr parseUnaryExpression();
    ExpressionPtr parsePtrExpression();
    ExpressionPtr parseMemberAccessAndIndexingExpression();
    ExpressionPtr parseHighPrecedenceUnaryExpression();
    ExpressionPtr parseIndexSequence();
    ExpressionPtr parsePrimaryExpression();

    std::shared_ptr<Diagnostic> reportUnclosedDelimiterError(const Token &closingDelimiter);
    std::shared_ptr<Diagnostic> reportExpectedExpression(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedOperatorOrClosingDelimiter(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifier(const Token &token);
};