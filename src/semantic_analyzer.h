#pragma once

#include "ast.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "log.h"

enum class ExpressionContext : uint8_t {
    InstructionOperand,
    DataDefinition,
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast);

    void analyze();

private:
    void visit(const ASTPtr &node);

    // ASTexpression nodes
    void visitExpression(const ASTExpressionPtr &node, ExpressionContext context);
    void visitBrackets(const std::shared_ptr<Brackets> &node, ExpressionContext context);
    void visitSquareBrackets(const std::shared_ptr<SquareBrackets> &node, ExpressionContext context);
    void visitImplicitPlusOperator(const std::shared_ptr<ImplicitPlusOperator> &node, ExpressionContext context);
    void visitBinaryOperator(const std::shared_ptr<BinaryOperator> &node, ExpressionContext context);
    void visitUnaryOperator(const std::shared_ptr<UnaryOperator> &node, ExpressionContext context);
    void visitLeaf(const std::shared_ptr<Leaf> &node, ExpressionContext context);
    void visitInvalidExpression(const std::shared_ptr<InvalidExpression> &node, ExpressionContext context);

    void reportNumberTooLarge(const Token &number);
    void reportStringTooLarge(const Token &string);
    void reportUnaryOperatorIncorrectArgument(const std::shared_ptr<UnaryOperator> &node);
    void reportDotOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    void reportPtrOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    void reportDivisionByZero(const std::shared_ptr<BinaryOperator> &node);
    void reportInvalidScaleValue(const std::shared_ptr<BinaryOperator> &node);
    void reportIncorrectIndexRegister(const std::shared_ptr<Leaf> &node);
    void reportOtherBinaryOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    void reportInvalidAddressExpression(const ASTExpressionPtr &node);
    void reportCantAddVariables(const ASTExpressionPtr &node, bool implicit);
    void reportMoreThanTwoRegistersAfterAdd(const ASTExpressionPtr &node, bool implicit);
    void reportMoreThanOneScaleAfterAdd(const ASTExpressionPtr &node, bool implicit);
    void reportTwoEsp(const ASTExpressionPtr &node, bool implicit);
    void reportNon32bitRegister(const ASTExpressionPtr &node, bool implicit);
    void reportBinaryMinusOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);

    void warnTypeReturnsZero(const std::shared_ptr<UnaryOperator> &node);

    static void findRelocatableVariables(const ASTExpressionPtr &node, std::optional<Token> &firstVar,
                                         std::optional<Token> &secondVar);

    static void findInvalidExpressionCause(const ASTExpressionPtr &node, ASTExpressionPtr &errorNode);

    std::shared_ptr<ParseSession> parseSess;
    ASTPtr ast;

    bool panicLine = false;
    int expressionDepth = 0;

    static std::map<std::string, int> registerSizes;
    static std::map<int, std::string> sizeValueToStr;
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
        break;
    }

    // If there's a valid suffix, remove it from the number part
    if (suffix == 'h' || suffix == 'b' || suffix == 'y' || suffix == 'o' || suffix == 'q' || suffix == 'd' ||
        suffix == 't') {
        numberPart = input.substr(0, input.size() - 1);
    }

    // Convert the string to a number
    char *end = nullptr;
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