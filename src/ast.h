#pragma once

#include "token.h"
#include "log.h"
#include "diagnostic.h"
#include <memory>
#include <optional>
#include <map>
#include <iostream>

class AST;
class Expression;
class Statement;
class EndDir;
class Instruction;
class Directive;
class InitializerList;
class RecordField;
using ASTPtr = std::shared_ptr<AST>;
using ExpressionPtr = std::shared_ptr<Expression>;

#define INVALID(node) ((node)->diagnostic)
#define INVALID_EXPRESSION(diag) (std::make_shared<Expression>(diag))

#define INVALID_STATEMENT(diag) (std::make_shared<Statement>(diag))

#define INVALID_SEG_DIR(diag) (std::make_shared<SegDir>(diag))
#define INVALID_DATA_DIR(diag) (std::make_shared<DataDir>(diag))
#define INVALID_STRUCT_DIR(diag) (std::make_shared<StructDir>(diag))
#define INVALID_PROC_DIR(diag) (std::make_shared<ProcDir>(diag))
#define INVALID_RECORD_DIR(diag) (std::make_shared<RecordDir>(diag))
#define INVALID_RECORD_FIELD(diag) (std::make_shared<RecordField>(diag))
#define INVALID_EQU_DIR(diag) (std::make_shared<EquDir>(diag))
#define INVALID_EQUAL_DIR(diag) (std::make_shared<EqualDir>(diag))
#define INVALID_END_DIR(diag) (std::make_shared<EndDir>(diag))

#define INVALID_INSTRUCTION(diag) (std::make_shared<Instruction>(diag))
#define INVALID_LABEL_DEF(diag) (std::make_shared<LabelDef>(diag))

#define INVALID_DATA_ITEM(diag) (std::make_shared<DataItem>(diag))
#define INVALID_INIT_VALUE(diag) (std::make_shared<InitValue>(diag))
#define INVALID_INITIALIZER_LIST(diag) (std::make_shared<InitializerList>(diag))

class AST {
public:
    AST() = default;

    virtual ~AST() = default;

    AST(const AST &) = default;
    AST &operator=(const AST &) = default;

    AST(AST &&) = default;
    AST &operator=(AST &&) = default;

    mutable std::optional<std::shared_ptr<Diagnostic>> diagnostic;
};

class Program : public AST {
public:
    Program(const std::vector<std::shared_ptr<Statement>> &sentences, std::shared_ptr<Directive> endDir)
        : statements(sentences), endDir(std::move(endDir))
    {
    }
    std::vector<std::shared_ptr<Statement>> statements;
    std::shared_ptr<Directive> endDir;
};

