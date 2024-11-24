#include "parser.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "log.h"

#include <ranges>

// #include <fmt/core.h>

Parser::Parser(const std::shared_ptr<ParseSession> &parseSession, const std::vector<Token> &tokens)
    : parseSess(parseSession), tokens(tokens), currentToken(tokens[currentIndex])
{
}

ASTPtr Parser::parse()
{
    std::vector<ASTExpressionPtr> expressions;
    currentIndex = 0;
    while (true) {
        ASTExpressionPtr expr = parseLine();
        expressions.push_back(expr);
        if (currentToken.type == TokenType::EndOfFile) {
            return std::make_shared<Program>(expressions);
        }
    }
    return std::make_shared<Program>(expressions);
}

ASTExpressionPtr Parser::parseLine()
{
    // reset panicLine
    panicLine = false;
    ASTExpressionPtr expr = parseExpression();

    if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
        if (!panicLine) {
            Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNEXPECTED_TOKEN, currentToken.lexeme);
            diag.addPrimaryLabel(currentToken.span, "");
            parseSess->dcx->addDiagnostic(diag);
        }
    }
    while (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
        advance();
    }
    if (currentToken.type == TokenType::EndOfFile) {
        return expr;
    }
    consume(TokenType::EndOfLine);

    return expr;
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

bool Parser::match(TokenType type) const { return currentToken.type == type; }

bool Parser::match(const std::string &value) const { return stringToUpper(currentToken.lexeme) == value; }

bool Parser::match(TokenType type, const std::string &value) const
{
    return currentToken.type == type && stringToUpper(currentToken.lexeme) == value;
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
    // need to initialize to {} before every parseExpression()
    expressionDelimitersStack = {};
    return parseExpressionHelper();
}

