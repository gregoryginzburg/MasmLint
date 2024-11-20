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
        // Symbol *symbol = symbolTable->findSymbol(node->token.lexeme);
        // if (!symbol) {
        //     // Report an undefined symbol error
        //     Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNDEFINED_SYMBOL, node->token.lexeme);
        //     diag.addPrimaryLabel(node->token.span, "Symbol not defined");
        //     diagCtxt->addDiagnostic(diag);
        // } else {
        //     // You can perform additional checks, such as type validation
        // }
    }
    // Handle other leaf types if needed
}
