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
    IsStructField = 1 << 2,
    IsDQDirectiveOperand = 1 << 3,
    IsDBDirectiveOperand = 1 << 4
};

inline ExprCtxtFlags operator|(ExprCtxtFlags a, ExprCtxtFlags b) { return static_cast<ExprCtxtFlags>(static_cast<int>(a) | static_cast<int>(b)); }

inline ExprCtxtFlags operator&(ExprCtxtFlags a, ExprCtxtFlags b) { return static_cast<ExprCtxtFlags>(static_cast<int>(a) & static_cast<int>(b)); }

struct ExpressionContext {
    explicit ExpressionContext(ExprCtxtFlags flags)
        : allowRegisters(bool(flags & ExprCtxtFlags::AllowRegisters)), isStructField(bool(flags & ExprCtxtFlags::IsStructField)),
          allowForwardReferences(bool(flags & ExprCtxtFlags::AllowForwardReferences)),
          isDQDirectiveOperand(bool(flags & ExprCtxtFlags::IsDQDirectiveOperand)),
          isDBDirectiveOperand(bool(flags & ExprCtxtFlags::IsDBDirectiveOperand))

    {
    }

    bool allowRegisters;
    bool isStructField;
    bool allowForwardReferences;
    bool isDQDirectiveOperand;
    bool isDBDirectiveOperand;
};