class Statement : public AST {
public:
    Statement() = default;
    explicit Statement(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit Statement(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
};

// Init values
class InitValue : public AST {
public:
    InitValue() = default;
    explicit InitValue(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
};

class DupOperator : public InitValue {
public:
    DupOperator(ExpressionPtr repeatCount, Token op, Token leftBracket, std::shared_ptr<InitializerList> operands, Token rightBracket)
        : repeatCount(std::move(repeatCount)), op(std::move(op)), leftBracket(std::move(leftBracket)), operands(std::move(operands)),
          rightBracket(std::move(rightBracket))
    {
    }
    ExpressionPtr repeatCount;
    Token op;
    Token leftBracket;
    std::shared_ptr<InitializerList> operands;
    Token rightBracket;
};

class QuestionMarkInitValue : public InitValue {
public:
    explicit QuestionMarkInitValue(Token token) : token(std::move(token)) {}
    Token token;
};

class ExpressionInitValue : public InitValue {
public:
    explicit ExpressionInitValue(ExpressionPtr expr) : expr(std::move(expr)) {}
    ExpressionPtr expr;
};

class StructOrRecordInitValue : public InitValue {
public:
    StructOrRecordInitValue(Token leftBracket, Token rightBracket, std::shared_ptr<InitializerList> fields)
        : leftBracket(std::move(leftBracket)), rightBracket(std::move(rightBracket)), initList(std::move(fields))
    {
    }
    Token leftBracket;
    Token rightBracket;
    std::shared_ptr<InitializerList> initList;
};

class InitializerList : public InitValue {
public:
    explicit InitializerList(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit InitializerList(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    explicit InitializerList(std::vector<std::shared_ptr<InitValue>> fields) : fields(std::move(fields)) {}
    std::vector<std::shared_ptr<InitValue>> fields;
};

// Define data (data items)
class DataItem : public AST {
public:
    Token dataTypeToken;
    std::shared_ptr<InitializerList> initValues;

    DataItem() = default;
    explicit DataItem(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    DataItem(Token dataTypeToken, std::shared_ptr<InitializerList> initValues)
        : dataTypeToken(std::move(dataTypeToken)), initValues(std::move(initValues))
    {
    }
};

// Directives
class Directive : public Statement {};

class SegDir : public Directive {
public:
    Token directiveToken;
    std::optional<ExpressionPtr> constExpr;

    explicit SegDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit SegDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    explicit SegDir(Token directiveToken, std::optional<ExpressionPtr> constExpr = std::nullopt)
        : directiveToken(std::move(directiveToken)), constExpr(std::move(constExpr))
    {
    }
};

class DataDir : public Directive {
public:
    std::optional<Token> idToken;
    std::shared_ptr<DataItem> dataItem;

    explicit DataDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit DataDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    DataDir(std::optional<Token> idToken, std::shared_ptr<DataItem> dataItem) : idToken(std::move(idToken)), dataItem(std::move(dataItem)) {}
};

class StructDir : public Directive {
public:
    Token firstIdToken;
    Token directiveToken;
    std::vector<std::shared_ptr<DataDir>> fields;
    Token secondIdToken;
    Token endsDirToken;

    explicit StructDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit StructDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    StructDir(Token firstIdToken, Token directiveToken, const std::vector<std::shared_ptr<DataDir>> &fields, Token secondIdToken, Token endsDirToken)
        : firstIdToken(std::move(firstIdToken)), directiveToken(std::move(directiveToken)), fields(fields), secondIdToken(std::move(secondIdToken)),
          endsDirToken(std::move(endsDirToken))
    {
    }
};

class RecordDir : public Directive {
public:
    Token idToken;
    Token directiveToken;
    std::vector<std::shared_ptr<RecordField>> fields;

    RecordDir() = default;
    explicit RecordDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit RecordDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    RecordDir(Token idToken, Token directiveToken, std::vector<std::shared_ptr<RecordField>> fields)
        : idToken(std::move(idToken)), directiveToken(std::move(directiveToken)), fields(std::move(fields))
    {
    }
};

class RecordField : public AST {
public:
    Token fieldToken;
    ExpressionPtr width;
    std::optional<ExpressionPtr> initialValue;

    explicit RecordField(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit RecordField(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    RecordField(Token fieldToken, ExpressionPtr width, std::optional<ExpressionPtr> initialValue)
        : fieldToken(std::move(fieldToken)), width(std::move(width)), initialValue(std::move(initialValue))
    {
    }
};

class EquDir : public Directive {
public:
    Token idToken;
    Token directiveToken;
    ExpressionPtr value; // TODO: can also be a string

    explicit EquDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit EquDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
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

    explicit EqualDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit EqualDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    EqualDir(Token idToken, Token directiveToken, ExpressionPtr value)
        : idToken(std::move(idToken)), directiveToken(std::move(directiveToken)), value(std::move(value))
    {
    }
};

class ProcDir : public Directive {
public:
    Token firstIdToken;
    Token directiveToken;
    std::vector<std::shared_ptr<Instruction>> instructions;
    Token secondIdToken;
    Token endpDirToken;

    explicit ProcDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit ProcDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    ProcDir(Token firstIdToken, Token directiveToken, const std::vector<std::shared_ptr<Instruction>> &fields, Token secondIdToken,
            Token endsDirToken)
        : firstIdToken(std::move(firstIdToken)), directiveToken(std::move(directiveToken)), instructions(fields),
          secondIdToken(std::move(secondIdToken)), endpDirToken(std::move(endsDirToken))
    {
    }
};

class EndDir : public Directive {
public:
    Token endToken;
    std::optional<ExpressionPtr> addressExpr;

    explicit EndDir(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit EndDir(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    EndDir(Token endToken, std::optional<ExpressionPtr> addressExpr) : endToken(std::move(endToken)), addressExpr(std::move(addressExpr)) {}
};

// Instructions
class Instruction : public Statement {
public:
    std::optional<Token> label;
    std::optional<Token> mnemonicToken;
    std::vector<ExpressionPtr> operands;

    explicit Instruction(std::optional<std::shared_ptr<Diagnostic>> diag) { diagnostic = std::move(diag); }
    explicit Instruction(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    Instruction(std::optional<Token> label, std::optional<Token> mnemonicToken, const std::vector<ExpressionPtr> &operands)
        : label(std::move(label)), mnemonicToken(std::move(mnemonicToken)), operands(operands)
    {
    }
};

// Expressions
// UnfinishedMemoryOperand is when [] are forgotten
enum class OperandType : uint8_t { ImmediateOperand, RegisterOperand, MemoryOperand, UnfinishedMemoryOperand, Unspecified };

struct OperandSize {
    OperandSize(std::string symbol, int value) : symbol(std::move(symbol)), value(value) {}
    std::string symbol;
    int value;
};

class Expression : public AST {
public:
    Expression() = default;
    explicit Expression(std::shared_ptr<Diagnostic> diag) { diagnostic = std::move(diag); }
    // expression attributes for semantic analysis
    std::optional<int32_t> constantValue;
    bool unresolvedSymbols = false;
    bool isRelocatable = false;
    std::map<Token, std::optional<int32_t>> registers;

    // attributes for later operands semantic analysis
    OperandType type = OperandType::Unspecified;
    std::optional<OperandSize> size = std::nullopt;
};

class BinaryOperator : public Expression {
public:
    BinaryOperator(Token op, ExpressionPtr left, ExpressionPtr right) : op(std::move(op)), left(std::move(left)), right(std::move(right)) {}

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

inline void printAST(const ASTPtr &node, size_t indent)
{
    if (!node) {
        return;
    }

    // Create indentation string
    std::string indentation(indent, ' ');

    if (INVALID(node)) {
        std::cout << indentation << "Invalid Node: ";
        std::cout << node->diagnostic.value()->getMessage() << "\n";
        return;
    }

    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        std::cout << indentation << "Program\n";
        std::cout << indentation << "Statements:\n";
        for (const auto &stmt : program->statements) {
            printAST(stmt, indent + 2);
        }
        if (program->endDir) {
            std::cout << indentation << "End Directive:\n";
            printAST(program->endDir, indent + 2);
        }
    } else if (auto instruction = std::dynamic_pointer_cast<Instruction>(node)) {
        std::cout << indentation << "Instruction\n";
        if (instruction->label) {
            std::cout << indentation << "Label: " << instruction->label.value().lexeme << "\n";
        }
        if (!instruction->mnemonicToken) {
            return;
        }
        std::cout << indentation << "Mnemonic: " << instruction->mnemonicToken.value().lexeme << "\n";
        std::cout << indentation << "Operands:\n";
        for (const auto &operand : instruction->operands) {
            printAST(operand, indent + 2);
        }
    } else if (auto directive = std::dynamic_pointer_cast<Directive>(node)) {
        if (auto segDir = std::dynamic_pointer_cast<SegDir>(directive)) {
            std::cout << indentation << "Segment Directive\n";
            std::cout << indentation << "Directive Token: " << segDir->directiveToken.lexeme << "\n";
            if (segDir->constExpr) {
                std::cout << indentation << "Constant Expression:\n";
                printAST(*segDir->constExpr, indent + 2);
            }
        } else if (auto dataDir = std::dynamic_pointer_cast<DataDir>(directive)) {
            std::cout << indentation << "Data Directive\n";
            if (dataDir->idToken) {
                std::cout << indentation << "Identifier: " << dataDir->idToken->lexeme << "\n";
            }
            std::cout << indentation << "Data Item:\n";
            printAST(dataDir->dataItem, indent + 2);
        } else if (auto structDir = std::dynamic_pointer_cast<StructDir>(directive)) {
            std::cout << indentation << "Struct Directive\n";
            std::cout << indentation << "First Identifier: " << structDir->firstIdToken.lexeme << "\n";
            std::cout << indentation << "Directive Token: " << structDir->directiveToken.lexeme << "\n";
            std::cout << indentation << "Fields:\n";
            for (const auto &field : structDir->fields) {
                printAST(field, indent + 2);
            }
            std::cout << indentation << "Second Identifier: " << structDir->secondIdToken.lexeme << "\n";
            std::cout << indentation << "Ends Directive Token: " << structDir->endsDirToken.lexeme << "\n";
        } else if (auto recordDir = std::dynamic_pointer_cast<RecordDir>(directive)) {
            std::cout << indentation << "Record Directive\n";
            std::cout << indentation << "Identifier: " << recordDir->idToken.lexeme << "\n";
            std::cout << indentation << "Directive Token: " << recordDir->directiveToken.lexeme << "\n";
            std::cout << indentation << "Fields:\n";
            for (const auto &field : recordDir->fields) {
                printAST(field, indent + 2);
            }
        } else if (auto equDir = std::dynamic_pointer_cast<EquDir>(directive)) {
            std::cout << indentation << "Equ Directive\n";
            std::cout << indentation << "Identifier: " << equDir->idToken.lexeme << "\n";
            std::cout << indentation << "Directive Token: " << equDir->directiveToken.lexeme << "\n";
            std::cout << indentation << "Value:\n";
            printAST(equDir->value, indent + 2);
        } else if (auto equalDir = std::dynamic_pointer_cast<EqualDir>(directive)) {
            std::cout << indentation << "Equal Directive\n";
            std::cout << indentation << "Identifier: " << equalDir->idToken.lexeme << "\n";
            std::cout << indentation << "Directive Token: " << equalDir->directiveToken.lexeme << "\n";
            std::cout << indentation << "Value:\n";
            printAST(equalDir->value, indent + 2);
        } else if (auto procDir = std::dynamic_pointer_cast<ProcDir>(directive)) {
            std::cout << indentation << "Proc Directive\n";
            std::cout << indentation << "First Identifier: " << procDir->firstIdToken.lexeme << "\n";
            std::cout << indentation << "Directive Token: " << procDir->directiveToken.lexeme << "\n";
            std::cout << indentation << "Instructions:\n";
            for (const auto &instr : procDir->instructions) {
                printAST(instr, indent + 2);
            }
            std::cout << indentation << "Second Identifier: " << procDir->secondIdToken.lexeme << "\n";
            std::cout << indentation << "Endp Directive Token: " << procDir->endpDirToken.lexeme << "\n";
        } else if (auto endDir = std::dynamic_pointer_cast<EndDir>(directive)) {
            std::cout << indentation << "End Directive\n";
            std::cout << indentation << "End Token: " << endDir->endToken.lexeme << "\n";
            if (endDir->addressExpr) {
                std::cout << indentation << "Address Expression:\n";
                printAST(*endDir->addressExpr, indent + 2);
            }
        } else {
            std::cout << indentation << "Unhandled Directive Type\n";
        }
    } else if (auto dataItem = std::dynamic_pointer_cast<DataItem>(node)) {
        std::cout << indentation << "Builtin Instance\n";
        std::cout << indentation << "Data Type Token: " << dataItem->dataTypeToken.lexeme << "\n";
        std::cout << indentation << "Init Values:\n";
        printAST(dataItem->initValues, indent + 2);

    } else if (auto initValue = std::dynamic_pointer_cast<InitValue>(node)) {
        if (auto dupOperator = std::dynamic_pointer_cast<DupOperator>(initValue)) {
            std::cout << indentation << "Dup Operator\n";
            std::cout << indentation << "Repeat Count:\n";
            if (dupOperator->repeatCount) {
                printAST(dupOperator->repeatCount, indent + 2);
            }
            std::cout << indentation << "Operator: " << dupOperator->op.lexeme << "\n";
            std::cout << indentation << "Operands:\n";
            for (const auto &operand : dupOperator->operands->fields) {
                printAST(operand, indent + 2);
            }
        } else if (auto questionMarkInitValue = std::dynamic_pointer_cast<QuestionMarkInitValue>(initValue)) {
            std::cout << indentation << "Question Mark Init Value: " << questionMarkInitValue->token.lexeme << "\n";
        } else if (auto addressExprInitValue = std::dynamic_pointer_cast<ExpressionInitValue>(initValue)) {
            std::cout << indentation << "Expression Init Value:\n";
            printAST(addressExprInitValue->expr, indent + 2);
        } else if (auto structOrRecord = std::dynamic_pointer_cast<StructOrRecordInitValue>(initValue)) {
            std::cout << indentation << "Struct or Record Init Value\n";
            std::cout << indentation << "Left Bracket: " << structOrRecord->leftBracket.lexeme << "\n";
            std::cout << indentation << "Right Bracket: " << structOrRecord->rightBracket.lexeme << "\n";
            std::cout << indentation << "Fields:\n";
            printAST(structOrRecord->initList, indent + 2);
        } else if (auto initList = std::dynamic_pointer_cast<InitializerList>(initValue)) {
            std::cout << indentation << "Initializer List\n";
            for (const auto &operand : initList->fields) {
                printAST(operand, indent + 2);
            }
        } else {
            std::cout << indentation << "Unhandled InitValue Type\n";
        }
    } else if (auto expression = std::dynamic_pointer_cast<Expression>(node)) {
        if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(expression)) {
            std::cout << indentation << "Binary Operator (" << binaryOp->op.lexeme << ")\n";
            std::cout << indentation << "Left:\n";
            printAST(binaryOp->left, indent + 2);
            std::cout << indentation << "Right:\n";
            printAST(binaryOp->right, indent + 2);
        } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(expression)) {
            std::cout << indentation << "Unary Operator (" << unaryOp->op.lexeme << ")\n";
            std::cout << indentation << "Operand:\n";
            printAST(unaryOp->operand, indent + 2);
        } else if (auto brackets = std::dynamic_pointer_cast<Brackets>(expression)) {
            std::cout << indentation << "Brackets\n";
            std::cout << indentation << "Left Bracket: " << brackets->leftBracket.lexeme << "\n";
            std::cout << indentation << "Right Bracket: " << brackets->rightBracket.lexeme << "\n";
            std::cout << indentation << "Operand:\n";
            printAST(brackets->operand, indent + 2);
        } else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(expression)) {
            std::cout << indentation << "Square Brackets\n";
            std::cout << indentation << "Left Bracket: " << squareBrackets->leftBracket.lexeme << "\n";
            std::cout << indentation << "Right Bracket: " << squareBrackets->rightBracket.lexeme << "\n";
            std::cout << indentation << "Operand:\n";
            printAST(squareBrackets->operand, indent + 2);
        } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(expression)) {
            std::cout << indentation << "Implicit Plus Operator\n";
            std::cout << indentation << "Left:\n";
            printAST(implicitPlus->left, indent + 2);
            std::cout << indentation << "Right:\n";
            printAST(implicitPlus->right, indent + 2);
        } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(expression)) {
            std::cout << indentation << "Leaf (" << leaf->token.lexeme << ")\n";
        } else {
            std::cout << indentation << "Unhandled Expression Type\n";
        }
    } else if (auto recordField = std::dynamic_pointer_cast<RecordField>(node)) {
        std::cout << indentation << "Record Field\n";
        std::cout << indentation << "Field Token: " << recordField->fieldToken.lexeme << "\n";
        std::cout << indentation << "Width:\n";
        printAST(recordField->width, indent + 2);
        if (recordField->initialValue) {
            std::cout << indentation << "Initial Value:\n";
            printAST(*recordField->initialValue, indent + 2);
        }
    } else {
        std::cout << indentation << "Unhandled AST Node Type\n";
    }
}

inline Span getExpressionSpan(const ExpressionPtr &node)
{
    if (node->diagnostic) {
        return {0, 0, nullptr};
    }
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
    } else {
        LOG_DETAILED_ERROR("Unknown Expression Node!\n");
        return {0, 0, nullptr};
    }
}

inline Span getInitValueSpan(const std::shared_ptr<InitValue> &node)
{
    if (node->diagnostic) {
        return {0, 0, nullptr};
    }

    if (auto dupOperator = std::dynamic_pointer_cast<DupOperator>(node)) {
        Span span = getExpressionSpan(dupOperator->repeatCount);
        span = Span::merge(span, dupOperator->rightBracket.span);
        return span;
    }

    if (auto questionMarkInitValue = std::dynamic_pointer_cast<QuestionMarkInitValue>(node)) {
        return questionMarkInitValue->token.span;
    }

    if (auto expressionInitValue = std::dynamic_pointer_cast<ExpressionInitValue>(node)) {
        return getExpressionSpan(expressionInitValue->expr);
    }

    if (auto structOrRecord = std::dynamic_pointer_cast<StructOrRecordInitValue>(node)) {
        Span span = Span::merge(structOrRecord->leftBracket.span, structOrRecord->rightBracket.span);
        return span;
    }

    if (auto initList = std::dynamic_pointer_cast<InitializerList>(node)) {
        if (initList->fields.empty()) {
            LOG_DETAILED_ERROR("Initalizer list length can't be 0");
            return {0, 0, nullptr};
        }

        Span result = Span::merge(getInitValueSpan(initList->fields[0]), getInitValueSpan(initList->fields.back()));
        return result;
    }

    LOG_DETAILED_ERROR("Unknown InitValue node!\n");
    return {0, 0, nullptr};
}

inline std::shared_ptr<Leaf> getLeaf(const ASTPtr &node)
{
    std::shared_ptr<Leaf> leaf = std::dynamic_pointer_cast<Leaf>(node);
    return leaf;
}

inline bool isRegister(const ASTPtr &node, const std::string &registerStr)
{
    std::shared_ptr<Leaf> leaf = std::dynamic_pointer_cast<Leaf>(node);
    if (!leaf) {
        return false;
    }
    if (leaf->type != OperandType::RegisterOperand) {
        return false;
    }
    return stringToUpper(leaf->token.lexeme) == registerStr;
}