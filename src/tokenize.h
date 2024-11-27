#pragma once

#include "span.h"
#include "session.h"
#include "diagnostic.h"
#include "token.h"
#include <string>
#include <deque>
#include <unordered_set>


class Tokenizer {
public:
    Tokenizer(const std::shared_ptr<ParseSession> &psess, const std::string &src) : psess(psess), src(src) {}
    std::vector<Token> tokenize();

private:
    void skipWhitespace();
    Token getNextToken();
    Token getNumberToken();
    Token getIdentifierOrKeywordToken();
    Token getStringLiteralToken();
    Token getSpecialSymbolToken();
    bool isDotName();
    bool isValidNumber(const std::string &lexeme);
    bool isValidIdentifier(const std::string &lexeme);
    bool isValidIdentifierStart(char c);
    bool isValidIdentifierChar(char c);
    bool isValidNumberStart(char c);

    template <typename... Args> void addDiagnostic(size_t start, size_t end, ErrorCode errorCode, Args &&...args)
    {
        Diagnostic diag(Diagnostic::Level::Error, errorCode, std::forward<Args>(args)...);
        diag.addPrimaryLabel(Span(start, end, nullptr), "");
        psess->dcx->addDiagnostic(diag);
    }

    std::shared_ptr<ParseSession> psess;
    const std::string &src;
    std::size_t pos = 0;
    std::vector<Token> tokens;
};

