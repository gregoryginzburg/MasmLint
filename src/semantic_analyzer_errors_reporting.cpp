#include "semantic_analyzer.h"
#include "log.h"
#include "token.h"

// TODO: think more about naming types for users
// dont rely on this in code
std::string SemanticAnalyzer::getOperandType(const ExpressionPtr &node)
{
    std::shared_ptr<Leaf> leaf;
    if ((leaf = std::dynamic_pointer_cast<Leaf>(node))) {
        switch (leaf->token.type) {
        case TokenType::Identifier: {
            if (!parseSess->symbolTable->findSymbol(leaf->token)) {
                return "undefined identifier";
            }
            std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(leaf->token);
            if (auto dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(symbol)) {
                return "data variable";
            } else if (auto equVariableSymbol = std::dynamic_pointer_cast<EquVariableSymbol>(symbol)) {
                return "`EQU` variable";
            } else if (auto equalVariableSymbol = std::dynamic_pointer_cast<EqualVariableSymbol>(symbol)) {
                return "`=` variable";
            } else if (auto labelSymbol = std::dynamic_pointer_cast<LabelSymbol>(symbol)) {
                return "label variable";
            } else if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {
                return "`STRUC` symbol";
            } else if (auto procSymbol = std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
                return "`PROC` variable";
            } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
                return "`RECORD` symbol";
            } else if (auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
                return "`RECORD` field symbol";
            }
        }
        case TokenType::Number:
        case TokenType::StringLiteral:
            return "constant";
        case TokenType::Type:
            return "builtin type";
        default:
            return "error";
        }
    }

    if (node->constantValue) {
        return "constant expression";
    }
    if (node->type == OperandType::RegisterOperand) {
        return "register";
    }
    if (node->type == OperandType::ImmediateOperand) {
        return "immediate operand"; // = relocatable constant expression
    }
    if (node->type == OperandType::UnfinishedMemoryOperand) {
        return "invalid expression"; // shouldn't have to display this to user, should catch it earlier!
        // return "address expression without []";
    }
    if (node->registers.empty()) {
        return "address expression";
    } else {
        return "address expression with modificators";
    }
}

std::string SemanticAnalyzer::getSymbolType(const std::shared_ptr<Symbol> &symbol)
{
    if (auto dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(symbol)) {
        return "Data Variable";
    } else if (auto equVariableSymbol = std::dynamic_pointer_cast<EquVariableSymbol>(symbol)) {
        return "EQU Variable";
    } else if (auto equalVariableSymbol = std::dynamic_pointer_cast<EqualVariableSymbol>(symbol)) {
        return "`=` Variable";
    } else if (auto labelSymbol = std::dynamic_pointer_cast<LabelSymbol>(symbol)) {
        return "Label Variable";
    } else if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {
        return "STRUC";
    } else if (auto procSymbol = std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
        return "PROC";
    } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
        return "RECORD";
    } else if (auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
        return "RECORD Field";
    }
    return "unknown";
}

// DataItem errors
DiagnosticPtr SemanticAnalyzer::reportInvalidDataType(const std::shared_ptr<DataItem> &dataItem)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_DATA_TYPE, dataItem->dataTypeToken.lexeme);
    Token dataTypeToken = dataItem->dataTypeToken;
    std::shared_ptr<Symbol> dataTypeSymbol = parseSess->symbolTable->findSymbol(dataTypeToken);
    if (!dataTypeSymbol) {
        LOG_DETAILED_ERROR("this shouldn't be null");
        return parseSess->dcx->getLastDiagnostic();
    }
    diag.addPrimaryLabel(dataTypeToken.span, fmt::format("Expected a `STRUC` or `RECORD` type, but this is a `{}`", getSymbolType(dataTypeSymbol)));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// InitValue errors
