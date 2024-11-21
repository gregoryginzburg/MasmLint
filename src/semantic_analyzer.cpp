// File: semantic_analyzer.cpp
#include "semantic_analyzer.h"
#include "log.h"

SemanticAnalyzer::SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast)
    : parseSess(parseSession), ast(ast)
{
}

void SemanticAnalyzer::analyze() { visit(ast); }

void SemanticAnalyzer::visit(ASTPtr node)
{
    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        for (auto expr : program->expressions) {
            visitExpression(expr);
        }
    }
}

void SemanticAnalyzer::visitExpression(ASTExpressionPtr node)
{
    if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        visitBinaryOperator(binaryOp);
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        visitUnaryOperator(unaryOp);
    } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        visitLeaf(leaf);
    } else {
        // Handle other node types
    }
}

void SemanticAnalyzer::visitBinaryOperator(std::shared_ptr<BinaryOperator> node)
{
    visit(node->left);
    visit(node->right);

    // Perform type checking or operation validation
    // For example, ensure that operands are compatible with the operator
}

void SemanticAnalyzer::visitUnaryOperator(std::shared_ptr<UnaryOperator> node)
{
    visit(node->operand);

    // Perform checks specific to unary operators
}

void SemanticAnalyzer::visitLeaf(std::shared_ptr<Leaf> node)
{
    // Check if the identifier exists in the symbol table
    if (node->token.type == TokenType::Identifier) {
        // TODO: check if symbol is defined
        }
    // Handle other leaf types if needed
}
