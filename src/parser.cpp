#include "parser.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "log.h"
#include "symbol_table.h"

#include <ranges>
#include <unordered_set>

// #include <fmt/core.h>

static const std::unordered_set<std::string> dataDirectives = {"DB", "DW", "DD", "DQ"};

Parser::Parser(const std::shared_ptr<ParseSession> &parseSession, const std::vector<Token> &tokens)
    : parseSess(parseSession), tokens(tokens), currentToken(tokens[currentIndex])
{
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

void Parser::synchronize()
{
    while (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
        advance();
    }
}

bool Parser::match(TokenType type) const { return currentToken.type == type; }

bool Parser::match(const std::string &value) const { return stringToUpper(currentToken.lexeme) == value; }

bool Parser::match(const std::unordered_set<std::string> &values) const
{
    return values.contains(stringToUpper(currentToken.lexeme));
}

bool Parser::match(TokenType type, const std::string &value) const
{
    return currentToken.type == type && stringToUpper(currentToken.lexeme) == value;
}

// Can't consume EndOfFile
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
// Can't consume EndOfFile
std::optional<Token> Parser::consume(const std::string &value)
{
    if (stringToUpper(currentToken.lexeme) == value) {
        Token token = currentToken;
        advance();
        return token;
    } else {
        return std::nullopt;
    }
}

bool Parser::lookaheadMatch(size_t n, const std::string &value) const
{
    if (currentIndex + n < tokens.size()) {
        return stringToUpper(tokens[currentIndex + n].lexeme) == value;
    } else {
        return stringToUpper(tokens.back().lexeme) == value;
    }
}

bool Parser::lookaheadMatch(size_t n, const std::unordered_set<std::string> &values) const
{
    if (currentIndex + n < tokens.size()) {
        return values.contains(stringToUpper(tokens[currentIndex + n].lexeme));
    } else {
        return values.contains(stringToUpper(tokens.back().lexeme));
    }
}

bool Parser::lookaheadNextLineMatch(const std::string &value) const
{
    size_t tempIndex = currentIndex;

    while (tempIndex < tokens.size() && tokens[tempIndex].type != TokenType::EndOfLine) {
        ++tempIndex;
    }
    if (tempIndex + 1 < tokens.size()) {
        return stringToUpper(tokens[tempIndex + 1].lexeme) == value;
    } else {
        return stringToUpper(tokens.back().lexeme) == value;
    }
}

bool Parser::lookaheadMatch(size_t n, TokenType type) const
{
    if (currentIndex + n < tokens.size()) {
        return tokens[currentIndex + n].type == type;
    } else {
        return tokens.back().type == type;
    }
}

ASTPtr Parser::parse()
{
    std::vector<std::shared_ptr<Statement>> statements;
    std::shared_ptr<Directive> endDir;
    currentIndex = 0;
    currentToken = tokens[currentIndex];
    while (!match("END") && !match(TokenType::EndOfFile)) {
        if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
            std::shared_ptr<Statement> statement;
            statement = parseStatement();
            if (INVALID(statement)) {
                synchronize();
            } else {
                // remove and handle everyhting in parseStatement()?
                if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
                    reportExpectedEndOfLine(currentToken);
                    synchronize();
                    // continue parsing after synchronize
                }
            }

            // Can't consume endoffile
            if (match(TokenType::EndOfLine)) {
                consume(TokenType::EndOfLine);
            }
            statements.push_back(statement);
        } else {
            // empty line
            if (match(TokenType::EndOfLine)) {
                consume(TokenType::EndOfLine);
            }
        }
    }
    if (match("END")) {
        endDir = parseEndDir();
    } else {
        reportExpectedEndDir(currentToken);
    }
    return std::make_shared<Program>(statements, endDir);
}

