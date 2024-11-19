#pragma once

#include "tokenize.h"
#include <memory>

class AST {
public:
    virtual ~AST() = default;
};

using ASTPtr = std::shared_ptr<AST>;

class BinaryOperator : public AST {
public:
    BinaryOperator(const Token &op, ASTPtr left, ASTPtr right) : op(op), left(left), right(right) {}

    Token op;
    ASTPtr left;
    ASTPtr right;
};

class Brackets : public AST {
public:
    Brackets(const Token &leftBracket, const Token &rightBracket, ASTPtr operand)
        : leftBracket(leftBracket), rightBracket(rightBracket), operand(operand)
    {
    }
    Token leftBracket;
    Token rightBracket;
    ASTPtr operand;
};

class SquareBrackets : public AST {
public:
    SquareBrackets(const Token &leftBracket, const Token &rightBracket, ASTPtr operand)
        : leftBracket(leftBracket), rightBracket(rightBracket), operand(operand)
    {
    }
    Token leftBracket;
    Token rightBracket;
    ASTPtr operand;
};

class ImplicitPlusOperator : public AST {
public:
    ImplicitPlusOperator(ASTPtr left, ASTPtr right) : left(left), right(right) {}

    ASTPtr left;
    ASTPtr right;
};

class UnaryOperator : public AST {
public:
    UnaryOperator(const Token &op, ASTPtr operand) : op(op), operand(operand) {}

    Token op;
    ASTPtr operand;
};

class Leaf : public AST {
public:
    Leaf(const Token &token) : token(token) {}

    Token token;
};


inline void printAST(const ASTPtr& node, int indent) {
    if (!node) return;

    // Create indentation string
    std::string indentation(indent, ' ');

    if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        std::cout << indentation << "BinaryOperator (" << binaryOp->op.lexeme << ")\n";
        std::cout << indentation << "Left:\n";
        printAST(binaryOp->left, indent + 2);
        std::cout << indentation << "Right:\n";
        printAST(binaryOp->right, indent + 2);
    }
    else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        std::cout << indentation << "UnaryOperator (" << unaryOp->op.lexeme << ")\n";
        std::cout << indentation << "Operand:\n";
        printAST(unaryOp->operand, indent + 2);
    }
    else if (auto brackets = std::dynamic_pointer_cast<Brackets>(node)) {
        std::cout << indentation << "Brackets\n";
        std::cout << indentation << "Operand:\n";
        printAST(brackets->operand, indent + 2);
    }
    else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(node)) {
        std::cout << indentation << "SquareBrackets\n";
        std::cout << indentation << "Operand:\n";
        printAST(squareBrackets->operand, indent + 2);
    }
    else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(node)) {
        std::cout << indentation << "ImplicitPlusOperator\n";
        std::cout << indentation << "Left:\n";
        printAST(implicitPlus->left, indent + 2);
        std::cout << indentation << "Right:\n";
        printAST(implicitPlus->right, indent + 2);
    }
    else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        std::cout << indentation << "Leaf (" << leaf->token.lexeme << ")\n";
    }
    else {
        std::cout << indentation << "Unknown AST Node\n";
    }
}