#include "parser.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "log.h"

#include <ranges>

// #include <fmt/core.h>

Parser::Parser(std::shared_ptr<ParseSession> parseSession, const std::vector<Token> &tokens)
    : parseSess(parseSession), tokens(tokens), currentIndex(0)
{
    currentToken = tokens[currentIndex];
}

ASTExpressionPtr Parser::parse()
{
    return parseExpression();
    // currentIndex = 0;
    // while (currentIndex < tokens.size()) {
    //     advance();
    //     if (currentToken.type == TokenType::EndOfFile) {
    //         return; // End parsing when EOF is reached
    //     }
    //     parseLine();
    // }
}

ASTExpressionPtr Parser::parseLine()
{
    // reset panicLine, delimitersStack, ...
    return nullptr;
}

bool Parser::existUnclosedBracketsOrSquareBrackets() const
{
    bool exist = false;
    auto temp = delimitersStack;

    // Iterate through the stack by popping elements
    while (!temp.empty()) {
        Token token = temp.top();
        if (token.type == TokenType::OpenBracket || token.type == TokenType::OpenSquareBracket) {
            exist = true;
        }
        temp.pop();
    }
    return exist;
}

// Only advance when matched a not EndOfFile
void Parser::advance()
{
    if (currentToken.type == TokenType::EndOfFile) {
        LOG_DETAILED_ERROR("Trying to advance() after EndOfFile encountered!");
        return;
    }
    currentToken = tokens[++currentIndex];
}

bool Parser::match(TokenType type) { return currentToken.type == type; }

bool Parser::match(TokenType type, const std::string &value)
{
    return currentToken.type == type && currentToken.lexeme == value;
}

// Can't consume EndOfFile ?
std::optional<Token> Parser::consume(TokenType type)
{
    if (currentToken.type == type) {
        Token token = currentToken;
        advance();
        return token;
    } else {
        return std::nullopt;
    }
}

std::optional<Token> Parser::consume(TokenType type, const std::string &value)
{
    if (currentToken.type == type && currentToken.lexeme == value) {
        Token token = currentToken;
        advance();
        return token;
    } else {
        return std::nullopt;
    }
}

