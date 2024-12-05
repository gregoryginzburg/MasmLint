#pragma once

#include "ast.h"
#include "symbol_table.h"
#include "diag_ctxt.h"
#include "log.h"
#include "session.h"
#include <unordered_set>

enum class ExprCtxtFlags {
    None = 0,
    AllowRegisters = 1 << 0,
    AllowForwardReferences = 1 << 1,
    IsStructField = 1 << 3,
    IsPTROperand = 1 << 4,
    IsWidthOrMaskOperand = 1 << 5,
    IsSizeOperand = 1 << 6
};

inline ExprCtxtFlags operator|(ExprCtxtFlags a, ExprCtxtFlags b) { return static_cast<ExprCtxtFlags>(static_cast<int>(a) | static_cast<int>(b)); }

inline ExprCtxtFlags operator&(ExprCtxtFlags a, ExprCtxtFlags b) { return static_cast<ExprCtxtFlags>(static_cast<int>(a) & static_cast<int>(b)); }

struct ExpressionContext {
    explicit ExpressionContext(ExprCtxtFlags flags)
        : allowRegisters(bool(flags & ExprCtxtFlags::AllowRegisters)), isStructField(bool(flags & ExprCtxtFlags::IsStructField)),
          isPTROperand(bool(flags & ExprCtxtFlags::IsPTROperand)), isSizeOperand(bool(flags & ExprCtxtFlags::IsSizeOperand)),
          allowForwardReferences(bool(flags & ExprCtxtFlags::AllowForwardReferences)),
          isWidthOrMaskOperand(bool(flags & ExprCtxtFlags::IsWidthOrMaskOperand))
    {
    }

    bool allowRegisters;
    bool isStructField;
    bool isPTROperand;
    bool isSizeOperand;
    bool allowForwardReferences;
    bool isWidthOrMaskOperand;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast);

    void analyze();