DiagnosticPtr SemanticAnalyzer::reportExpectedStrucOrRecordDataInitializer(const std::shared_ptr<InitValue> &initValue,
                                                                           const Token &expectedTypeToken)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_DATA_INITIALIZER);
    std::shared_ptr<Symbol> expectedTypeSymbol = parseSess->symbolTable->findSymbol(expectedTypeToken);
    if (std::dynamic_pointer_cast<StructSymbol>(expectedTypeSymbol)) {
        diag.addPrimaryLabel(getInitValueSpan(initValue), fmt::format("expected a `STRUC` data initializer"));
    } else if (std::dynamic_pointer_cast<RecordSymbol>(expectedTypeSymbol)) {
        diag.addPrimaryLabel(getInitValueSpan(initValue), fmt::format("expected a `RECORD` data initializer"));
    } else {
        LOG_DETAILED_ERROR("should be only record or struc expected type");
        return parseSess->dcx->getLastDiagnostic();
    }
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportExpectedSingleItemDataInitializer(const std::shared_ptr<InitValue> &initValue,
                                                                        const Token & /*expectedTypeToken*/)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_DATA_INITIALIZER);
    diag.addPrimaryLabel(getInitValueSpan(initValue), fmt::format("expected a single item data initializer"));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportTooManyInitialValuesForRecord(const std::shared_ptr<InitValue> &initValue,
                                                                    const std::shared_ptr<RecordSymbol> &recordSymbol)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::TOO_MANY_INITIAL_VALUES_FOR_RECORD);
    diag.addPrimaryLabel(getInitValueSpan(initValue), fmt::format("expected `{}` initial values or less", recordSymbol->fields.size()));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportTooManyInitialValuesForStruc(const std::shared_ptr<InitValue> &initValue,
                                                                   const std::shared_ptr<StructSymbol> &structSymbol)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::TOO_MANY_INITIAL_VALUES_FOR_STRUC);
    diag.addPrimaryLabel(getInitValueSpan(initValue), fmt::format("expected `{}` initial values or less", structSymbol->structDir->fields.size()));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportInitializerTooLargeForSpecifiedSize(const std::shared_ptr<ExpressionInitValue> &initValue,
                                                                          const Token & /*expectedTypeToken*/, int32_t actualSize)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INITIALIZER_TOO_LARGE_FOR_SPECIFIED_SIZE);
    ExpressionPtr expr = initValue->expr;
    if (expr->constantValue) {
        diag.addPrimaryLabel(getExpressionSpan(expr),
                             fmt::format("this has value `{}` and needs `{}` bytes", expr->constantValue.value(), actualSize));
    } else {
        diag.addPrimaryLabel(getExpressionSpan(expr), fmt::format("this has size `{}`", actualSize));
    }
    // TODO: add secondary label for expectedTypeToken?
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// Instruction errors

DiagnosticPtr SemanticAnalyzer::reportInvalidNumberOfOperands(const std::shared_ptr<Instruction> &instruction, int numberOfOps)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_NUMBER_OF_OPERANDS);
    Token mnemonicToken = instruction->mnemonicToken.value();
    diag.addPrimaryLabel(mnemonicToken.span, fmt::format("`{}` instruction takes {} operands", stringToUpper(mnemonicToken.lexeme), numberOfOps));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportCantHaveTwoMemoryOperands(const std::shared_ptr<Instruction> &instruction)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::CANT_HAVE_TWO_MEMORY_OPERANDS);
    Token mnemonicToken = instruction->mnemonicToken.value();
    diag.addPrimaryLabel(mnemonicToken.span, "");
    diag.addSecondaryLabel(getExpressionSpan(instruction->operands[0]), "this is a memory operand");
    diag.addSecondaryLabel(getExpressionSpan(instruction->operands[1]), "this is a memory operand");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportDestinationOperandCantBeImmediate(const std::shared_ptr<Instruction> &instruction)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DESTINATION_OPERAND_CANT_BE_IMMEDIATE);
    diag.addPrimaryLabel(getExpressionSpan(instruction->operands[0]), "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportImmediateOperandTooBig(const std::shared_ptr<Instruction> &instruction, int firstOpSize, int immediateOpSize)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::IMMEDIATE_OPERAND_TOO_BIG);
    Token mnemonicToken = instruction->mnemonicToken.value();
    diag.addPrimaryLabel(mnemonicToken.span, "");

    diag.addSecondaryLabel(getExpressionSpan(instruction->operands[0]), fmt::format("this operand has size `{}`", firstOpSize));
    diag.addSecondaryLabel(
        getExpressionSpan(instruction->operands[1]),
        fmt::format("immediate operand has value `{}` and needs `{}` bytes", instruction->operands[1]->constantValue.value(), immediateOpSize));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportOperandsHaveDifferentSize(const std::shared_ptr<Instruction> &instruction, int firstOpSize, int secondOpSize)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::OPERANDS_HAVE_DIFFERENT_SIZE);
    Token mnemonicToken = instruction->mnemonicToken.value();
    diag.addPrimaryLabel(mnemonicToken.span, "");

    diag.addSecondaryLabel(getExpressionSpan(instruction->operands[0]), fmt::format("this operand has size `{}`", firstOpSize));
    diag.addSecondaryLabel(getExpressionSpan(instruction->operands[1]), fmt::format("this operand has size `{}`", secondOpSize));

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportExpressionMustBeMemoryOrRegisterOperand(const ExpressionPtr &operand)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPRESSION_MUST_BE_MEMORY_OR_REGISTER_OPERAND);
    diag.addPrimaryLabel(getExpressionSpan(operand), fmt::format("this has type `{}`", getOperandType(operand)));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportExpressionMustHaveSize(const ExpressionPtr &operand)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPRESSION_MUST_HAVE_SIZE);
    diag.addPrimaryLabel(getExpressionSpan(operand), "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportInvalidOperandSize(const ExpressionPtr &operand, const std::string &expectedSize, int actualSize)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_OPERAND_SIZE);
    diag.addPrimaryLabel(getExpressionSpan(operand), fmt::format("this operand must have size `{}`, but it has size `{}`", expectedSize, actualSize));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// RecordDir errors
DiagnosticPtr SemanticAnalyzer::reportRecordWidthTooBig(const std::shared_ptr<RecordDir> &recordDir, int32_t width)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::RECORD_WIDTH_TOO_BIG);
    Token recordIdToken = recordDir->idToken;
    diag.addPrimaryLabel(recordIdToken.span, fmt::format("this `RECORD` has total width `{}`", width));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// RecordField errors