std::shared_ptr<Statement> Parser::parseStatement()
{
    if (match(".CODE") || match(".DATA") || match(".STACK")) {
        // not set cursegment when parseSegDir is not successfull?
        if (match(".CODE")) {
            currentSegment = ".CODE";
        } else if (match(".DATA")) {
            currentSegment = ".DATA";
        }
        return parseSegDir();
    }
    if (match("STRUC")) {
        auto diag = reportExpectedIdentifierBeforeStruc(currentToken);
        return INVALID_STATEMENT(diag);
    } else if (match("RECORD")) {
        // TODO:
    } else if (match("PROC")) {
        auto diag = reportExpectedIdentifierBeforeProc(currentToken);
        return INVALID_STATEMENT(diag);
    } else if (match("EQU")) {
        auto diag = reportExpectedIdentifierBeforeEqu(currentToken);
        return INVALID_STATEMENT(diag);
    } else if (match("=")) {
        auto diag = reportExpectedIdentifierBeforeEqual(currentToken);
        return INVALID_STATEMENT(diag);
    }

    if (lookaheadMatch(1, "STRUC")) {
        return parseStructDir();
    } else if (lookaheadMatch(1, "PROC")) {
        return parseProcDir();
    } else if (lookaheadMatch(1, "EQU")) {
        return parseEquDir();
    } else if (lookaheadMatch(1, "=")) {
        return parseEqualDir();
    } // TODO: RECORD dir
    else {
        if (currentSegment) {
            if (currentSegment.value() == ".DATA") {
                return parseDataDir();
            } else {
                return parseInstruction();
            }
        } else {
            // TODO: change?
            Token firstToken = currentToken;
            while (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
                advance();
            }
            Token lastToken = currentToken;
            auto diag = reportMustBeInSegmentBlock(firstToken, lastToken);
            return INVALID_STATEMENT(diag);
        }
    }
}

std::shared_ptr<SegDir> Parser::parseSegDir()
{
    if (!match(".CODE") && !match(".DATA") && !match(".STACK")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_SEG_DIR(std::nullopt);
    }
    Token directiveToken = currentToken;
    std::optional<ExpressionPtr> expression;
    consume(TokenType::Directive);
    if (stringToUpper(directiveToken.lexeme) == ".STACK") {
        ExpressionPtr expr = parseExpression();
        if (INVALID(expr)) {
            return INVALID_SEG_DIR(expr->diagnostic);
        }
    }
    return std::make_shared<SegDir>(directiveToken, expression);
}

std::shared_ptr<DataDir> Parser::parseDataDir()
{
    std::optional<Token> idToken;
    // TODO: debug and test this
    if (match(dataDirectives)) {
        idToken = std::nullopt;
    } else if ((lookaheadMatch(1, dataDirectives) || lookaheadMatch(1, TokenType::Identifier))) {
        idToken = currentToken;
        if (!match(TokenType::Identifier)) {
            auto diag = reportExpectedIdentifierInDataDir(currentToken);
            return INVALID_DATA_DIR(diag);
        }
        consume(TokenType::Identifier);
    }
    std::shared_ptr<DataItem> dataItem = parseDataItem();
    if (INVALID(dataItem)) {
        return INVALID_DATA_DIR(dataItem->diagnostic);
    }
    return std::make_shared<DataDir>(idToken, dataItem);
}

