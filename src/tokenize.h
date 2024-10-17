#pragma once

#include "span.h"
#include "session.h"
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
    Separator,
    EndOfFile,
    EndOfLine,
    Comment,
};

struct Token {
    enum TokenType type;
    std::string lexeme;
    Span span;

    // data about macro expansion
};

class Tokenizer {
public:
    Tokenizer(std::shared_ptr<ParseSession> psess, const std::string &src, std::size_t startPos)
        : psess(psess), startPos(startPos), src(src), pos(startPos)
    {
    }
    std::vector<Token> tokenize();

private:
    std::size_t startPos;
    std::size_t pos;
    const std::string &src;
    std::shared_ptr<ParseSession> psess;
};