DiagnosticPtr SemanticAnalyzer::reportRecordFieldWidthMustBePositive(const std::shared_ptr<RecordField> &recordField, int64_t width)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::RECORD_FIELD_WIDTH_MUST_BE_POSITIVE);
    diag.addPrimaryLabel(getExpressionSpan(recordField->width), fmt::format("this evaluates to `{}`", width));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportRecordFieldWidthTooBig(const std::shared_ptr<RecordField> &recordField, int64_t width)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::RECORD_FIELD_TOO_BIG);
    diag.addPrimaryLabel(getExpressionSpan(recordField->width), fmt::format("this evaluates to `{}`", width));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// Expression errors
DiagnosticPtr SemanticAnalyzer::reportExpressionMustBeConstant(ExpressionPtr &expr)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPRESSION_MUST_BE_CONSTANT);
    diag.addPrimaryLabel(getExpressionSpan(expr), "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportUndefinedSymbol(const Token &token, bool isDefinedLater)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNDEFINED_SYMBOL, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    if (isDefinedLater) {
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(token);
        if (!symbol) {
            LOG_DETAILED_ERROR("Defined later symbol not found");
            return parseSess->dcx->getLastDiagnostic();
        }
        diag.addSecondaryLabel(symbol->token.span, "this symbol is defined later, but forward references aren't allowed");
    }
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportRegisterNotAllowed(const Token &reg)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::REGISTER_NOT_ALLOWED);
    diag.addPrimaryLabel(reg.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportNumberTooLarge(const Token &number)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::CONSTANT_TOO_LARGE);
    diag.addPrimaryLabel(number.span, "");
    diag.addNoteMessage("maximum allowed size is 32 bits");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportStringTooLarge(const Token &string)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::CONSTANT_TOO_LARGE);
    diag.addPrimaryLabel(string.span, "");
    diag.addNoteMessage("maximum allowed size is 32 bits");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportUnaryOperatorIncorrectArgument(const std::shared_ptr<UnaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNARY_OPERATOR_INCORRECT_ARGUMENT, stringToUpper(node->op.lexeme));

    std::string op = stringToUpper(node->op.lexeme);
    std::string expectedStr;
    if (op == "LENGTH" || op == "LENGTHOF" || op == "SIZE" || op == "SIZEOF") {
        expectedStr = "expected `data label`"; // TODO: change this?
    } else if (op == "WIDTH" || op == "MASK") {
        expectedStr = "expected `RECORD symbol` or `RECORD field symbol";
    } else if (op == "OFFSET") {
        expectedStr = "expected `address expression`";
    } else if (op == "TYPE") {
        expectedStr = "expected valid expression";
    } else if (op == "+" || op == "-") {
        expectedStr = "expected `constant expression`";
    }
    auto operand = node->operand;
    diag.addPrimaryLabel(node->op.span, "");
    diag.addSecondaryLabel(getExpressionSpan(operand), fmt::format("{}, found `{}`", expectedStr, getOperandType(operand)));

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// Dot Operator
DiagnosticPtr SemanticAnalyzer::reportDotOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DOT_OPERATOR_INCORRECT_ARGUMENT);

    auto left = node->left;
    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "");

    std::shared_ptr<Leaf> leaf;
    if (left->constantValue || left->type == OperandType::RegisterOperand) {
        diag.addSecondaryLabel(getExpressionSpan(left), fmt::format("expected `address expression`, found `{}`", getOperandType(left)));
    }
    if (!(leaf = std::dynamic_pointer_cast<Leaf>(right)) || leaf->token.type != TokenType::Identifier) {
        diag.addSecondaryLabel(getExpressionSpan(right), fmt::format("expected `identifier`, found `{}`", getOperandType(right)));
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportDotOperatorSizeNotSpecified(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DOT_OPERATOR_INCORRECT_ARGUMENT);
    diag.addPrimaryLabel(node->op.span, "");
    diag.addSecondaryLabel(getExpressionSpan(node->left), "this expression doesn't have a type");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportDotOperatorTypeNotStruct(const std::shared_ptr<BinaryOperator> &node, const std::string &actualType)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DOT_OPERATOR_INCORRECT_ARGUMENT);
    diag.addPrimaryLabel(node->op.span, "");
    diag.addSecondaryLabel(getExpressionSpan(node->left),
                           fmt::format("this expression must have `STRUC` type, but it has a builtin type `{}`", actualType));
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportDotOperatorFieldDoesntExist(const std::shared_ptr<BinaryOperator> &node, const std::string &strucName,
                                                                  const std::string &fieldName)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DOT_OPERATOR_FIELD_DOESNT_EXIST, strucName, fieldName);
    diag.addPrimaryLabel(getExpressionSpan(node->right), "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// PTR operator
DiagnosticPtr SemanticAnalyzer::reportPtrOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::PTR_OPERATOR_INCORRECT_ARGUMENT);

    auto left = node->left;
    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "");

    std::shared_ptr<Leaf> leaf;
    // CRITICAL TODO: left can also be identifier, if it's a type symbol
    if (!(leaf = std::dynamic_pointer_cast<Leaf>(left)) || leaf->token.type != TokenType::Type /* !=TokenType::Identifier */) {
        diag.addSecondaryLabel(getExpressionSpan(left), fmt::format("expected `type`, found `{}`", getOperandType(left)));
    }
    if (right->type == OperandType::UnfinishedMemoryOperand || right->type == OperandType::RegisterOperand) {
        // Change that we can expect also constants?
        diag.addSecondaryLabel(getExpressionSpan(right), fmt::format("expected `address expression`, found `{}`", getOperandType(right)));
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportDivisionByZero(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DIVISION_BY_ZERO_IN_EXPRESSION);

    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "");

    diag.addSecondaryLabel(getExpressionSpan(right), "this evaluates to `0`");

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportInvalidScaleValue(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_SCALE_VALUE);

    auto left = node->left;
    auto right = node->right;

    if (left->constantValue) {
        diag.addPrimaryLabel(getExpressionSpan(left), fmt::format("this evaluates to `{}`", left->constantValue.value()));
    } else {
        diag.addPrimaryLabel(getExpressionSpan(right), fmt::format("this evaluates to `{}`", right->constantValue.value()));
    }

    diag.addNoteMessage("scale can only be {1, 2, 4, 8}");

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportIncorrectIndexRegister(const std::shared_ptr<Leaf> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INCORRECT_INDEX_REGISTER);
    diag.addPrimaryLabel(node->token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportOtherBinaryOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::OTHER_BINARY_OPERATOR_INCORRECT_ARGUMENT, node->op.lexeme);

    auto left = node->left;
    auto right = node->right;

    if (left->type == OperandType::RegisterOperand && node->op.lexeme == "*") {
        diag.addPrimaryLabel(node->op.span, "");
        diag.addSecondaryLabel(getExpressionSpan(right), fmt::format("expected `constant expression`, found `{}`", getOperandType(right)));
    } else if (right->type == OperandType::RegisterOperand && node->op.lexeme == "*") {
        diag.addPrimaryLabel(node->op.span, "");
        diag.addSecondaryLabel(getExpressionSpan(left), fmt::format("expected `constant expression`, found `{}`", getOperandType(left)));
    } else {
        if (node->op.lexeme == "*") {
            diag.addPrimaryLabel(node->op.span, "can only multiply constant expressions or a register by the scale");
        } else {
            diag.addPrimaryLabel(node->op.span, fmt::format("operator `{}` supports only constant expressions", node->op.lexeme));
        }

        diag.addSecondaryLabel(getExpressionSpan(left), fmt::format("help: this has type `{}`", getOperandType(left)));
        diag.addSecondaryLabel(getExpressionSpan(right), fmt::format("help: this has type `{}`", getOperandType(right)));
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// called when there's unfinished memory operand that needs to be finished
DiagnosticPtr SemanticAnalyzer::reportCantHaveRegistersInExpression(const ExpressionPtr &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::CANT_HAVE_REGISTERS_IN_EXPRESSION);

    // find first thing that lead to UnfinishedMemoryOperand and print it
    ExpressionPtr errorNode;
    findInvalidExpressionCause(node, errorNode);

    if (!errorNode) {
        LOG_DETAILED_ERROR("Can't find invalid expression cause");
    } else {
        if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(errorNode)) {
            diag.addPrimaryLabel(getExpressionSpan(binaryOp), "");

        } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(errorNode)) {
            diag.addPrimaryLabel(getExpressionSpan(implicitPlus), "");
        }
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

