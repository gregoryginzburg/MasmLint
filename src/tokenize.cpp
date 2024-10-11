#include "tokenize.h"
#include <algorithm>
#include <cctype>

size_t Tokenizer::currentIndex;
std::string Tokenizer::currentLine;

std::unordered_set<std::string> Tokenizer::directives;
std::unordered_set<std::string> Tokenizer::instructions;
std::unordered_set<std::string> Tokenizer::registers;

void Tokenizer::init() { currentIndex = 0; }

void Tokenizer::loadReservedWords()
{
    // Initialize directives
    directives = {"DB", "DW", "DD", "%include", "%macro", "%endmacro"};

    // Initialize instructions
    instructions = {"MOV", "ADD", "SUB", "JMP", "PUSH", "POP"};

    // Initialize registers
    registers = {"AX", "BX", "CX", "DX", "SP", "BP", "SI", "DI"};
}

std::vector<Token> Tokenizer::tokenize(const std::string &input)
{
    std::vector<Token> tokens;
    currentLine = input;
    currentIndex = 0;

    while (currentIndex < currentLine.length()) {
        skipWhitespace();

        if (currentIndex >= currentLine.length()) {
            break; // End of string
        }

        Token token = lexToken();
        tokens.push_back(token);
    }

    return tokens;
}

char Tokenizer::peekChar()
{
    if (currentIndex < currentLine.length()) {
        return currentLine[currentIndex];
    }
    return '\0';
}

char Tokenizer::getChar()
{
    if (currentIndex < currentLine.length()) {
        return currentLine[currentIndex++];
    }
    return '\0';
}

void Tokenizer::skipWhitespace()
{
    while (std::isspace(peekChar())) {
        getChar();
    }
}

Token Tokenizer::lexToken()
{
    char ch = peekChar();
    int tokenColumn = static_cast<int>(currentIndex);

    // Identifiers or Reserved Words
    if (std::isalpha(ch) || ch == '_') {
        std::string lexeme;
        while (std::isalnum(peekChar()) || peekChar() == '_') {
            lexeme += getChar();
        }

        TokenType type = TokenType::Identifier;
        if (isReservedWord(lexeme, type)) {
            return {type, lexeme, Context::getLineNumber(), tokenColumn, Context::getFileName()};
        } else {
            return {TokenType::Identifier, lexeme, Context::getLineNumber(), tokenColumn, Context::getFileName()};
        }
    }

    // Numbers
    if (std::isdigit(ch)) {
        std::string lexeme;
        while (std::isdigit(peekChar())) {
            lexeme += getChar();
        }
        return {TokenType::Number, lexeme, Context::getLineNumber(), tokenColumn, Context::getFileName()};
    }

    // Operators and Separators
    if (ch == ',' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == ':' || ch == ';') {
        getChar();
        if (ch == ';') {
            // Comment
            std::string comment = currentLine.substr(currentIndex);
            currentIndex = currentLine.length();
            return {TokenType::Comment, comment, Context::getLineNumber(), tokenColumn, Context::getFileName()};
        }
        return {TokenType::Operator, std::string(1, ch), Context::getLineNumber(), tokenColumn, Context::getFileName()};
    }

    // String Literals
    if (ch == '\'' || ch == '"') {
        char quoteType = getChar(); // Consume the opening quote
        std::string lexeme;
        while (peekChar() != quoteType && peekChar() != '\0') {
            lexeme += getChar();
        }
        if (peekChar() == quoteType) {
            getChar(); // Consume the closing quote
        }
        return {TokenType::StringLiteral, lexeme, Context::getLineNumber(), tokenColumn, Context::getFileName()};
    }

    // Unknown character
    getChar(); // Consume the character
    return {TokenType::Separator, std::string(1, ch), Context::getLineNumber(), tokenColumn, Context::getFileName()};
}

bool Tokenizer::isReservedWord(const std::string &word, TokenType &type)
{
    std::string upperWord = word;
    std::transform(upperWord.begin(), upperWord.end(), upperWord.begin(), ::toupper);

    if (directives.find(upperWord) != directives.end()) {
        type = TokenType::Directive;
        return true;
    }
    if (instructions.find(upperWord) != instructions.end()) {
        type = TokenType::Instruction;
        return true;
    }
    if (registers.find(upperWord) != registers.end()) {
        type = TokenType::Register;
        return true;
    }
    return false;
}
