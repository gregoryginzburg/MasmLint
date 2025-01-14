#include "preprocessor.h"
#include "token.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

Preprocessor::Preprocessor(const std::shared_ptr<ParseSession> &parseSess, const std::vector<Token> &tokens) : parseSess(parseSess), tokens(tokens) {}

void Preprocessor::advance()
{
    if (currentToken.type == Token::Type::EndOfFile) {
        LOG_DETAILED_ERROR("Trying to advance() after EndOfFile encountered!");
        return;
    }
    currentToken = tokens[++currentIndex];
}

bool Preprocessor::match(Token::Type type) const { return currentToken.type == type; }

bool Preprocessor::match(const std::string &value) const { return stringToUpper(currentToken.lexeme) == value; }

std::vector<Token> Preprocessor::preprocess()
{
    std::vector<Token> preprocessedTokens;
    preprocessedTokens.reserve(tokens.size());

    currentIndex = 0;
    currentToken = tokens[currentIndex];
    while (!match(Token::Type::EndOfFile)) {
        // Delete all INCLUDE lines
        if (match("INCLUDE")) {
            while (!match(Token::Type::EndOfLine) && !match(Token::Type::EndOfFile)) {
                advance();
            }
        }
        preprocessedTokens.push_back(currentToken);
        advance();
    }
    preprocessedTokens.push_back(currentToken); // push EndOfFile
    return preprocessedTokens;
}