void SemanticAnalyzer::findInvalidExpressionCause(const ExpressionPtr &node, ExpressionPtr &errorNode)
{
    if (node->diagnostic) {
        return;
    }
    if (node->type == OperandType::UnfinishedMemoryOperand) {
        errorNode = node;
    }
    if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        if (binaryOp->left->type == OperandType::UnfinishedMemoryOperand) {
            errorNode = binaryOp;
            findInvalidExpressionCause(binaryOp->left, errorNode);
        }
        if (binaryOp->right->type == OperandType::UnfinishedMemoryOperand) {
            errorNode = binaryOp;
            findInvalidExpressionCause(binaryOp->right, errorNode);
        }
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        findInvalidExpressionCause(unaryOp->operand, errorNode);
    } else if (auto brackets = std::dynamic_pointer_cast<Brackets>(node)) {
        findInvalidExpressionCause(brackets->operand, errorNode);
    } else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(node)) {
        findInvalidExpressionCause(squareBrackets->operand, errorNode);
    } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(node)) {
        if (implicitPlus->left->type == OperandType::UnfinishedMemoryOperand) {
            errorNode = implicitPlus;
            findInvalidExpressionCause(implicitPlus->left, errorNode);
        }
        if (implicitPlus->right->type == OperandType::UnfinishedMemoryOperand) {
            errorNode = implicitPlus;
            findInvalidExpressionCause(implicitPlus->right, errorNode);
        }
    } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        return;
    } else {
        LOG_DETAILED_ERROR("Unknown Expression Node!\n");
        return;
    }
}