ASTExpressionPtr Parser::parseExpressionHelper()
{
    ASTExpressionPtr term1 = parseMultiplicativeExpression();
    while (match("+") || match("-")) {
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
    while (match("*") || match("/") || match("MOD") || match("SHL") || match("SHR")) {
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
    while (match("+") || match("-") || match("OFFSET") || match("TYPE")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }
    ASTExpressionPtr term = parsePtrExpression();
    for (const Token &op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ASTExpressionPtr Parser::parsePtrExpression()
{
    ASTExpressionPtr term1 = parseMemberAccessAndIndexingExpression();
    while (match("PTR")) {
        Token op = currentToken;
        advance();
        ASTExpressionPtr term2 = parseMemberAccessAndIndexingExpression();
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ASTExpressionPtr Parser::parseMemberAccessAndIndexingExpression()
{
    ASTExpressionPtr term1 = parseHighPrecedenceUnaryExpression();
    while (match(TokenType::OpenSquareBracket) || match(TokenType::OpenBracket) || match(".")) {
        if (match(TokenType::OpenSquareBracket)) {
            Token leftBracket = currentToken;
            expressionDelimitersStack.push(leftBracket);
            advance();
            ASTExpressionPtr expr = parseExpressionHelper();
            std::optional<Token> rightBracket = consume(TokenType::CloseSquareBracket);
            if (!rightBracket) {
                std::shared_ptr<Diagnostic> diag = reportUnclosedDelimiterError(currentToken);
                return std::make_shared<InvalidExpression>(diag);
            }
            expressionDelimitersStack.pop();
            ASTExpressionPtr term2 = std::make_shared<SquareBrackets>(leftBracket, rightBracket.value(), expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        } else if (match(TokenType::OpenBracket)) {
            Token leftBracket = currentToken;
            expressionDelimitersStack.push(leftBracket);
            advance();
            ASTExpressionPtr expr = parseExpressionHelper();
            std::optional<Token> rightBracket = consume(TokenType::CloseBracket);
            if (!rightBracket) {
                std::shared_ptr<Diagnostic> diag = reportUnclosedDelimiterError(currentToken);
                return std::make_shared<InvalidExpression>(diag);
            }
            expressionDelimitersStack.pop();
            ASTExpressionPtr term2 = std::make_shared<Brackets>(leftBracket, rightBracket.value(), expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        } else if (match(".")) {
            Token dot = currentToken;
            advance();
            if (currentToken.type != TokenType::Identifier) {
                auto diag = reportExpectedIdentifier(currentToken);
                return std::make_shared<InvalidExpression>(diag);
            }
            auto term2 = std::make_shared<Leaf>(currentToken);
            advance();
            term1 = std::make_shared<BinaryOperator>(dot, term1, term2);
        }
    }
    return term1;
}

ASTExpressionPtr Parser::parseHighPrecedenceUnaryExpression()
{
    std::vector<Token> operators;
    while (match("LENGTH") || match("LENGTHOF") || match("SIZE") || match("SIZEOF") || match("WIDTH") ||
           match("MASK")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }

    ASTExpressionPtr term = parsePrimaryExpression();
    for (const Token &op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ASTExpressionPtr Parser::parsePrimaryExpression()
{
    if (match(TokenType::OpenBracket)) {
        Token leftBracket = currentToken;
        expressionDelimitersStack.push(leftBracket);
        advance();
        ASTExpressionPtr expr = parseExpressionHelper();
        std::optional<Token> rightBracket = consume(TokenType::CloseBracket);
        if (!rightBracket) {
            auto diag = reportUnclosedDelimiterError(currentToken);
            return std::make_shared<InvalidExpression>(diag);
        }
        expressionDelimitersStack.pop();
        return std::make_shared<Brackets>(leftBracket, rightBracket.value(), expr);

    } else if (match(TokenType::OpenSquareBracket)) {
        Token leftBracket = currentToken;
        expressionDelimitersStack.push(leftBracket);
        advance();
        ASTExpressionPtr expr = parseExpressionHelper();
        std::optional<Token> rightBracket = consume(TokenType::CloseSquareBracket);
        if (!rightBracket) {
            auto diag = reportUnclosedDelimiterError(currentToken);
            return std::make_shared<InvalidExpression>(diag);
        }
        expressionDelimitersStack.pop();
        return std::make_shared<SquareBrackets>(leftBracket, rightBracket.value(), expr);
    } else if (match(TokenType::Identifier) || match(TokenType::Number) || match(TokenType::StringLiteral) ||
               match(TokenType::Register) || match(TokenType::Type) || match(TokenType::Dollar)) {
        Token token = currentToken;
        advance();
        // (var var) - can't be
        std::string curentTokenLexemeUpper = stringToUpper(currentToken.lexeme);
        // after leaf when there'are unclosed parenthesis `()` or `[]` must be operator (binary operator)
        // or closing `)` or `]`
        // or there might be `(` or `[` - implicit plus for index operator
        if (!expressionDelimitersStack.empty() && currentToken.type != TokenType::CloseSquareBracket &&
            currentToken.type != TokenType::CloseBracket && currentToken.type != TokenType::OpenSquareBracket &&
            currentToken.type != TokenType::OpenBracket && curentTokenLexemeUpper != "+" &&
            curentTokenLexemeUpper != "-" && curentTokenLexemeUpper != "*" && curentTokenLexemeUpper != "/" &&
            curentTokenLexemeUpper != "PTR" && curentTokenLexemeUpper != "." && curentTokenLexemeUpper != "MOD" &&
            curentTokenLexemeUpper != "SHL" && curentTokenLexemeUpper != "SHR") {

            // try to distinct between `(var var` and `(1 + 2` or `(1 + 2,
            // when after var there aren't any vars and only possible closing things and then endofline -
            if (currentToken.type == TokenType::EndOfLine || currentToken.type == TokenType::EndOfFile ||
                currentToken.type == TokenType::Comma || currentToken.type == TokenType::OpenAngleBracket) {
                auto diag = reportUnclosedDelimiterError(currentToken);
                return std::make_shared<InvalidExpression>(diag);
            }
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
