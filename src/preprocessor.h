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
    Preprocessor(std::shared_ptr<ParseSession> parseSess) : parseSess(parseSess) {}
    std::vector<Token> preprocess(const std::vector<Token> &tokens);

private:
    std::shared_ptr<ParseSession> parseSess;
};
