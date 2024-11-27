#include "preprocessor.h"
#include "token.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

Preprocessor::Preprocessor(const std::shared_ptr<ParseSession> &parseSess, const std::vector<Token> &tokens)
    : parseSess(parseSess), tokens(tokens)
{
}

std::vector<Token> Preprocessor::preprocess() const
{
    // TODO: implement preprocessing
    return tokens;
}
