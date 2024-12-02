#pragma once

#include <stack>
#include <unordered_set>

#include "symbol_table.h"
#include "diag_ctxt.h"
#include "preprocessor.h"
#include "session.h"
#include "ast.h"

enum class TokenType : uint8_t;

class Parser {
public:
    Parser(const std::shared_ptr<ParseSession> &parseSession, const std::vector<Token> &tokens);
    ASTPtr parse();

private:
    std::shared_ptr<ParseSession> parseSess;
    const std::vector<Token> &tokens;
    size_t currentIndex = 0;
    Token currentToken;

    std::optional<std::string> currentSegment;

    std::stack<Token> expressionDelimitersStack;
    std::stack<Token> dataInitializerDelimitersStack;

    void advance();
    void synchronize();
    bool match(TokenType type) const;
    bool match(const std::string &value) const;
    bool match(TokenType type, const std::string &value) const;
    bool match(const std::unordered_set<std::string> &values) const;
    std::optional<Token> consume(TokenType type);
    std::optional<Token> consume(const std::string &value);
    bool lookaheadMatch(size_t n, const std::string &value) const;
    bool lookaheadMatch(size_t n, const std::unordered_set<std::string> &values) const;
    bool lookaheadMatch(size_t n, TokenType type) const;
    bool lookaheadNextLineMatch(const std::string &value) const;

    std::shared_ptr<Statement> parseStatement();

    std::shared_ptr<SegDir> parseSegDir();
    std::shared_ptr<DataDir> parseDataDir();
    std::shared_ptr<StructDir> parseStructDir();
    std::shared_ptr<RecordDir> parseRecordDir();
    std::shared_ptr<EquDir> parseEquDir();
    std::shared_ptr<EqualDir> parseEqualDir();
    std::shared_ptr<ProcDir> parseProcDir();
    std::shared_ptr<EndDir> parseEndDir();

    std::shared_ptr<Instruction> parseInstruction();

    std::shared_ptr<DataItem> parseDataItem();
    std::shared_ptr<InitValue> parseInitValue();
    std::shared_ptr<InitValue> parseSingleInitValue();
    std::shared_ptr<InitializerList> parseInitializerList();

    ExpressionPtr parseExpression();
    ExpressionPtr parseExpressionHelper();
    ExpressionPtr parseMultiplicativeExpression();
    ExpressionPtr parseUnaryExpression();
    ExpressionPtr parsePtrExpression();
    ExpressionPtr parseMemberAccessAndIndexingExpression();
    ExpressionPtr parseHighPrecedenceUnaryExpression();
    ExpressionPtr parsePrimaryExpression();

    // Program
    std::shared_ptr<Diagnostic> reportExpectedEndOfLine(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedEndDir(const Token &token);

    // Statement
    std::shared_ptr<Diagnostic> reportMustBeInSegmentBlock(const Token &firstToken, const Token &lastToken);

    // SegDir

    // DataDir
    std::shared_ptr<Diagnostic> reportExpectedIdentifierInDataDir(const Token &token);

    // StructDir
    std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeStruc(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifierInStrucDir(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedDifferentIdentifierInStructDir(const Token &found, const Token &expected);
    std::shared_ptr<Diagnostic> reportExpectedEnds(const Token &token);
    std::shared_ptr<Diagnostic> reportMissingIdentifierBeforeEnds(const Token &token);

    // ProcDir
    std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeProc(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifierInProcDir(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedDifferentIdentifierInProcDir(const Token &found, const Token &expected);
    std::shared_ptr<Diagnostic> reportExpectedEndp(const Token &token);
    std::shared_ptr<Diagnostic> reportMissingIdentifierBeforeEndp(const Token &token);

    // EquDir
    std::shared_ptr<Diagnostic> reportExpectedIdentifierInEquDir(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeEqu(const Token &token);

    // EqualDir
    std::shared_ptr<Diagnostic> reportExpectedIdentifierInEqualDir(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeEqual(const Token &token);

    // Instruction
    std::shared_ptr<Diagnostic> reportExpectedInstruction(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedCommaOrEndOfLine(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifierInLabel(const Token &token);

    // DataItem
    std::shared_ptr<Diagnostic> reportExpectedVariableNameOrDataDirective(const Token &token);

    // InitValue
    std::shared_ptr<Diagnostic> reportUnclosedDelimiterInDataInitializer(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedCommaOrClosingDelimiter(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedOpenBracket(const Token &token);

    // Expression
    std::shared_ptr<Diagnostic> reportUnclosedDelimiterError(const Token &closingDelimiter);
    std::shared_ptr<Diagnostic> reportExpectedExpression(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedOperatorOrClosingDelimiter(const Token &token);
    std::shared_ptr<Diagnostic> reportExpectedIdentifierInExpression(const Token &token);
};