ASTExpressionPtr Parser::parseExpression()
{
    ASTExpressionPtr term1 = parseMultiplicativeExpression();
    while (match(TokenType::Operator, "+") || match(TokenType::Operator, "-")) {
        Token op = currentToken;
        advance();
        ASTExpressionPtr term2 = parseMultiplicativeExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTExpressionPtr Parser::parseMultiplicativeExpression()
{
    ASTExpressionPtr term1 = parseUnaryExpression();
    while (match(TokenType::Operator, "*") || match(TokenType::Operator, "/") || match(TokenType::Operator, "MOD") ||
           match(TokenType::Instruction, "SHL") || match(TokenType::Instruction, "SHR")) {
        Token op = currentToken;
        advance();
        ASTExpressionPtr term2 = parseUnaryExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTExpressionPtr Parser::parseUnaryExpression()
{
    std::vector<Token> operators;
    while (match(TokenType::Operator, "+") || match(TokenType::Operator, "-") || match(TokenType::Operator, "OFFSET") ||
           match(TokenType::Operator, "TYPE")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }
    ASTExpressionPtr term = parsePostfixExpression();
    for (Token op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ASTExpressionPtr Parser::parsePostfixExpression()
{
    ASTExpressionPtr term1 = parseMemberAccessExpression();
    while (match(TokenType::Operator, "PTR")) {
        Token op = currentToken;
        advance();
        ASTExpressionPtr term2 = parseMemberAccessExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTExpressionPtr Parser::parseMemberAccessExpression()
{
    ASTExpressionPtr term1 = parseHighPrecedenceUnaryExpression();
    while (match(TokenType::Operator, ".")) {
        Token op = currentToken;
        advance();
        ASTExpressionPtr term2 = parseHighPrecedenceUnaryExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTExpressionPtr Parser::parseHighPrecedenceUnaryExpression()
{
    std::vector<Token> operators;
    while (match(TokenType::Operator, "LENGTH") || match(TokenType::Operator, "LENGTHOF") ||
           match(TokenType::Operator, "SIZE") || match(TokenType::Operator, "SIZEOF") ||
           match(TokenType::Operator, "WIDTH") || match(TokenType::Operator, "MASK")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }

    ASTExpressionPtr term = parseIndexSequence();
    for (Token op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ASTExpressionPtr Parser::parseIndexSequence()
{
    ASTExpressionPtr term1 = parsePrimaryExpression();
    while (match(TokenType::OpenSquareBracket) || match(TokenType::OpenBracket)) {
        if (match(TokenType::OpenSquareBracket)) {
            Token leftBracket = currentToken;
            delimitersStack.push(leftBracket);
            advance();
            ASTExpressionPtr expr = parseExpression();
            std::optional<Token> rightBracket = consume(TokenType::CloseSquareBracket);
            if (!rightBracket) {
                std::shared_ptr<Diagnostic> diag = reportUnclosedDelimiterError(currentToken);
                return std::make_shared<InvalidExpression>(diag);
            }
            delimitersStack.pop();
            ASTExpressionPtr term2 = std::make_shared<SquareBrackets>(leftBracket, rightBracket.value(), expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        } else if (match(TokenType::OpenBracket)) {
            Token leftBracket = currentToken;
            delimitersStack.push(leftBracket);
            advance();
            ASTExpressionPtr expr = parseExpression();
            std::optional<Token> rightBracket = consume(TokenType::CloseBracket);
            if (!rightBracket) {
                std::shared_ptr<Diagnostic> diag = reportUnclosedDelimiterError(currentToken);
                return std::make_shared<InvalidExpression>(diag);
            }
            delimitersStack.pop();
            ASTExpressionPtr term2 = std::make_shared<Brackets>(leftBracket, rightBracket.value(), expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        }
    }
    return term1;
}

ASTExpressionPtr Parser::parsePrimaryExpression()
{
    if (match(TokenType::OpenBracket)) {
        Token leftBracket = currentToken;
        delimitersStack.push(leftBracket);
        advance();
        ASTExpressionPtr expr = parseExpression();
        std::optional<Token> rightBracket = consume(TokenType::CloseBracket);
        if (!rightBracket) {
            auto diag = reportUnclosedDelimiterError(currentToken);
            return std::make_shared<InvalidExpression>(diag);
        }
        delimitersStack.pop();
        return std::make_shared<Brackets>(leftBracket, rightBracket.value(), expr);

    } else if (match(TokenType::OpenSquareBracket)) {
        Token leftBracket = currentToken;
        delimitersStack.push(leftBracket);
        advance();
        ASTExpressionPtr expr = parseExpression();
        std::optional<Token> rightBracket = consume(TokenType::CloseSquareBracket);
        if (!rightBracket) {
            auto diag = reportUnclosedDelimiterError(currentToken);
            return std::make_shared<InvalidExpression>(diag);
        }
        delimitersStack.pop();
        return std::make_shared<SquareBrackets>(leftBracket, rightBracket.value(), expr);
    } else if (match(TokenType::Identifier) || match(TokenType::Number) || match(TokenType::StringLiteral) ||
               match(TokenType::Register) || match(TokenType::QuestionMark, "$") || match(TokenType::Type)) {
        Token token = currentToken;
        advance();
        // (var var) - can't be
        std::string curentTokenLexemeUpper = currentToken.lexeme;
        std::transform(curentTokenLexemeUpper.begin(), curentTokenLexemeUpper.end(), curentTokenLexemeUpper.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        // after leaf when there'are unclosed parenthesis `()` or `[]` must be operator or closing `)` or `]`
        // or there might be `(` or `[` - implicit plus for index operator
        if (existUnclosedBracketsOrSquareBrackets() && currentToken.type != TokenType::CloseSquareBracket &&
            currentToken.type != TokenType::CloseBracket && currentToken.type != TokenType::OpenSquareBracket &&
            currentToken.type != TokenType::OpenBracket && currentToken.type != TokenType::Operator &&
            curentTokenLexemeUpper != "SHL" && curentTokenLexemeUpper != "SHR") {

            auto diag = reportExpectedOperatorOrClosingDelimiter(currentToken);
            return std::make_shared<InvalidExpression>(diag);
        }
        // var var - can't be - handled in the upper parsing (only var is parsed as expression)
        return std::make_shared<Leaf>(token);
    } else {
        auto diag = reportExpectedExpression(currentToken);
        return std::make_shared<InvalidExpression>(diag);
    }
}
