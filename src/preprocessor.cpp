#include "preprocessor.h"
#include "tokenize.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

Preprocessor::Preprocessor(std::shared_ptr<ParseSession> parseSess, const std::vector<Token> &tokens) : parseSess(parseSess), tokens(tokens) {}

std::vector<Token> Preprocessor::preprocess()
{
    // TODO: implement preprocessing
    return tokens;
}
