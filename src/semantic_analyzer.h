#pragma once

#include "ast.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "log.h"

enum class ExpressionContext {
    InstructionOperand,
    DataDefinition,
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast);

    void analyze();

private:
    void visit(ASTPtr node);

    // ASTexpression nodes
    void visitExpression(ASTExpressionPtr node, ExpressionContext context);
    void visitBrackets(std::shared_ptr<Brackets> node, ExpressionContext context);
    void visitSquareBrackets(std::shared_ptr<SquareBrackets> node, ExpressionContext context);
    void visitImplicitPlusOperator(std::shared_ptr<ImplicitPlusOperator> node, ExpressionContext context);
    void visitBinaryOperator(std::shared_ptr<BinaryOperator> node, ExpressionContext context);
    void visitUnaryOperator(std::shared_ptr<UnaryOperator> node, ExpressionContext context);
    void visitLeaf(std::shared_ptr<Leaf> node, ExpressionContext context);
    void visitInvalidExpression(std::shared_ptr<InvalidExpression> node, ExpressionContext context);

    void reportNumberTooLarge(const Token &number);
    void reportStringTooLarge(const Token &string);
    void reportUnaryOperatorIncorrectArgument(std::shared_ptr<UnaryOperator> node);
    void reportDotOperatorIncorrectArgument(std::shared_ptr<BinaryOperator> node);
    void reportPtrOperatorIncorrectArgument(std::shared_ptr<BinaryOperator> node);
    void reportDivisionByZero(std::shared_ptr<BinaryOperator> node);
    void reportInvalidScaleValue(std::shared_ptr<BinaryOperator> node);
    void reportIncorrectIndexRegister(std::shared_ptr<Leaf> node);
    void reportOtherBinaryOperatorIncorrectArgument(std::shared_ptr<BinaryOperator> node);
    void reportCantAddVariables(ASTExpressionPtr node, bool implicit);
    void reportInvalidAddressExpression(ASTExpressionPtr node);
    void reportMoreThanTwoRegistersAfterAdd(ASTExpressionPtr node, bool implicit);
    void reportMoreThanOneScaleAfterAdd(ASTExpressionPtr node, bool implicit);
    void reportBinaryMinusOperatorIncorrectArgument(std::shared_ptr<BinaryOperator> node);

    void warnTypeReturnsZero(std::shared_ptr<UnaryOperator> node);

    static void findRelocatableVariables(ASTExpressionPtr node, std::optional<Token> &firstVar,
                                         std::optional<Token> &secondVar);

    static void findInvalidExpressionCause(ASTExpressionPtr node, ASTExpressionPtr &errorNode);

    ASTPtr ast;
    std::shared_ptr<ParseSession> parseSess;
    bool panicLine = false;
    int expressionDepth = 0;
};

// TODO: move this function somewhere else
inline std::optional<int32_t> parseNumber(const std::string &input)
{
    if (input.empty()) {
        LOG_DETAILED_ERROR("Input string is empty!");
        return -1;
    }

    char suffix = static_cast<char>(tolower(input.back()));
    int base = 10;
    std::string numberPart = input;

    switch (suffix) {
    case 'h':
        base = 16;
        break;
    case 'b':
    case 'y':
        base = 2;
        break;
    case 'o':
    case 'q':
        base = 8;
        break;
    case 'd':
    case 't':
        base = 10;
        break;
    default:
        base = 10;
        break;
    }

    // If there's a valid suffix, remove it from the number part
    if (suffix == 'h' || suffix == 'b' || suffix == 'y' || suffix == 'o' || suffix == 'q' || suffix == 'd' ||
        suffix == 't') {
        numberPart = input.substr(0, input.size() - 1);
    }

    // Convert the string to a number
    char *end;
    int64_t result = std::strtoll(numberPart.c_str(), &end, base);

    // Check for conversion errors
    if (*end != '\0') {
        LOG_DETAILED_ERROR("Invalid number format!");
        return -1;
    }

    // Check for out-of-range values for int32_t
    if (result < INT32_MIN || result > INT32_MAX) {
        return std::nullopt;
    }

    return static_cast<int32_t>(result);
}