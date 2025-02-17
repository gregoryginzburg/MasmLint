#pragma once

#include "diag_ctxt.h"
#include "token.h"
#include "session.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>

class Preprocessor {
public:
    Preprocessor(const std::shared_ptr<ParseSession> &parseSess, const std::vector<Token> &tokens);
    std::vector<Token> preprocess();

    void advance();
    bool match(Token::Type type) const;
    bool match(const std::string &value) const;

private:
    std::shared_ptr<ParseSession> parseSess;
    const std::vector<Token> &tokens;
    size_t currentIndex = 0;
    Token currentToken;
};
