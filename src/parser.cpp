#include "parser.h"
#include "symbol_table.h"
#include "diag_ctxt.h"

#include <ranges>

// #include <fmt/core.h>

Parser::Parser(std::shared_ptr<ParseSession> parseSession, const std::vector<Token> &tokens)
    : parseSess(parseSession), tokens(tokens), currentIndex(0)
{
    currentToken = tokens[currentIndex];
}

ASTPtr Parser::parse()
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

// TODO: refactor this?
void Parser::advance()
{
    if (currentIndex < tokens.size()) {
        currentToken = tokens[++currentIndex];
    } else {
        currentToken = {TokenType::EndOfFile, "", Span(1, 2, nullptr)};
    }
}

ASTPtr Parser::parseLine() { return nullptr; }

bool Parser::match(TokenType type) { return currentToken.type == type; }

bool Parser::match(TokenType type, const std::string &value)
{
    return currentToken.type == type && currentToken.lexeme == value;
}

Token Parser::consume(TokenType type)
{
    if (currentToken.type == type) {
        Token token = currentToken;
        advance();
        return token;
    } else {
        // reportError(currentToken.span, ErrorCode::EXPECTED_TOKEN, "Expected token of type");
        Token token = currentToken;
        advance();
        return token;
    }
}

Token Parser::consume(TokenType type, const std::string &value)
{
    if (currentToken.type == type && currentToken.lexeme == value) {
        Token token = currentToken;
        advance();
        return token;
    } else {
        // reportError(currentToken.span, ErrorCode::EXPECTED_TOKEN, "Expected token '" + value + "'");
        Token token = currentToken;
        advance();
        return token;
    }
}

ASTPtr Parser::parseExpression()
{
    ASTPtr term1 = parseMultiplicativeExpression();
    while (match(TokenType::Operator, "+") || match(TokenType::Operator, "-")) {
        Token op = currentToken;
        advance();
        ASTPtr term2 = parseMultiplicativeExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTPtr Parser::parseMultiplicativeExpression()
{
    ASTPtr term1 = parseUnaryExpression();
    while (match(TokenType::Operator, "*") || match(TokenType::Operator, "/") || match(TokenType::Operator, "MOD") ||
           match(TokenType::Instruction, "SHL") || match(TokenType::Instruction, "SHR")) {
        Token op = currentToken;
        advance();
        ASTPtr term2 = parseUnaryExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTPtr Parser::parseUnaryExpression()
{
    std::vector<Token> operators;
    while (match(TokenType::Operator, "+") || match(TokenType::Operator, "-") ||
           match(TokenType::Operator, "OFFSET") || match(TokenType::Operator, "TYPE")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }
    ASTPtr term = parsePostfixExpression();
    for (Token op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ASTPtr Parser::parsePostfixExpression()
{
    ASTPtr term1 = parseMemberAccessExpression();
    while (match(TokenType::Operator, "PTR")) {
        Token op = currentToken;
        advance();
        ASTPtr term2 = parseMemberAccessExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTPtr Parser::parseMemberAccessExpression()
{
    ASTPtr term1 = parseHighPrecedenceUnaryExpression();
    while (match(TokenType::Operator, ".")) {
        Token op = currentToken;
        advance();
        ASTPtr term2 = parseHighPrecedenceUnaryExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTPtr Parser::parseHighPrecedenceUnaryExpression()
{
    std::vector<Token> operators;
    while (match(TokenType::Operator, "LENGTH") || match(TokenType::Operator, "LENGTHOF") ||
           match(TokenType::Operator, "SIZE") || match(TokenType::Operator, "SIZEOF") ||
           match(TokenType::Operator, "WIDTH") || match(TokenType::Operator, "MASK")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }

    ASTPtr term = parseIndexSequence();
    for (Token op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ASTPtr Parser::parseIndexSequence()
{
    ASTPtr term1 = parsePrimaryExpression();
    while (match(TokenType::OpenSquareBracket) || match(TokenType::OpenBracket)) {
        if (match(TokenType::OpenSquareBracket)) {
            Token leftBracket = currentToken;
            advance();
            ASTPtr expr = parseExpression();
            Token rightBracket = consume(TokenType::CloseSquareBracket);
            ASTPtr term2 = std::make_shared<SquareBrackets>(leftBracket, rightBracket, expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        } else if (match(TokenType::OpenBracket)) {
            Token leftBracket = currentToken;
            advance();
            ASTPtr expr = parseExpression();
            Token rightBracket = consume(TokenType::CloseBracket);
            ASTPtr term2 = std::make_shared<Brackets>(leftBracket, rightBracket, expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        }
    }
    return term1;
}

ASTPtr Parser::parsePrimaryExpression()
{
    if (match(TokenType::OpenBracket)) {
        Token leftBracket = currentToken;
        advance();
        ASTPtr expr = parseExpression();
        Token rightBracket = consume(TokenType::CloseBracket);
        return std::make_shared<Brackets>(leftBracket, rightBracket, expr);

    } else if (match(TokenType::OpenSquareBracket)) {
        Token leftBracket = currentToken;
        advance();
        ASTPtr expr = parseExpression();
        Token rightBracket = consume(TokenType::CloseSquareBracket);
        return std::make_shared<SquareBrackets>(leftBracket, rightBracket, expr);
    } else if (match(TokenType::Identifier)) {
        Token idToken = currentToken;
        advance();
        return std::make_shared<Leaf>(idToken);
    } else if (match(TokenType::Number)) {
        Token numberToken = currentToken;
        advance();
        return std::make_shared<Leaf>(numberToken);
    } else if (match(TokenType::StringLiteral)) {
        Token stringToken = currentToken;
        advance();
        return std::make_shared<Leaf>(stringToken);
    } else if (match(TokenType::Register)) {
        Token registerToken = currentToken;
        advance();
        return std::make_shared<Leaf>(registerToken);
    } else if (match(TokenType::QuestionMark, "$")) {
        Token dollarToken = currentToken;
        advance();
        return std::make_shared<Leaf>(dollarToken);
    } else if (match(TokenType::Type)) {
        Token typeToken = currentToken;
        advance();
        return std::make_shared<Leaf>(typeToken);
    } 
    else {
        // reportError(currentToken.span, ErrorCode::EXPECTED_PRIMARY_EXPRESSION, "Expected primary expression");
        advance(); // Attempt to recover
        return nullptr;
    }
}
