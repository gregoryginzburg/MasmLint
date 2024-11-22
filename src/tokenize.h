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
    Type,
    Register,
    Number,
    StringLiteral,
    Operator,
    OpenBracket,        // '('
    CloseBracket,       // ')'
    OpenSquareBracket,  // '['
    CloseSquareBracket, // ']'
    OpenAngleBracket,   // '<'
    CloseAngleBracket,  // '>'
    Comma,              // ','
    Colon,              // ':'
    Dollar,             // '$'
    QuestionMark,       // '?'
    EndOfFile,
    EndOfLine,
    Comment,
    Invalid
};

struct Token {
    enum TokenType type;
    std::string lexeme;
    Span span;
    bool operator<(const Token &other) const
    {
        if (span.lo != other.span.lo) {
            return span.lo < other.span.lo;
        }
        return span.hi < other.span.hi;
    }
};

class Tokenizer {
public:
    Tokenizer(std::shared_ptr<ParseSession> psess, const std::string &src) : psess(psess), src(src), pos(0) {}
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
    bool isValidIdentifier(const std::string &lexeme);
    bool isValidIdentifierStart(char c);
    bool isValidIdentifierChar(char c);
    bool isValidNumberStart(char c);

    template <typename... Args> void addDiagnostic(size_t start, size_t end, ErrorCode errorCode, Args &&...args)
    {
        Diagnostic diag(Diagnostic::Level::Error, errorCode, std::forward<Args>(args)...);
        diag.addPrimaryLabel(Span(start, end, nullptr), "");
        psess->dcx->addDiagnostic(diag);
    }

    std::vector<Token> tokens;
    std::size_t pos;
    const std::string &src;
    std::shared_ptr<ParseSession> psess;
};

inline std::string stringToUpper(const std::string &str)
{
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upperStr;
}