DiagnosticPtr SemanticAnalyzer::reportCantAddVariables(const ExpressionPtr &node, bool implicit)
{
    std::optional<Token> firstVar;
    std::optional<Token> secondVar;
    findRelocatableVariables(node, firstVar, secondVar);
    if (!firstVar || !secondVar) {
        LOG_DETAILED_ERROR("Can't find the 2 relocatable variables!\n");
        return parseSess->dcx->getLastDiagnostic();
    }

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::CANT_ADD_VARIABLES, implicit ? "implicitly" : "");

    if (implicit) {
        auto implicitOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(node);
        // TODO: what to underline when [var][var]
        diag.addPrimaryLabel(firstVar.value().span, "first variable");
        diag.addSecondaryLabel(secondVar.value().span, "second variable");
    } else {
        auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node);
        diag.addPrimaryLabel(binaryOp->op.span, "");
        diag.addSecondaryLabel(firstVar.value().span, "first variable");
        diag.addSecondaryLabel(secondVar.value().span, "second variable");
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

void SemanticAnalyzer::findRelocatableVariables(const ExpressionPtr &node, std::optional<Token> &firstVar, std::optional<Token> &secondVar)
{
    if (node->diagnostic) {
        return;
    }
    if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        if (binaryOp->op.lexeme == ".") {
            if (binaryOp->left->isRelocatable) {
                findRelocatableVariables(binaryOp->left, firstVar, secondVar);
                return;
            }
        }
        if (binaryOp->left->isRelocatable) {
            findRelocatableVariables(binaryOp->left, firstVar, secondVar);
        }
        if (binaryOp->right->isRelocatable) {
            findRelocatableVariables(binaryOp->right, firstVar, secondVar);
        }
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        if (unaryOp->operand->isRelocatable) {
            findRelocatableVariables(unaryOp->operand, firstVar, secondVar);
        }
    } else if (auto brackets = std::dynamic_pointer_cast<Brackets>(node)) {
        if (brackets->operand->isRelocatable) {
            findRelocatableVariables(brackets->operand, firstVar, secondVar);
        }
    } else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(node)) {
        if (squareBrackets->operand->isRelocatable) {
            findRelocatableVariables(squareBrackets->operand, firstVar, secondVar);
        }
    } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(node)) {
        if (implicitPlus->left->isRelocatable) {
            findRelocatableVariables(implicitPlus->left, firstVar, secondVar);
        }
        if (implicitPlus->right->isRelocatable) {
            findRelocatableVariables(implicitPlus->right, firstVar, secondVar);
        }
    } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        if (!firstVar) {
            firstVar = leaf->token;
        } else if (!secondVar) {
            secondVar = leaf->token;
        }
    } else {
        LOG_DETAILED_ERROR("Unknown Expression Node!\n");
        return;
    }
}