std::shared_ptr<StructDir> Parser::parseStructDir()
{
    Token firstIdToken, secondIdToken;
    Token directiveToken, endsDirToken;
    std::vector<std::shared_ptr<DataDir>> fields;
    if (!match(TokenType::Identifier)) {
        auto diag = reportExpectedIdentifierInStrucDir(currentToken);
        return INVALID_STRUCT_DIR(diag);
    }
    firstIdToken = currentToken;
    consume(TokenType::Identifier);
    if (!match("STRUC")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_STRUCT_DIR(std::nullopt);
    }
    directiveToken = currentToken;
    consume("STRUC");
    if (!match(TokenType::EndOfLine)) {
        auto diag = reportExpectedEndOfLine(currentToken);
        return INVALID_STRUCT_DIR(diag);
    }
    consume(TokenType::EndOfLine);

    while (!match("ENDS") && !lookaheadMatch(1, "ENDS") && !match(TokenType::EndOfFile)) {
        if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
            std::shared_ptr<DataDir> dataDir;
            dataDir = parseDataDir();
            if (INVALID(dataDir)) {
                synchronize();
            } else {
                // remove and handle everyhting in parseStatement()?
                if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
                    reportExpectedEndOfLine(currentToken);
                    synchronize();
                    // continue parsing after synchronize
                }
            }

            // Can't consume endoffile
            if (match(TokenType::EndOfLine)) {
                consume(TokenType::EndOfLine);
            }

            fields.push_back(dataDir);

        } else {
            // empty line
            if (match(TokenType::EndOfLine)) {
                consume(TokenType::EndOfLine);
            }
        }
    }
    if (match("ENDS")) {
        auto diag = reportMissingIdentifierBeforeEnds(currentToken);
        return INVALID_STRUCT_DIR(diag);
    }

    // parse ENDS
    if (!lookaheadMatch(1, "ENDS")) {
        auto diag = reportExpectedEnds(currentToken);
        return INVALID_STRUCT_DIR(diag);
    }
    if (stringToUpper(currentToken.lexeme) != stringToUpper(firstIdToken.lexeme)) {
        auto diag = reportExpectedDifferentIdentifierInStructDir(currentToken, firstIdToken);
        return INVALID_STRUCT_DIR(diag);
    }

    secondIdToken = currentToken;
    consume(TokenType::Identifier);
    if (!match("ENDS")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_STRUCT_DIR(std::nullopt);
    }
    endsDirToken = currentToken;
    consume("ENDS");
    return std::make_shared<StructDir>(firstIdToken, directiveToken, fields, secondIdToken, endsDirToken);
}

std::shared_ptr<RecordDir> Parser::parseRecordDir() { return nullptr; }

std::shared_ptr<ProcDir> Parser::parseProcDir()
{
    Token firstIdToken, secondIdToken;
    Token directiveToken, endpDirToken;
    std::vector<std::shared_ptr<Instruction>> fields;
    if (!match(TokenType::Identifier)) {
        auto diag = reportExpectedIdentifierInProcDir(currentToken);
        return INVALID_PROC_DIR(diag);
    }
    firstIdToken = currentToken;
    consume(TokenType::Identifier);
    if (!match("PROC")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_PROC_DIR(std::nullopt);
    }
    directiveToken = currentToken;
    consume("PROC");
    if (!match(TokenType::EndOfLine)) {
        auto diag = reportExpectedEndOfLine(currentToken);
        return INVALID_PROC_DIR(diag);
    }
    consume(TokenType::EndOfLine);

    while (!match("ENDP") && !lookaheadMatch(1, "ENDP") && !match(TokenType::EndOfFile)) {
        if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
            std::shared_ptr<Instruction> instruction;
            instruction = parseInstruction();
            if (INVALID(instruction)) {
                synchronize();
            } else {
                if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
                    reportExpectedEndOfLine(currentToken);
                    synchronize();
                    // continue parsing after synchronize
                }
            }

            // Can't consume endoffile
            if (match(TokenType::EndOfLine)) {
                consume(TokenType::EndOfLine);
            }

            fields.push_back(instruction);

        } else {
            // empty line
            if (match(TokenType::EndOfLine)) {
                consume(TokenType::EndOfLine);
            }
        }
    }
    if (match("ENDP")) {
        auto diag = reportMissingIdentifierBeforeEndp(currentToken);
        return INVALID_PROC_DIR(diag);
    }

    // parse ENDP
    if (!lookaheadMatch(1, "ENDP")) {
        auto diag = reportExpectedEndp(currentToken);
        return INVALID_PROC_DIR(diag);
    }
    if (stringToUpper(currentToken.lexeme) != stringToUpper(firstIdToken.lexeme)) {
        auto diag = reportExpectedDifferentIdentifierInProcDir(currentToken, firstIdToken);
        return INVALID_PROC_DIR(diag);
    }
    secondIdToken = currentToken;
    consume(TokenType::Identifier);
    if (!match("ENDP")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_PROC_DIR(std::nullopt);
    }
    endpDirToken = currentToken;
    consume("ENDP");
    return std::make_shared<ProcDir>(firstIdToken, directiveToken, fields, secondIdToken, endpDirToken);
}