private:
    void visit(const ASTPtr &node);

    [[nodiscard]] bool visitStatement(const std::shared_ptr<Statement> &statement);
    [[nodiscard]] bool visitInstruction(const std::shared_ptr<Instruction> &instruction);
    [[nodiscard]] bool visitDirective(const std::shared_ptr<Directive> &directive);

    [[nodiscard]] bool visitSegDir(const std::shared_ptr<SegDir> &segDir);
    [[nodiscard]] bool visitDataDir(const std::shared_ptr<DataDir> &dataDir, const std::optional<Token> &strucNameToken = std::nullopt);
    [[nodiscard]] bool visitStructDir(const std::shared_ptr<StructDir> &structDir);
    [[nodiscard]] bool visitProcDir(const std::shared_ptr<ProcDir> &procDir);
    [[nodiscard]] bool visitRecordDir(const std::shared_ptr<RecordDir> &recordDir);
    [[nodiscard]] bool visitRecordField(const std::shared_ptr<RecordField> &recordField);
    [[nodiscard]] bool visitEquDir(const std::shared_ptr<EquDir> &equDir);
    [[nodiscard]] bool visitEqualDir(const std::shared_ptr<EqualDir> &equalDir);
    [[nodiscard]] bool visitEndDir(const std::shared_ptr<EndDir> &endDir);

    [[nodiscard]] bool visitDataItem(const std::shared_ptr<DataItem> &dataItem);
    [[nodiscard]] bool visitInitValue(const std::shared_ptr<InitValue> &initValue, const std::string &dataType);

    // ASTexpression nodes
    [[nodiscard]] bool visitExpression(const ExpressionPtr &node, const ExpressionContext &context);
    [[nodiscard]] bool visitExpressionHelper(const ExpressionPtr &node, const ExpressionContext &context);
    [[nodiscard]] bool visitBrackets(const std::shared_ptr<Brackets> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitSquareBrackets(const std::shared_ptr<SquareBrackets> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitImplicitPlusOperator(const std::shared_ptr<ImplicitPlusOperator> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitBinaryOperator(const std::shared_ptr<BinaryOperator> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitUnaryOperator(const std::shared_ptr<UnaryOperator> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitLeaf(const std::shared_ptr<Leaf> &node, const ExpressionContext &context);

    // Instruction errors
    [[nodiscard]] std::shared_ptr<Diagnostic> reportInvalidNumberOfOperands(const std::shared_ptr<Instruction> &instruction, int numberOfOps);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportCantHaveTwoMemoryOperands(const std::shared_ptr<Instruction> &instruction);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportDestinationOperandCantBeImmediate(const std::shared_ptr<Instruction> &instruction);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportImmediateOperandTooBig(const std::shared_ptr<Instruction> &instruction, int firstOpSize,
                                                                           int immediateOpSize);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportOperandsHaveDifferentSize(const std::shared_ptr<Instruction> &instruction, int firstOpSize,
                                                                              int secondOpSize);

    // RecordDir errors
    [[nodiscard]] std::shared_ptr<Diagnostic> reportRecordWidthTooBig(const std::shared_ptr<RecordDir> &recordDir, int32_t width);

    // RecordField errors
    [[nodiscard]] std::shared_ptr<Diagnostic> reportRecordFieldWidthMustBePositive(const std::shared_ptr<RecordField> &recordField, int64_t width);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportRecordFieldWidthTooBig(const std::shared_ptr<RecordField> &recordField, int64_t width);

    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpressionMustBeMemoryOrRegisterOperand(const ExpressionPtr &operand);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpressionMustHaveSize(const ExpressionPtr &operand);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportInvalidOperandSize(const ExpressionPtr &operand, const std::string &expectedSize, int actualSize);

    // Expression errors
    [[nodiscard]] std::shared_ptr<Diagnostic> reportExpressionMustBeConstant(ExpressionPtr &expr);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportRecordNotAllowed(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportRecordFieldNotAllowed(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportUndefinedSymbol(const Token &token, bool isDefinedLater);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportTypeNotAllowed(const Token &token);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportRegisterNotAllowed(const Token &reg);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportNumberTooLarge(const Token &number);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportStringTooLarge(const Token &string);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportUnaryOperatorIncorrectArgument(const std::shared_ptr<UnaryOperator> &node);

    [[nodiscard]] std::shared_ptr<Diagnostic> reportDotOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportDotOperatorSizeNotSpecified(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportDotOperatorTypeNotStruct(const std::shared_ptr<BinaryOperator> &node,
                                                                             const std::string &actualType);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportDotOperatorFieldDoesntExist(const std::shared_ptr<BinaryOperator> &node,
                                                                                const std::string &strucName, const std::string &fieldName);

    [[nodiscard]] std::shared_ptr<Diagnostic> reportPtrOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);

    [[nodiscard]] std::shared_ptr<Diagnostic> reportDivisionByZero(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportInvalidScaleValue(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportIncorrectIndexRegister(const std::shared_ptr<Leaf> &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportOtherBinaryOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportCantHaveRegistersInExpression(const ExpressionPtr &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportCantAddVariables(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportMoreThanTwoRegistersAfterAdd(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportMoreThanOneScaleAfterAdd(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportTwoEsp(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportNon32bitRegister(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportBinaryMinusOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] std::shared_ptr<Diagnostic> reportNonRegisterInSquareBrackets(const std::shared_ptr<BinaryOperator> &node);

    void warnTypeReturnsZero(const std::shared_ptr<UnaryOperator> &node);

    static void findRelocatableVariables(const ExpressionPtr &node, std::optional<Token> &firstVar, std::optional<Token> &secondVar);

    static void findInvalidExpressionCause(const ExpressionPtr &node, ExpressionPtr &errorNode);

    std::string getOperandType(const ExpressionPtr &node);

    std::shared_ptr<ParseSession> parseSess;
    ASTPtr ast;

    int pass = 1;
    int expressionDepth = 0;

    uint32_t currentOffset = 0;
    ASTPtr currentLine;
    std::vector<ASTPtr> linesForSecondPass;

    static std::map<std::string, int> registerSizes;
    static std::map<int, std::string> sizeValueToStr;
    static std::map<std::string, int> sizeStrToValue;
    static std::unordered_set<std::string> builtinTypes;
};

// TODO: move this function somewhere else
inline std::optional<uint64_t> parseNumber(const std::string &input)
{
    if (input.empty()) {
        LOG_DETAILED_ERROR("Input string is empty!");
        return std::nullopt;
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
    if (suffix == 'h' || suffix == 'b' || suffix == 'y' || suffix == 'o' || suffix == 'q' || suffix == 'd' || suffix == 't') {
        numberPart = input.substr(0, input.size() - 1);
    }

    // Convert the string to a number
    char *end = nullptr;
    errno = 0;
    uint64_t result = std::strtoull(numberPart.c_str(), &end, base);

    if (errno == ERANGE) {
        // overflow occured
        return std::nullopt;
    } else if (errno != 0) {
        return std::nullopt;
    }

    // Check for conversion errors
    if (*end != '\0') {
        LOG_DETAILED_ERROR("Invalid number format!");
        return std::nullopt;
    }

    return result;
}