#pragma once

#include "ast.h"
#include "symbol_table.h"
#include "diag_ctxt.h"

class SemanticAnalyzer {
public:
    SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast);

    void analyze();

private:
    void visit(ASTPtr node);
    void visitExpression(ASTExpressionPtr node);
    void visitBinaryOperator(std::shared_ptr<BinaryOperator> node);
    void visitUnaryOperator(std::shared_ptr<UnaryOperator> node);
    void visitLeaf(std::shared_ptr<Leaf> node);

    ASTPtr ast;
    std::shared_ptr<ParseSession> parseSess;
};