std::shared_ptr<EquDir> Parser::parseEquDir()
{
    if (!match(TokenType::Identifier)) {
        auto diag = reportExpectedIdentifierInEquDir(currentToken);
        return INVALID_EQU_DIR(diag);
    }
    Token idToken = currentToken;
    consume(TokenType::Identifier);

    if (!match("EQU")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_EQU_DIR(std::nullopt);
    }
    Token directiveToken = currentToken;
    consume(TokenType::Directive);

    // TODO: can also be a string in <> (or without <>?)
    ExpressionPtr expr = parseExpression();
    if (INVALID(expr)) {
        return INVALID_EQU_DIR(expr->diagnostic);
    }

    return std::make_shared<EquDir>(idToken, directiveToken, expr);
}

std::shared_ptr<EqualDir> Parser::parseEqualDir()
{
    if (!match(TokenType::Identifier)) {
        auto diag = reportExpectedIdentifierInEqualDir(currentToken);
        return INVALID_EQUAL_DIR(diag);
    }
    Token idToken = currentToken;
    consume(TokenType::Identifier);

    if (!match("=")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_EQUAL_DIR(std::nullopt);
    }
    Token directiveToken = currentToken;
    consume(TokenType::Directive);

    ExpressionPtr expr = parseExpression();
    if (INVALID(expr)) {
        return INVALID_EQUAL_DIR(expr->diagnostic);
    }

    return std::make_shared<EqualDir>(idToken, directiveToken, expr);
}

std::shared_ptr<EndDir> Parser::parseEndDir()
{
    if (!match("END")) {
        LOG_DETAILED_ERROR("shouldn't happen");
        return INVALID_END_DIR(std::nullopt);
    }
    Token directiveToken = currentToken;
    consume(TokenType::Directive);

    if (match(TokenType::EndOfLine) || match(TokenType::EndOfFile)) {
        return std::make_shared<EndDir>(directiveToken, std::nullopt);
    }

    ExpressionPtr expr = parseExpression();
    if (INVALID(expr)) {
        return INVALID_END_DIR(expr->diagnostic);
    }

    return std::make_shared<EndDir>(directiveToken, expr);
}

std::shared_ptr<Instruction> Parser::parseInstruction()
{
    std::optional<Token> label;
    std::optional<Token> menmonicToken;
    std::vector<ExpressionPtr> operands;
    if (lookaheadMatch(1, ":")) {
        if (!match(TokenType::Identifier)) {
            auto diag = reportExpectedIdentifierInLabel(currentToken);
            return INVALID_INSTRUCTION(diag);
        }
        Token labelToken = currentToken;
        consume(TokenType::Identifier);
        consume(":");
        label = labelToken;
    }
    if (match(TokenType::EndOfLine) || match(TokenType::EndOfFile)) {
        return std::make_shared<Instruction>(label, std::nullopt, operands);
    }

    if (!match(TokenType::Instruction)) {
        auto diag = reportExpectedInstruction(currentToken);
        return INVALID_INSTRUCTION(diag);
    }
    menmonicToken = currentToken;
    consume(TokenType::Instruction);

    if (match(TokenType::EndOfLine) || match(TokenType::EndOfFile)) {
        // 0 arguments
        return std::make_shared<Instruction>(label, menmonicToken, operands);
    }
    ExpressionPtr expr = parseExpression();
    if (INVALID(expr)) {
        return INVALID_INSTRUCTION(expr->diagnostic);
    }
    operands.push_back(expr);
    while (match(",")) {
        consume(",");
        expr = parseExpression();
        if (INVALID(expr)) {
            return INVALID_INSTRUCTION(expr->diagnostic);
        }
        operands.push_back(expr);
    }
    if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
        auto diag = reportExpectedCommaOrEndOfLine(currentToken);
        return INVALID_INSTRUCTION(diag);
    }
    return std::make_shared<Instruction>(label, menmonicToken, operands);
}

