#pragma once

#include "tokenize.h"
#include "log.h"
#include <memory>
#include <optional>
#include <map>

class AST {
public:
    AST() = default;

    virtual ~AST() = default;

    AST(const AST &) = default;
    AST &operator=(const AST &) = default;

    AST(AST &&) = default;
    AST &operator=(AST &&) = default;
};

class ASTExpression;
using ASTPtr = std::shared_ptr<AST>;
using ASTExpressionPtr = std::shared_ptr<ASTExpression>;

class Program : public AST {
public:
    explicit Program(const std::vector<ASTExpressionPtr> &expressions) : expressions(expressions) {}
    std::vector<ASTExpressionPtr> expressions;
};

// UnfinishedMemoryOperand is when [] are forgotten
enum class OperandType : uint8_t {
    ImmediateOperand,
    RegisterOperand,
    MemoryOperand,
    UnfinishedMemoryOperand,
    InvalidOperand
};

struct OperandSize {
    OperandSize(std::string symbol, int value) : symbol(std::move(symbol)), value(value) {}
    std::string symbol;
    int value;
};

class ASTExpression : public AST {
public:
    // expression attributes for semantic analysis
    std::optional<int32_t> constantValue;
    bool isRelocatable = false;
    std::map<Token, std::optional<int32_t>> registers;

    // attributes for later operands semantic analysis
    OperandType type = OperandType::InvalidOperand;
    std::optional<OperandSize> size = std::nullopt;
};

class BinaryOperator : public ASTExpression {
public:
    BinaryOperator(Token op, ASTExpressionPtr left, ASTExpressionPtr right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right))
    {
    }

    Token op;
    ASTExpressionPtr left;
    ASTExpressionPtr right;
};

class Brackets : public ASTExpression {
public:
    Brackets(Token leftBracket, Token rightBracket, ASTExpressionPtr operand)
        : leftBracket(std::move(leftBracket)), rightBracket(std::move(rightBracket)), operand(std::move(operand))
    {
    }
    Token leftBracket;
    Token rightBracket;
    ASTExpressionPtr operand;
};

class SquareBrackets : public ASTExpression {
public:
    SquareBrackets(Token leftBracket, Token rightBracket, ASTExpressionPtr operand)
        : leftBracket(std::move(leftBracket)), rightBracket(std::move(rightBracket)), operand(std::move(operand))
    {
    }
    Token leftBracket;
    Token rightBracket;
    ASTExpressionPtr operand;
};

class ImplicitPlusOperator : public ASTExpression {
public:
    ImplicitPlusOperator(ASTExpressionPtr left, ASTExpressionPtr right) : left(std::move(left)), right(std::move(right))
    {
    }

    ASTExpressionPtr left;
    ASTExpressionPtr right;
};

class UnaryOperator : public ASTExpression {
public:
    UnaryOperator(Token op, ASTExpressionPtr operand) : op(std::move(op)), operand(std::move(operand)) {}

    Token op;
    ASTExpressionPtr operand;
};

class Leaf : public ASTExpression {
public:
    explicit Leaf(Token token) : token(std::move(token)) {}

    Token token;
};

class InvalidExpression : public ASTExpression {
public:
    explicit InvalidExpression(std::shared_ptr<Diagnostic> diag) : diag(std::move(diag)) {}
    std::shared_ptr<Diagnostic> diag;
};

inline void printAST(const ASTPtr &node, size_t indent)
{
    if (!node) {
        return;
    }

    if (indent == 2) {
        auto expr = std::dynamic_pointer_cast<ASTExpression>(node);
        auto constantValue = expr->constantValue;
        if (constantValue) {
            std::cout << "constantValue: " << constantValue.value() << "\n";
        } else {
            std::cout << "constantValue: nullopt\n";
        }

        std::cout << "isRelocatable: " << (expr->isRelocatable ? "true" : "false") << "\n";

        std::cout << "registers:\n";
        for (const auto &[key, value] : expr->registers) {
            std::cout << key.lexeme;
            if (value) {
                std::cout << *value << "\n";
            } else {
                std::cout << "nullopt\n";
            }
        }

        if (expr->type == OperandType::ImmediateOperand) {
            std::cout << "type: " << "ImmediateOperand" << "\n";
        } else if (expr->type == OperandType::RegisterOperand) {
            std::cout << "type: " << "RegisterOperand" << "\n";
        } else if (expr->type == OperandType::MemoryOperand) {
            std::cout << "type: " << "MemoryOperand" << "\n";
        } else if (expr->type == OperandType::InvalidOperand) {
            std::cout << "type: " << "InvalidOperand" << "\n";
        }
        auto size = expr->size;
        if (size) {
            std::cout << "size: " << size.value().symbol << "\n";
        } else {
            std::cout << "size: nullopt\n";
        }
    }

    // Create indentation string
    std::string indentation(indent, ' ');
    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        for (const auto &expr : program->expressions) {
            std::cout << "Expr:\n";
            printAST(expr, indent + 2);
        }
    } else if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        std::cout << indentation << "BinaryOperator (" << binaryOp->op.lexeme << ")\n";
        std::cout << indentation << "Left:\n";
        printAST(binaryOp->left, indent + 2);
        std::cout << indentation << "Right:\n";
        printAST(binaryOp->right, indent + 2);
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        std::cout << indentation << "UnaryOperator (" << unaryOp->op.lexeme << ")\n";
        std::cout << indentation << "Operand:\n";
        printAST(unaryOp->operand, indent + 2);
    } else if (auto brackets = std::dynamic_pointer_cast<Brackets>(node)) {
        std::cout << indentation << "Brackets\n";
        std::cout << indentation << "Operand:\n";
        printAST(brackets->operand, indent + 2);
    } else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(node)) {
        std::cout << indentation << "SquareBrackets\n";
        std::cout << indentation << "Operand:\n";
        printAST(squareBrackets->operand, indent + 2);
    } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(node)) {
        std::cout << indentation << "ImplicitPlusOperator\n";
        std::cout << indentation << "Left:\n";
        printAST(implicitPlus->left, indent + 2);
        std::cout << indentation << "Right:\n";
        printAST(implicitPlus->right, indent + 2);
    } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        std::cout << indentation << "Leaf (" << leaf->token.lexeme << ")\n";
    } else if (auto invalidExpr = std::dynamic_pointer_cast<InvalidExpression>(node)) {
        std::cout << indentation << "Invalid Expression (" << invalidExpr->diag->getMessage() << ")\n";
    } else {
        LOG_DETAILED_ERROR("Unknown AST Node\n");
    }
}

inline Span getExpressionSpan(const ASTExpressionPtr &node)
{
    if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        return Span::merge(getExpressionSpan(binaryOp->left), getExpressionSpan(binaryOp->right));
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        return Span::merge(unaryOp->op.span, getExpressionSpan(unaryOp->operand));
    } else if (auto brackets = std::dynamic_pointer_cast<Brackets>(node)) {
        return Span::merge(brackets->leftBracket.span, brackets->rightBracket.span);
    } else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(node)) {
        return Span::merge(squareBrackets->leftBracket.span, squareBrackets->rightBracket.span);
    } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(node)) {
        return Span::merge(getExpressionSpan(implicitPlus->left), getExpressionSpan(implicitPlus->right));
    } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        return leaf->token.span;
    } else if (auto invalidExpr = std::dynamic_pointer_cast<InvalidExpression>(node)) {
        return {0, 0, nullptr};
    } else {
        LOG_DETAILED_ERROR("Unknown ASTExpression Node!\n");
        return {0, 0, nullptr};
    }
}