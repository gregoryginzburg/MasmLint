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