std::shared_ptr<DataItem> Parser::parseDataItem()
{
    if (!match(TokenType::Identifier) && !match(dataDirectives)) {
        auto diag = reportExpectedVariableNameOrDataDirective(currentToken);
        return INVALID_DATA_ITEM(diag);
    }

    // TODO: determine whether is's a struct or record
    Token dataTypeToken = currentToken;
    advance();
    std::shared_ptr<InitValue> initValue = parseInitValue();
    if (INVALID(initValue)) {
        return INVALID_DATA_ITEM(initValue->diagnostic);
    }
    return std::make_shared<BuiltinInstance>(dataTypeToken, initValue);
}

std::shared_ptr<InitValue> Parser::parseInitValue()
{
    dataInitializerDelimitersStack = {};
    std::shared_ptr<InitValue> initValue = parseInitializerList();
    if (INVALID(initValue)) {
        return initValue;
    }
    if (!match(TokenType::EndOfLine) && !match(TokenType::EndOfFile)) {
        auto diag = reportExpectedCommaOrEndOfLine(currentToken);
        return INVALID_INITIALIZER_LIST(diag);
    }
    return initValue;
}

std::shared_ptr<InitValue> Parser::parseSingleInitValue()
{
    if (match("<")) {
        Token leftBracket = currentToken;
        dataInitializerDelimitersStack.push(leftBracket);
        advance();
        std::shared_ptr<InitializerList> fields;
        if (match(">")) {
            Token rightBracket = currentToken;
            advance();
            dataInitializerDelimitersStack.pop();
            return std::make_shared<StructOrRecordInitValue>(leftBracket, rightBracket, fields);
        }
        fields = parseInitializerList();
        if (INVALID(fields)) {
            return INVALID_INIT_VALUE(fields->diagnostic);
        }
        std::optional<Token> rightBracket = consume(">");
        if (!rightBracket) {
            auto diag = reportUnclosedDelimiterInDataInitializer(currentToken);
            return INVALID_INIT_VALUE(diag);
        }
        dataInitializerDelimitersStack.pop();
        return std::make_shared<StructOrRecordInitValue>(leftBracket, rightBracket.value(), fields);
    } else if (match("?")) {
        Token token = currentToken;
        advance();
        return std::make_shared<QuestionMarkInitValue>(token);
    } else {
        ExpressionPtr expr = parseExpression();
        if (INVALID(expr)) {
            return INVALID_INIT_VALUE(expr->diagnostic);
        }
        if (match("DUP")) {
            Token op = currentToken;
            advance();
            std::optional<Token> leftBracket = consume("(");
            if (!leftBracket) {
                auto diag = reportExpectedOpenBracket(currentToken);
                return INVALID_INIT_VALUE(diag);
            }
            dataInitializerDelimitersStack.push(leftBracket.value());
            std::shared_ptr<InitializerList> operands = parseInitializerList();
            if (INVALID(operands)) {
                return INVALID_INIT_VALUE(operands->diagnostic);
            }
            std::optional<Token> rightBracket = consume(")");
            if (!rightBracket) {
                auto diag = reportUnclosedDelimiterInDataInitializer(currentToken);
                return INVALID_INIT_VALUE(diag);
            }
            dataInitializerDelimitersStack.pop();
            return std::make_shared<DupOperator>(expr, op, operands);
        } else {
            // <var var> - can't be
            if (!dataInitializerDelimitersStack.empty() && !match(TokenType::CloseAngleBracket) &&
                !match(TokenType::CloseBracket) && !match(TokenType::Comma)) {
                if (match(TokenType::EndOfLine) || match(TokenType::EndOfFile)) {
                    auto diag = reportUnclosedDelimiterInDataInitializer(currentToken);
                    return INVALID_INIT_VALUE(diag);
                }
                auto diag = reportExpectedCommaOrClosingDelimiter(currentToken);
                return INVALID_INIT_VALUE(diag);
            }
        }
        return std::make_shared<ExpressionInitValue>(expr);
    }
}