DiagnosticPtr SemanticAnalyzer::reportMoreThanTwoRegistersAfterAdd(const ExpressionPtr &node, bool implicit)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::MORE_THAN_TWO_REGISTERS);

    if (implicit) {
        auto implicitOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(node);
        // TODO: what to underline when [var][var]
        bool first = true;

        for (const auto &[reg, scale] : implicitOp->left->registers) {
            if (first) {
                diag.addPrimaryLabel(reg.span, "help: register");
                first = false;
                continue;
            }
            diag.addSecondaryLabel(reg.span, "help: register");
            first = false;
        }

        for (const auto &[reg, scale] : implicitOp->right->registers) {
            if (first) {
                diag.addPrimaryLabel(reg.span, "help: register");
                first = false;
                continue;
            }
            diag.addSecondaryLabel(reg.span, "help: register");
        }
    } else {
        auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node);
        diag.addPrimaryLabel(binaryOp->op.span, ""); // write "this + resulted in having more than 2 registers?""
        for (const auto &[reg, scale] : binaryOp->left->registers) {
            diag.addSecondaryLabel(reg.span, "help: register");
        }

        for (const auto &[reg, scale] : binaryOp->right->registers) {
            diag.addSecondaryLabel(reg.span, "help: register");
        }
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportMoreThanOneScaleAfterAdd(const ExpressionPtr &node, bool implicit)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::MORE_THAN_ONE_SCALE);

    if (implicit) {
        auto implicitOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(node);
        // TODO: what to underline when [var][var]
        bool first = true;

        for (const auto &[reg, scale] : implicitOp->left->registers) {
            if (first && scale) {
                diag.addPrimaryLabel(reg.span, "help: this register has a scale");
                first = false;
                continue;
            }
            if (scale) {
                diag.addSecondaryLabel(reg.span, "help: this register has a scale");
            }
        }

        for (const auto &[reg, scale] : implicitOp->right->registers) {
            if (first && scale) {
                diag.addPrimaryLabel(reg.span, "help: this register has a scale");
                first = false;
                continue;
            }
            if (scale) {
                diag.addSecondaryLabel(reg.span, "help: this register has a scale");
            }
        }
    } else {
        auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node);
        diag.addPrimaryLabel(binaryOp->op.span, ""); // write "this + resulted in having more than 1 scale?""
        for (const auto &[reg, scale] : binaryOp->left->registers) {
            if (scale) {
                diag.addSecondaryLabel(reg.span, "help: this register has a scale");
            }
        }

        for (const auto &[reg, scale] : binaryOp->right->registers) {
            if (scale) {
                diag.addSecondaryLabel(reg.span, "help: this register has a scale");
            }
        }
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportTwoEsp(const ExpressionPtr &node, bool implicit)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::TWO_ESP_REGISTERS);

    if (implicit) {
        auto implicitOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(node);
        bool first = true;

        for (const auto &[reg, scale] : implicitOp->left->registers) {
            if (first && stringToUpper(reg.lexeme) == "ESP") {
                diag.addPrimaryLabel(reg.span, "help: this is a ESP register");
                first = false;
                continue;
            }
            if (stringToUpper(reg.lexeme) == "ESP") {
                diag.addSecondaryLabel(reg.span, "help: this is a ESP register");
            }
        }

        for (const auto &[reg, scale] : implicitOp->right->registers) {
            if (first && stringToUpper(reg.lexeme) == "ESP") {
                diag.addPrimaryLabel(reg.span, "help: this is a ESP register");
                first = false;
                continue;
            }
            if (stringToUpper(reg.lexeme) == "ESP") {
                diag.addSecondaryLabel(reg.span, "help: this is a ESP register");
            }
        }
    } else {
        auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node);
        diag.addPrimaryLabel(binaryOp->op.span, "");
        for (const auto &[reg, scale] : binaryOp->left->registers) {
            if (stringToUpper(reg.lexeme) == "ESP") {
                diag.addSecondaryLabel(reg.span, "help: this is a ESP register");
            }
        }

        for (const auto &[reg, scale] : binaryOp->right->registers) {
            if (stringToUpper(reg.lexeme) == "ESP") {
                diag.addSecondaryLabel(reg.span, "help: this is a ESP register");
            }
        }
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportNon32bitRegister(const ExpressionPtr &node, bool implicit)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::NON_32BIT_REGISTER);

    if (auto reg = std::dynamic_pointer_cast<Leaf>(node)) {
        int size = registerSizes[stringToUpper(reg->token.lexeme)];
        diag.addPrimaryLabel(reg->token.span, fmt::format("this is a {} byte register", size));
        parseSess->dcx->addDiagnostic(diag);
        return parseSess->dcx->getLastDiagnostic();
    }

    if (implicit) {
        auto implicitOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(node);
        bool first = true;

        for (const auto &[reg, scale] : implicitOp->left->registers) {
            int size = registerSizes[stringToUpper(reg.lexeme)];
            if (first && size != 4) {
                diag.addSecondaryLabel(reg.span, fmt::format("help: this is a {} byte register", size));
                first = false;
                continue;
            }
            if (size != 4) {
                diag.addSecondaryLabel(reg.span, fmt::format("help: this is a {} byte register", size));
            }
        }

        for (const auto &[reg, scale] : implicitOp->right->registers) {
            int size = registerSizes[stringToUpper(reg.lexeme)];
            if (first && size != 4) {
                diag.addSecondaryLabel(reg.span, fmt::format("help: this is a {} byte register", size));
                first = false;
                continue;
            }
            if (size != 4) {
                diag.addSecondaryLabel(reg.span, fmt::format("help: this is a {} byte register", size));
            }
        }
    } else {
        auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node);
        diag.addPrimaryLabel(binaryOp->op.span, "");
        for (const auto &[reg, scale] : binaryOp->left->registers) {
            int size = registerSizes[stringToUpper(reg.lexeme)];
            if (size != 4) {
                diag.addSecondaryLabel(reg.span, fmt::format("help: this is a {} byte register", size));
            }
        }

        for (const auto &[reg, scale] : binaryOp->right->registers) {
            int size = registerSizes[stringToUpper(reg.lexeme)];
            if (size != 4) {
                diag.addSecondaryLabel(reg.span, fmt::format("help: this is a {} byte register", size));
            }
        }
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportBinaryMinusOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::BINARY_MINUS_OPERATOR_INCORRECT_ARGUMENT);

    auto left = node->left;
    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "can only subtract constant expressions or 2 address expressions");

    diag.addSecondaryLabel(getExpressionSpan(left), fmt::format("help: this has type `{}`", getOperandType(left)));
    diag.addSecondaryLabel(getExpressionSpan(right), fmt::format("help: this has type `{}`", getOperandType(right)));

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

DiagnosticPtr SemanticAnalyzer::reportNonRegisterInSquareBrackets(const std::shared_ptr<BinaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::NON_REGISTER_IN_SQUARE_BRACKETS);
    diag.addPrimaryLabel(getExpressionSpan(node), "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

void SemanticAnalyzer::warnTypeReturnsZero(const std::shared_ptr<UnaryOperator> &node)
{
    Diagnostic diag(Diagnostic::Level::Warning, ErrorCode::TYPE_RETURNS_ZERO);
    diag.addPrimaryLabel(node->op.span, "");
    diag.addSecondaryLabel(getExpressionSpan(node->operand), "this expression doesn't have a type");
    parseSess->dcx->addDiagnostic(diag);
}