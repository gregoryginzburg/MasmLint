#pragma once

#include <stack>
#include <unordered_set>

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

    std::optional<std::string> currentSegment;

    std::stack<Token> expressionDelimitersStack;
    std::stack<Token> dataInitializerDelimitersStack;

    void advance();
    void synchronizeLine();
    void synchronizeProcDir();
    void synchronizeStrucDir();
    bool match(Token::Type type) const;
    bool match(const std::string &value) const;
    bool match(Token::Type type, const std::string &value) const;
    bool match(const std::unordered_set<std::string> &values) const;
    std::optional<Token> consume(Token::Type type);
    std::optional<Token> consume(const std::string &value);
    bool lookaheadMatch(size_t n, const std::string &value) const;
    bool lookaheadMatch(size_t n, const std::unordered_set<std::string> &values) const;
    bool lookaheadMatch(size_t n, Token::Type type) const;

    [[nodiscard]] std::shared_ptr<Statement> parseStatement();

    [[nodiscard]] std::shared_ptr<SegDir> parseSegDir();
    [[nodiscard]] std::shared_ptr<DataDir>
    parseDataDir(const std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<DataVariableSymbol>>> &namedFields = nullptr);
    [[nodiscard]] std::shared_ptr<StructDir> parseStructDir();
    [[nodiscard]] std::shared_ptr<ProcDir> parseProcDir();
    [[nodiscard]] std::shared_ptr<RecordDir> parseRecordDir();
    [[nodiscard]] std::shared_ptr<RecordField> parseRecordField();
    [[nodiscard]] std::shared_ptr<EquDir> parseEquDir();
    [[nodiscard]] std::shared_ptr<EqualDir> parseEqualDir();
    [[nodiscard]] std::shared_ptr<EndDir> parseEndDir();

    [[nodiscard]] std::shared_ptr<Instruction> parseInstruction();

    [[nodiscard]] std::shared_ptr<DataItem>
    parseDataItem(const std::optional<Token> &idToken,
                  const std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<DataVariableSymbol>>> &namedFields);
    [[nodiscard]] std::shared_ptr<InitializerList> parseInitValues();
    [[nodiscard]] std::shared_ptr<InitValue> parseSingleInitValue();
    [[nodiscard]] std::shared_ptr<InitializerList> parseInitializerList();

    [[nodiscard]] ExpressionPtr parseExpression();
    [[nodiscard]] ExpressionPtr parseExpressionHelper();
    [[nodiscard]] ExpressionPtr parseMultiplicativeExpression();
    [[nodiscard]] ExpressionPtr parseUnaryExpression();
    [[nodiscard]] ExpressionPtr parsePtrExpression();
    [[nodiscard]] ExpressionPtr parseMemberAccessAndIndexingExpression();
    [[nodiscard]] ExpressionPtr parseHighPrecedenceUnaryExpression();
    [[nodiscard]] ExpressionPtr parsePrimaryExpression();

    // General
    [[nodiscard]] std::shared_ptr<Diagnostic> reportSymbolRedefinition(const Token &token, const Token &firstDefinedToken);

    // Program
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedEndOfLine(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedEndDir(const Token &token);

    // Statement
    [[nodiscard]] std::shared_ptr<Diagnostic> reportMustBeInSegmentBlock(const Token &firstToken, const Token &lastToken);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportProcMustBeInSegmentBlock(const Token &firstToken, const Token &lastToken);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportMustBeInCodeSegment(const Token &firstToken, const Token &lastToken);

    // SegDir

    // DataDir
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInDataDir(const Token &token);

    // StructDir
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeStruc(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInStrucDir(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedDifferentIdentifierInStructDir(const Token &found, const Token &expected);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedEnds(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportMissingIdentifierBeforeEnds(const Token &token);

    // ProcDir
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeProc(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInProcDir(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedDifferentIdentifierInProcDir(const Token &found, const Token &expected);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedEndp(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportMissingIdentifierBeforeEndp(const Token &token);

    // RecordDir
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInRecordDir(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedColonInRecordField(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeRecord(const Token &token);

    // EquDir
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInEquDir(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeEqu(const Token &token);

    // EqualDir
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInEqualDir(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierBeforeEqual(const Token &token);

    // Instruction
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedInstruction(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedCommaOrEndOfLine(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInLabel(const Token &token);

    // DataItem
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedVariableNameOrDataDirective(const Token &token);

    // InitValue
    [[nodiscard]] std::shared_ptr<Diagnostic> reportUnclosedDelimiterInDataInitializer(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedCommaOrClosingDelimiter(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedOpenBracket(const Token &token);

    // Expression
    [[nodiscard]] std::shared_ptr<Diagnostic> reportUnclosedDelimiterError(const Token &closingDelimiter);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedExpression(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedOperatorOrClosingDelimiter(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpectedIdentifierInExpression(const Token &token);
};