#pragma once

#include "tokenize.h"
#include "log.h"
#include <memory>
#include <optional>
#include <map>

class AST;
class Expression;
class Statement;
class EndDir;
class LabelDef;
using ASTPtr = std::shared_ptr<AST>;
using ExpressionPtr = std::shared_ptr<Expression>;

class AST {
public:
    AST() = default;

    virtual ~AST() = default;

    AST(const AST &) = default;
    AST &operator=(const AST &) = default;

    AST(AST &&) = default;
    AST &operator=(AST &&) = default;
};

class Program : public AST {
public:
    Program(const std::vector<std::shared_ptr<Statement>> &sentences, std::shared_ptr<EndDir> endDir)
        : statements(sentences), endDir(endDir)
    {
    }
    std::vector<std::shared_ptr<Statement>> statements;
    std::shared_ptr<EndDir> endDir;
};

class Statement : public AST {};

// Init values
class InitValue : public AST {};

class DupOperator : public InitValue {
public:
    DupOperator(std::shared_ptr<ExpressionPtr> repeatCount, Token op, std::vector<std::shared_ptr<InitValue>> operands)
        : repeatCount(std::move(repeatCount)), op(std::move(op)), operands(std::move(operands))
    {
    }
    std::shared_ptr<ExpressionPtr> repeatCount;
    Token op;
    std::vector<std::shared_ptr<InitValue>> operands;
};

class StringInitValue : public InitValue {
public:
    StringInitValue(Token token) : token(std::move(token)) {}
    Token token;
};

class QuestionMarkInitValue : public InitValue {
public:
    QuestionMarkInitValue(Token token) : token(std::move(token)) {}
    Token token;
};

class AddressExprInitValue : public InitValue {
public:
    AddressExprInitValue(ExpressionPtr expr) : expr(std::move(expr)) {}
    ExpressionPtr expr;
};

class StructInitValue {
public:
    StructInitValue(std::vector<std::shared_ptr<InitValue>> fields) : fields(std::move(fields)) {}
    std::vector<std::shared_ptr<InitValue>> fields;
};

class RecordInitValue {
public:
    RecordInitValue(std::vector<std::shared_ptr<InitValue>> fields) : fields(std::move(fields)) {}
    std::vector<std::shared_ptr<InitValue>> fields;
};

// Define data (data items)
class DataItem : public AST {};

class BuiltinInstance : public DataItem {
public:
    Token dataTypeToken;
    std::vector<std::shared_ptr<InitValue>> initValues;

    BuiltinInstance(Token dataTypeToken, const std::vector<std::shared_ptr<InitValue>> &initValues)
        : dataTypeToken(std::move(dataTypeToken)), initValues(initValues)
    {
    }
};

class RecordInstance : public DataItem {
public:
    Token idToken;
    std::vector<std::shared_ptr<InitValue>> initValues;

    RecordInstance(Token idToken, const std::vector<std::shared_ptr<InitValue>> &initValues)
        : idToken(std::move(idToken)), initValues(initValues)
    {
    }
};

class StructInstance : public DataItem {
public:
    Token idToken;
    std::vector<std::shared_ptr<InitValue>> initValues;

    StructInstance(Token idToken, const std::vector<std::shared_ptr<InitValue>> &initValues)
        : idToken(std::move(idToken)), initValues(initValues)
    {
    }
};

// Directives
class Directive : public Statement {};

class SegDir : public Directive {
public:
    Token directiveToken;
    std::optional<ExpressionPtr> constExpr;

    SegDir(Token directiveToken, std::optional<ExpressionPtr> constExpr = std::nullopt)
        : directiveToken(std::move(directiveToken)), constExpr(std::move(constExpr))
    {
    }
};

class DataDir : public Directive {
public:
    std::optional<Token> idToken;
    Token directiveToken;
    std::shared_ptr<DataItem> dataItem;

    DataDir(std::optional<Token> idToken, Token directiveToken, std::shared_ptr<DataItem> dataItem)
        : idToken(std::move(idToken)), directiveToken(std::move(directiveToken)), dataItem(std::move(dataItem))
    {
    }
};

class StructDir : public Directive {
public:
    Token firstIdToken;
    Token directiveToken;
    std::vector<std::shared_ptr<DataDir>> fields;
    Token secondIdToken;
    Token endsDirToken;

