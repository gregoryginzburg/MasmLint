#pragma once

#include "span.h"
#include "session.h"
#include "diagnostic.h"
#include <string>
#include <deque>
#include <unordered_set>

enum class TokenType {
    Identifier,
    Directive,
    Instruction,
    Register,
    Number,
    StringLiteral,
    Operator,
    OpenBracket,         // '('
    CloseBracket,        // ')'
    OpenSquareBracket,   // '['
    CloseSquareBracket,  // ']'
    Comma,               // ','
    Colon,               // ':'
    Dot,                 // '.'
    Percent,             // '%'
    EndOfFile,
    EndOfLine,
    Comment,
    Invalid
};

struct Token {
    enum TokenType type;
    std::string lexeme;
    Span span;

    // TODO: data about macro expansion
};

class Tokenizer {
public:
    Tokenizer(std::shared_ptr<ParseSession> psess, const std::string &src, std::size_t startPos)
        : psess(psess), startPos(startPos), src(src), pos(startPos)
    {
    }
    std::vector<Token> tokenize();

private:
    void skipWhitespace();
    Token getNextToken();
    Token getNumberToken();
    Token getIdentifierOrKeywordToken();
    Token getStringLiteralToken();
    Token getSpecialSymbolToken();
    bool isDotName();
    bool isValidNumber(const std::string &lexeme);
    bool isValidIdentifier(const std::string& lexeme);
    bool isValidIdentifierStart(char c);
    bool isValidIdentifierChar(char c);
    bool isValidNumberStart(char c);

    template <typename... Args> void addDiagnostic(size_t start, size_t end, ErrorCode errorCode, Args &&...args)
    {
        Diagnostic diag(Diagnostic::Level::Error, errorCode, std::forward<Args>(args)...);
        diag.addLabel(Span(start, end, nullptr), "");
        psess->dcx->addDiagnostic(diag);
    }

    std::vector<Token> tokens;
    std::size_t startPos;
    std::size_t pos;
    const std::string &src;
    std::shared_ptr<ParseSession> psess;
};