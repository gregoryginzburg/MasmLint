#pragma once

#include "diag_ctxt.h"
#include "tokenize.h"
#include "session.h"
#include <string>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>

class Preprocessor {
public:
    Preprocessor(const std::shared_ptr<ParseSession> &parseSess, const std::vector<Token> &tokens);
    std::vector<Token> preprocess() const;

private:
    std::shared_ptr<ParseSession> parseSess;
    const std::vector<Token> &tokens;
};