    StructDir(Token firstIdToken, Token directiveToken, const std::vector<std::shared_ptr<DataDir>> &fields,
              Token secondIdToken, Token endsDirToken)
        : firstIdToken(std::move(firstIdToken)), directiveToken(std::move(directiveToken)), fields(fields),
          secondIdToken(std::move(secondIdToken)), endsDirToken(std::move(endsDirToken))
    {
    }
};

class RecordDir : public Directive {
    // TODO:
};

class EquDir : public Directive {
public:
    Token idToken;
    Token directiveToken;
    ExpressionPtr value; // TODO: can also be a string

    EquDir(Token idToken, Token directiveToken, ExpressionPtr value)
        : idToken(std::move(idToken)), directiveToken(std::move(directiveToken)), value(std::move(value))
    {
    }
};

class EqualDir : public Directive {
public:
    Token idToken;
    Token directiveToken;
    ExpressionPtr value;

    EqualDir(Token idToken, Token directiveToken, ExpressionPtr value)
        : idToken(std::move(idToken)), directiveToken(std::move(directiveToken)), value(std::move(value))
    {
    }
};

class ProcDir : public Directive {
    // TODO
};

class EndDir : public Directive {
public:
    Token endToken;
    std::optional<ExpressionPtr> addressExpr;

    EndDir(Token endToken, std::optional<ExpressionPtr> addressExpr)
        : endToken(std::move(endToken)), addressExpr(std::move(addressExpr))
    {
    }
};

// Instructions
class Instruction : public Statement {
public:
    std::shared_ptr<LabelDef> label;
    Token mnemonicToken;
    std::vector<ExpressionPtr> operands;

    Instruction(std::shared_ptr<LabelDef> label, Token mnemonicToken, const std::vector<ExpressionPtr> &operands)
        : label(label), mnemonicToken(std::move(mnemonicToken)), operands(operands)
    {
    }
};

class LabelDef : public Statement {
public:
    Token idToken;

    LabelDef(Token idToken) : idToken(std::move(idToken)) {}
};

// Expressions
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

class Expression : public AST {
public:
    // expression attributes for semantic analysis
    std::optional<int32_t> constantValue;
    bool isRelocatable = false;
    std::map<Token, std::optional<int32_t>> registers;

    // attributes for later operands semantic analysis
    OperandType type = OperandType::InvalidOperand;
    std::optional<OperandSize> size = std::nullopt;
};

class BinaryOperator : public Expression {
public:
    BinaryOperator(Token op, ExpressionPtr left, ExpressionPtr right)
        : op(std::move(op)), left(std::move(left)), right(std::move(right))
    {
    }

    Token op;
    ExpressionPtr left;
    ExpressionPtr right;
};

class Brackets : public Expression {
public:
    Brackets(Token leftBracket, Token rightBracket, ExpressionPtr operand)
        : leftBracket(std::move(leftBracket)), rightBracket(std::move(rightBracket)), operand(std::move(operand))
    {
    }
    Token leftBracket;
    Token rightBracket;
    ExpressionPtr operand;
};

class SquareBrackets : public Expression {
public:
    SquareBrackets(Token leftBracket, Token rightBracket, ExpressionPtr operand)
        : leftBracket(std::move(leftBracket)), rightBracket(std::move(rightBracket)), operand(std::move(operand))
    {
    }
    Token leftBracket;
    Token rightBracket;
    ExpressionPtr operand;
};

class ImplicitPlusOperator : public Expression {
public:
    ImplicitPlusOperator(ExpressionPtr left, ExpressionPtr right) : left(std::move(left)), right(std::move(right)) {}

    ExpressionPtr left;
    ExpressionPtr right;
};

class UnaryOperator : public Expression {
public:
    UnaryOperator(Token op, ExpressionPtr operand) : op(std::move(op)), operand(std::move(operand)) {}

    Token op;
    ExpressionPtr operand;
};

class Leaf : public Expression {
public:
    explicit Leaf(Token token) : token(std::move(token)) {}

    Token token;
};

class InvalidExpression : public Expression {
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
        auto expr = std::dynamic_pointer_cast<Expression>(node);
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
        for (const auto &expr : program->statements) {
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

inline Span getExpressionSpan(const ExpressionPtr &node)
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
        LOG_DETAILED_ERROR("Unknown Expression Node!\n");
        return {0, 0, nullptr};
    }
}