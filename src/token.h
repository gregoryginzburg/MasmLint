#pragma once

#include <string>
#include <algorithm>
#include "span.h"

enum class TokenType : uint8_t {
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

inline std::string stringToUpper(const std::string &str)
{
    std::string upperStr = str;
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upperStr;
}

inline bool isReservedWord(const Token &token)
{
    return token.lexeme.size() != 1 && token.type != TokenType::Number && token.type != TokenType::StringLiteral &&
           token.type != TokenType::Identifier;
}

