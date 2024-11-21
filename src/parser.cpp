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
    ASTExpressionPtr ast;
    currentIndex = 0;
    while (true) {
        ast = parseLine();
        if (currentToken.type == TokenType::EndOfFile) {
            return ast; // End parsing when EOF is reached
        }
    }
    return ast;
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

bool Parser::match(TokenType type) { return currentToken.type == type; }

bool Parser::match(TokenType type, const std::string &value)
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
        }
    }
    return term1;
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
               match(TokenType::Register) || match(TokenType::QuestionMark, "$") || match(TokenType::Type)) {
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
            curentTokenLexemeUpper != "/" && curentTokenLexemeUpper != "PTR" && curentTokenLexemeUpper != "." &&
            curentTokenLexemeUpper != "SHL" && curentTokenLexemeUpper != "SHR") {

            // try to distinct between `(var var` and `(1 + 2`
            // when after var there aren't any vars and only possible closing things and then endofline -
            if (currentToken.type == TokenType::EndOfLine || currentToken.type == TokenType::EndOfFile) {
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