std::shared_ptr<InitializerList> Parser::parseInitializerList()
{
    std::vector<std::shared_ptr<InitValue>> fields;
    std::shared_ptr<InitValue> initValue = parseSingleInitValue();
    if (INVALID(initValue)) {
        return INVALID_INITIALIZER_LIST(initValue->diagnostic);
    }
    fields.push_back(initValue);
    while (match(",")) {
        advance();
        initValue = parseSingleInitValue();
        if (INVALID(initValue)) {
            return INVALID_INITIALIZER_LIST(initValue->diagnostic);
        }
        fields.push_back(initValue);
    }

    return std::make_shared<InitializerList>(fields);
}

ExpressionPtr Parser::parseExpression()
{
    // need to initialize to {} before every parseExpression()
    expressionDelimitersStack = {};
    return parseExpressionHelper();
}

ExpressionPtr Parser::parseExpressionHelper()
{
    ExpressionPtr term1 = parseMultiplicativeExpression();
    if (INVALID(term1)) {
        return term1;
    }
    while (match("+") || match("-")) {
        Token op = currentToken;
        advance();
        ExpressionPtr term2 = parseMultiplicativeExpression();
        if (INVALID(term2)) {
            return term2;
        }
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ExpressionPtr Parser::parseMultiplicativeExpression()
{
    ExpressionPtr term1 = parseUnaryExpression();
    if (INVALID(term1)) {
        return term1;
    }
    while (match("*") || match("/") || match("MOD") || match("SHL") || match("SHR")) {
        Token op = currentToken;
        advance();
        ExpressionPtr term2 = parseUnaryExpression();
        if (INVALID(term2)) {
            return term2;
        }
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ExpressionPtr Parser::parseUnaryExpression()
{
    std::vector<Token> operators;
    while (match("+") || match("-") || match("OFFSET") || match("TYPE")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }
    ExpressionPtr term = parsePtrExpression();
    if (INVALID(term)) {
        return term;
    }
    for (const Token &op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ExpressionPtr Parser::parsePtrExpression()
{
    ExpressionPtr term1 = parseMemberAccessAndIndexingExpression();
    if (INVALID(term1)) {
        return term1;
    }
    while (match("PTR")) {
        Token op = currentToken;
        advance();
        ExpressionPtr term2 = parseMemberAccessAndIndexingExpression();
        if (INVALID(term2)) {
            return term2;
        }
        term1 = std::make_shared<BinaryOperator>(op, term1, term2);
    }
    return term1;
}

ExpressionPtr Parser::parseMemberAccessAndIndexingExpression()
{
    ExpressionPtr term1 = parseHighPrecedenceUnaryExpression();
    if (INVALID(term1)) {
        return term1;
    }
    while (match(TokenType::OpenSquareBracket) || match(TokenType::OpenBracket) || match(".")) {
        if (match(TokenType::OpenSquareBracket)) {
            Token leftBracket = currentToken;
            expressionDelimitersStack.push(leftBracket);
            advance();
            ExpressionPtr expr = parseExpressionHelper();
            if (INVALID(expr)) {
                return expr;
            }
            std::optional<Token> rightBracket = consume(TokenType::CloseSquareBracket);
            if (!rightBracket) {
                std::shared_ptr<Diagnostic> diag = reportUnclosedDelimiterError(currentToken);
                return INVALID_EXPRESSION(diag);
            }
            expressionDelimitersStack.pop();
            ExpressionPtr term2 = std::make_shared<SquareBrackets>(leftBracket, rightBracket.value(), expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        } else if (match(TokenType::OpenBracket)) {
            Token leftBracket = currentToken;
            expressionDelimitersStack.push(leftBracket);
            advance();
            ExpressionPtr expr = parseExpressionHelper();
            if (INVALID(expr)) {
                return expr;
            }
            std::optional<Token> rightBracket = consume(TokenType::CloseBracket);
            if (!rightBracket) {
                std::shared_ptr<Diagnostic> diag = reportUnclosedDelimiterError(currentToken);
                return INVALID_EXPRESSION(diag);
            }
            expressionDelimitersStack.pop();
            ExpressionPtr term2 = std::make_shared<Brackets>(leftBracket, rightBracket.value(), expr);
            term1 = std::make_shared<ImplicitPlusOperator>(term1, term2);
        } else if (match(".")) {
            Token dot = currentToken;
            advance();
            if (currentToken.type != TokenType::Identifier) {
                auto diag = reportExpectedIdentifierInExpression(currentToken);
                return INVALID_EXPRESSION(diag);
            }
            auto term2 = std::make_shared<Leaf>(currentToken);
            advance();
            term1 = std::make_shared<BinaryOperator>(dot, term1, term2);
        }
    }
    return term1;
}

ExpressionPtr Parser::parseHighPrecedenceUnaryExpression()
{
    std::vector<Token> operators;
    while (match("LENGTH") || match("LENGTHOF") || match("SIZE") || match("SIZEOF") || match("WIDTH") ||
           match("MASK")) {
        Token op = currentToken;
        operators.push_back(op);
        advance();
    }

    ExpressionPtr term = parsePrimaryExpression();
    if (INVALID(term)) {
        return term;
    }
    for (const Token &op : std::ranges::reverse_view(operators)) {
        term = std::make_shared<UnaryOperator>(op, term);
    }
    return term;
}

ExpressionPtr Parser::parsePrimaryExpression()
{
    if (match(TokenType::OpenBracket)) {
        Token leftBracket = currentToken;
        expressionDelimitersStack.push(leftBracket);
        advance();
        ExpressionPtr expr = parseExpressionHelper();
        if (INVALID(expr)) {
            return expr;
        }
        std::optional<Token> rightBracket = consume(TokenType::CloseBracket);
        if (!rightBracket) {
            auto diag = reportUnclosedDelimiterError(currentToken);
            return INVALID_EXPRESSION(diag);
        }
        expressionDelimitersStack.pop();
        return std::make_shared<Brackets>(leftBracket, rightBracket.value(), expr);

    } else if (match(TokenType::OpenSquareBracket)) {
        Token leftBracket = currentToken;
        expressionDelimitersStack.push(leftBracket);
        advance();
        ExpressionPtr expr = parseExpressionHelper();
        if (INVALID(expr)) {
            return expr;
        }
        std::optional<Token> rightBracket = consume(TokenType::CloseSquareBracket);
        if (!rightBracket) {
            auto diag = reportUnclosedDelimiterError(currentToken);
            return INVALID_EXPRESSION(diag);
        }
        expressionDelimitersStack.pop();
        return std::make_shared<SquareBrackets>(leftBracket, rightBracket.value(), expr);
    } else if (match(TokenType::Identifier) || match(TokenType::Number) || match(TokenType::StringLiteral) ||
               match(TokenType::Register) || match(TokenType::Type) || match(TokenType::Dollar)) {
        Token token = currentToken;
        advance();
        std::string curentTokenLexemeUpper = stringToUpper(currentToken.lexeme);
        // (var var) - can't be
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
                currentToken.type == TokenType::Comma) {
                auto diag = reportUnclosedDelimiterError(currentToken);
                return INVALID_EXPRESSION(diag);
            }
            auto diag = reportExpectedOperatorOrClosingDelimiter(currentToken);
            return INVALID_EXPRESSION(diag);
        }
        // var var - can't be - handled in the upper parsing (only var is parsed as expression)
        return std::make_shared<Leaf>(token);
    } else {
        auto diag = reportExpectedExpression(currentToken);
        return INVALID_EXPRESSION(diag);
    }
}
