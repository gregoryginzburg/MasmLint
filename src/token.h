#pragma once

#include <string>
#include <algorithm>
#include "span.h"

// TODO: why is this warning happening?
// NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
struct Token {
    // Define like this, because Token::Type name conflicts with windows headers
    // enum TOKEN_INFORMATION_CLASS::Token::Type = 8 from winnt.h conflicts with enum class Token::Type
    enum class Type : uint8_t {
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

    Type type{Token::Type::Invalid};
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
    std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upperStr;
}

inline bool isReservedWord(const Token &token)
{
    return token.lexeme.size() != 1 && token.type != Token::Type::Number && token.type != Token::Type::StringLiteral &&
           token.type != Token::Type::Identifier && token.type != Token::Type::EndOfLine && token.type != Token::Type::EndOfFile;
}

inline std::string tokenTypeToStr(enum Token::Type type)
{
    switch (type) {
    case Token::Type::Identifier:
        return "Identifier";
    case Token::Type::Directive:
        return "Directive";
    case Token::Type::Instruction:
        return "Instruction";
    case Token::Type::Type:
        return "Type";
    case Token::Type::Register:
        return "Register";
    case Token::Type::Number:
        return "Number";
    case Token::Type::StringLiteral:
        return "StringLiteral";
    case Token::Type::Operator:
        return "Operator";
    case Token::Type::OpenBracket:
        return "(";
    case Token::Type::CloseBracket:
        return ")";
    case Token::Type::OpenSquareBracket:
        return "[";
    case Token::Type::CloseSquareBracket:
        return "]";
    case Token::Type::OpenAngleBracket:
        return "<";
    case Token::Type::CloseAngleBracket:
        return ">";
    case Token::Type::Comma:
        return ",";
    case Token::Type::Colon:
        return ":";
    case Token::Type::Dollar:
        return "$";
    case Token::Type::QuestionMark:
        return "?";
    case Token::Type::EndOfFile:
        return "EndOfFile";
    case Token::Type::EndOfLine:
        return "\\n";
    case Token::Type::Comment:
        return "Comment";
    case Token::Type::Invalid:
        return "Invalid Token";
    default:
        return "Unknown";
    }
}