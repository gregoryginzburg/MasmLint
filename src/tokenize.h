#pragma once

#include "context.h"
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
    EndOfLine,
    EndOfFile,
    Comment,
    // Add other necessary token types.
};

struct Token {
    TokenType type;
    std::string lexeme;
    int lineNumber;
    int columnNumber;
    std::string fileName;
};

class Tokenizer {
public:
    static void init();
    static std::vector<Token> tokenize(const std::string &input);

private:
    static size_t currentIndex;
    static std::string currentLine;

    static std::unordered_set<std::string> directives;
    static std::unordered_set<std::string> instructions;
    static std::unordered_set<std::string> registers;

    static void loadReservedWords();
    static char peekChar();
    static char getChar();
    static void skipWhitespace();
    static Token lexToken();

    static bool isReservedWord(const std::string &word, TokenType &type);
};
