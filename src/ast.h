#pragma once

#include "tokenize.h"
#include <memory>

class AST {
public:
    virtual ~AST() = default;
};

class ASTExpression : public AST {};

using ASTPtr = std::shared_ptr<AST>;
using ASTExpressionPtr = std::shared_ptr<ASTExpression>;

class Program : public AST {
public:
    Program(std::vector<ASTExpressionPtr> expressions) : expressions(expressions) {}
    std::vector<ASTExpressionPtr> expressions;
};

class BinaryOperator : public ASTExpression {
public:
    BinaryOperator(const Token &op, ASTExpressionPtr left, ASTExpressionPtr right) : op(op), left(left), right(right) {}

    Token op;
    ASTExpressionPtr left;
    ASTExpressionPtr right;
};

class Brackets : public ASTExpression {
public:
    Brackets(const Token &leftBracket, const Token &rightBracket, ASTExpressionPtr operand)
        : leftBracket(leftBracket), rightBracket(rightBracket), operand(operand)
    {
    }
    Token leftBracket;
    Token rightBracket;
    ASTExpressionPtr operand;
};

class SquareBrackets : public ASTExpression {
public:
    SquareBrackets(const Token &leftBracket, const Token &rightBracket, ASTExpressionPtr operand)
        : leftBracket(leftBracket), rightBracket(rightBracket), operand(operand)
    {
    }
    Token leftBracket;
    Token rightBracket;
    ASTExpressionPtr operand;
};

class ImplicitPlusOperator : public ASTExpression {
public:
    ImplicitPlusOperator(ASTExpressionPtr left, ASTExpressionPtr right) : left(left), right(right) {}

    ASTExpressionPtr left;
    ASTExpressionPtr right;
};

class UnaryOperator : public ASTExpression {
public:
    UnaryOperator(const Token &op, ASTExpressionPtr operand) : op(op), operand(operand) {}

    Token op;
    ASTExpressionPtr operand;
};

class Leaf : public ASTExpression {
public:
    Leaf(const Token &token) : token(token) {}

    Token token;
};

class InvalidExpression : public ASTExpression {
public:
    InvalidExpression(std::shared_ptr<Diagnostic> diag) : diag(diag) {}
    std::shared_ptr<Diagnostic> diag;
};

inline void printAST(ASTPtr node, int indent)
{
    if (!node)
        return;

    // Create indentation string
    std::string indentation(indent, ' ');
    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        for (auto expr : program->expressions) {
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
        std::cout << indentation << "Unknown AST Node\n";
    }
}