using DiagnosticPtr = std::shared_ptr<Diagnostic>;

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

    [[nodiscard]] bool visitDataItem(const std::shared_ptr<DataItem> &dataItem,
                                     const std::optional<std::shared_ptr<DataVariableSymbol>> &dataVariableSymbol);
    [[nodiscard]] bool visitInitValue(const std::shared_ptr<InitValue> &initValue,
                                      const std::optional<std::shared_ptr<DataVariableSymbol>> &dataVariableSymbol, const Token &expectedTypeToken);
    [[nodiscard]] bool visitInitValueHelper(const std::shared_ptr<InitValue> &initValue,
                                            const std::optional<std::shared_ptr<DataVariableSymbol>> &dataVariableSymbol,
                                            const Token &expectedTypeToken, int dupMultiplier);

    // ASTexpression nodes
    [[nodiscard]] bool visitExpression(const ExpressionPtr &node, const ExpressionContext &context);
    [[nodiscard]] bool visitExpressionHelper(const ExpressionPtr &node, const ExpressionContext &context);
    [[nodiscard]] bool visitBrackets(const std::shared_ptr<Brackets> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitSquareBrackets(const std::shared_ptr<SquareBrackets> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitImplicitPlusOperator(const std::shared_ptr<ImplicitPlusOperator> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitBinaryOperator(const std::shared_ptr<BinaryOperator> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitUnaryOperator(const std::shared_ptr<UnaryOperator> &node, const ExpressionContext &context);
    [[nodiscard]] bool visitLeaf(const std::shared_ptr<Leaf> &node, const ExpressionContext &context);

    // DataItem errors
    [[nodiscard]] DiagnosticPtr reportInvalidDataType(const std::shared_ptr<DataItem> &dataItem);

    // InitValue errors
    [[nodiscard]] DiagnosticPtr reportExpectedStrucOrRecordDataInitializer(const std::shared_ptr<InitValue> &initValue,
                                                                           const Token &expectedTypeToken);
    [[nodiscard]] DiagnosticPtr reportExpectedSingleItemDataInitializer(const std::shared_ptr<InitValue> &initValue, const Token &expectedTypeToken);
    [[nodiscard]] DiagnosticPtr reportTooManyInitialValuesForRecord(const std::shared_ptr<InitValue> &initValue,
                                                                    const std::shared_ptr<RecordSymbol> &recordSymbol);
    [[nodiscard]] DiagnosticPtr reportTooManyInitialValuesForStruc(const std::shared_ptr<InitValue> &initValue,
                                                                   const std::shared_ptr<StructSymbol> &structSymbol);
    [[nodiscard]] DiagnosticPtr reportInitializerTooLargeForSpecifiedSize(const std::shared_ptr<ExpressionInitValue> &initValue,
                                                                          const Token &expectedTypeToken, int32_t actualSize);

    // Instruction errors
    [[nodiscard]] DiagnosticPtr reportInvalidNumberOfOperands(const std::shared_ptr<Instruction> &instruction, const std::string &numberOfOps);
    [[nodiscard]] DiagnosticPtr reportCantHaveTwoMemoryOperands(const std::shared_ptr<Instruction> &instruction);
    [[nodiscard]] DiagnosticPtr reportDestinationOperandCantBeImmediate(const std::shared_ptr<Instruction> &instruction);
    [[nodiscard]] DiagnosticPtr reportImmediateOperandTooBigForOperand(const std::shared_ptr<Instruction> &instruction, int firstOpSize,
                                                                       int immediateOpSize);
    [[nodiscard]] DiagnosticPtr reportOneOfOperandsMustHaveSize(const std::shared_ptr<Instruction> &instruction);
    [[nodiscard]] DiagnosticPtr reportOperandsHaveDifferentSize(const std::shared_ptr<Instruction> &instruction, int firstOpSize, int secondOpSize);
    [[nodiscard]] DiagnosticPtr reportOperandMustBeMemoryOrRegisterOperand(const ExpressionPtr &operand);
    [[nodiscard]] DiagnosticPtr reportOperandMustHaveSize(const ExpressionPtr &operand);
    [[nodiscard]] DiagnosticPtr reportInvalidOperandSize(const ExpressionPtr &operand, const std::string &expectedSize, int actualSize);
    [[nodiscard]] DiagnosticPtr reportOperandMustBeRegister(const ExpressionPtr &operand);
    [[nodiscard]] DiagnosticPtr reportOperandMustBeMemoryOperand(const ExpressionPtr &operand);
    [[nodiscard]] DiagnosticPtr reportFirstOperandMustBeBiggerThanSecond(const std::shared_ptr<Instruction> &instruction, int firstOpSize,
                                                                         int secondOpSize);
    [[nodiscard]] DiagnosticPtr reportOperandMustBeImmediateOrCLRegister(const ExpressionPtr &operand);
    [[nodiscard]] DiagnosticPtr reportOperandMustBeImmediate(const ExpressionPtr &operand);
    [[nodiscard]] DiagnosticPtr reportOperandMustBeLabel(const ExpressionPtr &operand);

    // RecordDir errors
    [[nodiscard]] DiagnosticPtr reportRecordWidthTooBig(const std::shared_ptr<RecordDir> &recordDir, int32_t width);

    // RecordField errors
    [[nodiscard]] DiagnosticPtr reportRecordFieldWidthMustBePositive(const std::shared_ptr<RecordField> &recordField, int64_t width);
    [[nodiscard]] DiagnosticPtr reportRecordFieldWidthTooBig(const std::shared_ptr<RecordField> &recordField, int64_t width);

    // Expression errors
    [[nodiscard]] DiagnosticPtr reportExpressionMustBeConstant(ExpressionPtr &expr);
    [[nodiscard]] DiagnosticPtr reportUndefinedSymbol(const Token &token, bool isDefinedLater);
    [[nodiscard]] DiagnosticPtr reportRegisterNotAllowed(const Token &reg);
    [[nodiscard]] DiagnosticPtr reportNumberTooLarge(const Token &number, int maxSize);
    [[nodiscard]] DiagnosticPtr reportStringTooLarge(const Token &string);
    [[nodiscard]] DiagnosticPtr reportUnaryOperatorIncorrectArgument(const std::shared_ptr<UnaryOperator> &node);

    [[nodiscard]] DiagnosticPtr reportDotOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] DiagnosticPtr reportDotOperatorSizeNotSpecified(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] DiagnosticPtr reportDotOperatorTypeNotStruct(const std::shared_ptr<BinaryOperator> &node, const std::string &actualType);
    [[nodiscard]] DiagnosticPtr reportDotOperatorFieldDoesntExist(const std::shared_ptr<BinaryOperator> &node, const std::string &strucName,
                                                                  const std::string &fieldName);

    [[nodiscard]] DiagnosticPtr reportPtrOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);

    [[nodiscard]] DiagnosticPtr reportDivisionByZero(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] DiagnosticPtr reportInvalidScaleValue(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] DiagnosticPtr reportIncorrectIndexRegister(const std::shared_ptr<Leaf> &node);
    [[nodiscard]] DiagnosticPtr reportOtherBinaryOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] DiagnosticPtr reportCantHaveRegistersInExpression(const ExpressionPtr &node);
    [[nodiscard]] DiagnosticPtr reportCantAddVariables(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] DiagnosticPtr reportMoreThanTwoRegistersAfterAdd(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] DiagnosticPtr reportMoreThanOneScaleAfterAdd(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] DiagnosticPtr reportTwoEsp(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] DiagnosticPtr reportNon32bitRegister(const ExpressionPtr &node, bool implicit);
    [[nodiscard]] DiagnosticPtr reportBinaryMinusOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node);
    [[nodiscard]] DiagnosticPtr reportMoreThanOneRegisterInSquareBrackets(const std::shared_ptr<BinaryOperator> &node);

    void warnTypeReturnsZero(const std::shared_ptr<UnaryOperator> &node);

    static void findRelocatableVariables(const ExpressionPtr &node, std::optional<Token> &firstVar, std::optional<Token> &secondVar);

    static void findInvalidExpressionCause(const ExpressionPtr &node, ExpressionPtr &errorNode);

    static std::string getSymbolType(const std::shared_ptr<Symbol> &symbol);

    OperandSize getSizeFromToken(const Token &dataTypeToken);
    OperandSize getMinimumSizeForConstant(int64_t value);

    std::string getOperandType(const ExpressionPtr &node);

    std::shared_ptr<ParseSession> parseSess;
    ASTPtr ast;

    int pass = 1;
    int expressionDepth = 0;      // TODO: pass this using function arguments
    int dataInitializerDepth = 0; // TODO: pass this using function arguments

    uint32_t currentOffset = 0;
    uint32_t dataInitializerSize = 0; // TODO: pass this using function arguments
    ASTPtr currentLine;
    std::vector<ASTPtr> linesForSecondPass;

    static std::map<std::string, int> registerSizes;
    static std::map<int, std::string> sizeValueToStr;
    static std::map<std::string, int> sizeStrToValue;
    static std::map<std::string, std::string> dataDirectiveToSizeStr;
    static std::unordered_set<std::string> builtinTypes;
    static std::unordered_set<std::string> dataDirectives;
};

// TODO: move this function somewhere else
inline std::optional<uint64_t> parseNumber64bit(const std::string &input)
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

// TODO: move this function somewhere else
inline std::optional<uint32_t> parseNumber32bit(const std::string &input)
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
    uint32_t result = std::strtoul(numberPart.c_str(), &end, base);

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