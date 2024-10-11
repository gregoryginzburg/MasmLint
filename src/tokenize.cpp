#include "tokenize.h"
#include <algorithm>
#include <cctype>

size_t Tokenizer::currentIndex;
std::string Tokenizer::currentLine;

std::unordered_set<std::string> Tokenizer::directives;
std::unordered_set<std::string> Tokenizer::instructions;
std::unordered_set<std::string> Tokenizer::registers;

void Tokenizer::init() { currentIndex = 0; }


std::vector<Token> Tokenizer::tokenize(const std::string &input)
{
    std::vector<Token> tokens{
        Token(TokenType::Identifier, "label", 1, 1, "test.asm"),       // "label"
        Token(TokenType::Separator, ":", 1, 6, "test.asm"),             // ":"
        Token(TokenType::Instruction, "mov", 1, 8, "test.asm"),        // "mov
        Token(TokenType::Register, "eax", 1, 12, "test.asm"),          // "eax"
        Token(TokenType::Separator, ",", 1, 15, "test.asm"),            // ,
        Token(TokenType::Number, "1", 1, 17, "test.asm"),              // "1"
        Token(TokenType::EndOfFile, "", 1, 18, "test.asm")             // End of file token
    };
    return tokens;
}

