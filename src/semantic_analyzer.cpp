// File: semantic_analyzer.cpp
#include "semantic_analyzer.h"
#include "log.h"
#include <set>
#include <unordered_set>
#include <cstdint>

std::map<std::string, int> SemanticAnalyzer::registerSizes = {{"AL", 1}, {"AX", 2},  {"EAX", 4}, {"BL", 1},  {"BX", 2},  {"EBX", 4}, {"CL", 1},
                                                              {"CX", 2}, {"ECX", 4}, {"DL", 1},  {"DX", 2},  {"EDX", 4}, {"SI", 2},  {"ESI", 4},
                                                              {"DI", 2}, {"EDI", 4}, {"BP", 2},  {"EBP", 4}, {"SP", 2},  {"ESP", 4}};

std::map<int, std::string> SemanticAnalyzer::sizeValueToStr = {{1, "BYTE"}, {2, "WORD"}, {4, "DWORD"}, {8, "QWORD"}};

std::map<std::string, int> SemanticAnalyzer::sizeStrToValue = {{"BYTE", 1}, {"WORD", 2}, {"DWORD", 4}, {"QWORD", 8}};

std::unordered_set<std::string> SemanticAnalyzer::builtinTypes = {"BYTE", "WORD", "DWORD", "QWORD"};

SemanticAnalyzer::SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast) : parseSess(std::move(parseSession)), ast(std::move(ast))
{
}

void SemanticAnalyzer::analyze() { visit(ast); }

void SemanticAnalyzer::visit(const ASTPtr &node)
{
    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        for (const auto &statement : program->statements) {
            if (INVALID(statement)) {
                continue;
            }
            std::ignore = visitStatement(statement);
        }
        if (program->endDir) {
            std::ignore = visitDirective(program->endDir);
        }
    } else {
        LOG_DETAILED_ERROR("Top AST Node must be Program");
    }
}

bool SemanticAnalyzer::visitStatement(const std::shared_ptr<Statement> &statement)
{
    if (auto instruction = std::dynamic_pointer_cast<Instruction>(statement)) {
        return visitInstruction(instruction);
    } else if (auto directive = std::dynamic_pointer_cast<Directive>(statement)) {
        return visitDirective(directive);
    } else {
        LOG_DETAILED_ERROR("Unknown statement type.");
        return false;
    }
}

bool SemanticAnalyzer::visitInstruction(const std::shared_ptr<Instruction> &instruction)
{
    for (const auto &operand : instruction->operands) {
        bool success = visitExpression(operand, ExpressionContext(ExprCtxtFlags::AllowRegisters | ExprCtxtFlags::AllowForwardReferences));
        if (!success) {
            return false;
        }
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            instruction->diagnostic = reportCantHaveRegistersInExpression(operand);
            return false;
        }
        if (operand->type == OperandType::Unspecified) {
            LOG_DETAILED_ERROR("Unspecified operand type, should've been catched earlier");
            return false;
        }
    }

    if (instruction->label) {
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(instruction->label.value());
        auto labelSymbol = std::dynamic_pointer_cast<LabelSymbol>(symbol);
        if (!labelSymbol) {
            LOG_DETAILED_ERROR("no label symbol in the symbol table, when should be");
            return false;
        }
        labelSymbol->value = -1; // TODO: calculate actual value
        labelSymbol->wasDefined = true;
    }

    if (!instruction->mnemonicToken) {
        // only label exist
        return true;
    }
    // TODO: finish all possible instructions (dont forget about unresolvedSymbols)
    Token mnemonicToken = instruction->mnemonicToken.value();
    std::string mnemonic = stringToUpper(mnemonicToken.lexeme);

    // TODO: delete if some instruction accepts 8 bytes operand
    for (const auto &operand : instruction->operands) {
        if (operand->size) {
            int opSize = operand->size.value().value;
            if (opSize != 1 && opSize != 2 && opSize != 4 && !operand->unresolvedSymbols) {
                instruction->diagnostic = reportInvalidOperandSize(operand, "{1, 2, 4}", opSize);
                return false;
            }
        }
    }

    if (mnemonic == "ADC" || mnemonic == "ADD" || mnemonic == "AND" || mnemonic == "CMP" || mnemonic == "MOV") {
        if (instruction->operands.size() != 2) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, 2);
            return false;
        }
        ExpressionPtr firstOp = instruction->operands[0];
        ExpressionPtr secondOp = instruction->operands[1];
        if (firstOp->type == OperandType::MemoryOperand && secondOp->type == OperandType::MemoryOperand) {
            instruction->diagnostic = reportCantHaveTwoMemoryOperands(instruction);
            return false;
        }
        if (firstOp->type == OperandType::ImmediateOperand) {
            instruction->diagnostic = reportDestinationOperandCantBeImmediate(instruction);
            return false;
        }

        // find out actual size of constantValue
        if (secondOp->constantValue) {
            int64_t value = secondOp->constantValue.value();
            if (value >= INT8_MIN && value <= static_cast<int64_t>(UINT8_MAX)) {
                secondOp->size = OperandSize("BYTE", 1);
            } else if (value >= INT16_MIN && value <= static_cast<int64_t>(UINT16_MAX)) {
                secondOp->size = OperandSize("WORD", 2);
            } else if (value >= INT32_MIN && value <= static_cast<int64_t>(UINT32_MAX)) {
                secondOp->size = OperandSize("DWORD", 4);
            } else {
                secondOp->size = OperandSize("DWORD", 8);
            }
        }

        // !firstOp->size && !secondOp->size - shouldn't happen, because then it's two memory operands
        if (firstOp->size && secondOp->size) {
            int firstOpSize = firstOp->size.value().value;
            int secondOpSize = secondOp->size.value().value;
            if (secondOp->constantValue && firstOpSize < secondOpSize && !(firstOp->unresolvedSymbols || secondOp->unresolvedSymbols)) {
                instruction->diagnostic = reportImmediateOperandTooBig(instruction, firstOpSize, secondOpSize);
                return false;
            }
            if (!secondOp->constantValue && firstOpSize != secondOpSize && !(firstOp->unresolvedSymbols || secondOp->unresolvedSymbols)) {
                instruction->diagnostic = reportOperandsHaveDifferentSize(instruction, firstOpSize, secondOpSize);
                return false;
            }
        }
    } else if (mnemonic == "CALL") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, 1);
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->type != OperandType::MemoryOperand && operand->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportExpressionMustBeMemoryOrRegisterOperand(operand);
            return false;
        }
        if (!operand->size) {
            instruction->diagnostic = reportExpressionMustHaveSize(operand);
            return false;
        }
        int operandSize = operand->size.value().value;
        if (operandSize != 4 && !operand->unresolvedSymbols) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize);
            return false;
        }
    } else if (mnemonic == "CBW" || mnemonic == "CDQ" || mnemonic == "CWD") {
        if (instruction->operands.size() != 0) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, 0);
            return false;
        }
    } else if (mnemonic == "DEC" || mnemonic == "DIV" || mnemonic == "IDIV") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, 1);
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->type != OperandType::MemoryOperand && operand->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportExpressionMustBeMemoryOrRegisterOperand(operand);
            return false;
        }
        if (!operand->size) {
            instruction->diagnostic = reportExpressionMustHaveSize(operand);
            return false;
        }
    } else if (mnemonic == "INCHAR") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, 1);
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->type != OperandType::MemoryOperand && operand->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportExpressionMustBeMemoryOrRegisterOperand(operand);
            return false;
        }
        if (!operand->size) {
            instruction->diagnostic = reportExpressionMustHaveSize(operand);
            return false;
        }
        int operandSize = operand->size.value().value;
        if (operandSize != 1 && !operand->unresolvedSymbols) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "1", operandSize);
            return false;
        }
    }
    return true;
}

bool SemanticAnalyzer::visitDirective(const std::shared_ptr<Directive> &directive)
{
    if (auto segDir = std::dynamic_pointer_cast<SegDir>(directive)) {
        return visitSegDir(segDir);
    } else if (auto dataDir = std::dynamic_pointer_cast<DataDir>(directive)) {
        return visitDataDir(dataDir);
    } else if (auto structDir = std::dynamic_pointer_cast<StructDir>(directive)) {
        return visitStructDir(structDir);
    } else if (auto procDir = std::dynamic_pointer_cast<ProcDir>(directive)) {
        return visitProcDir(procDir);
    } else if (auto recordDir = std::dynamic_pointer_cast<RecordDir>(directive)) {
        return visitRecordDir(recordDir);
    } else if (auto equDir = std::dynamic_pointer_cast<EquDir>(directive)) {
        return visitEquDir(equDir);
    } else if (auto equalDir = std::dynamic_pointer_cast<EqualDir>(directive)) {
        return visitEqualDir(equalDir);
    } else if (auto endDir = std::dynamic_pointer_cast<EndDir>(directive)) {
        return visitEndDir(endDir);
    } else {
        LOG_DETAILED_ERROR("Unknown directive type.");
        return false;
    }
}

bool SemanticAnalyzer::visitSegDir(const std::shared_ptr<SegDir> &segDir)
{
    if (segDir->constExpr) {
        bool success = visitExpression(segDir->constExpr.value(), ExpressionContext(ExprCtxtFlags::None));
        // TODO: checl that expr is constant
        if (!success) {
            return false;
        }
    }
    return true;
}

bool SemanticAnalyzer::visitDataDir(const std::shared_ptr<DataDir> &dataDir, const std::optional<Token> &strucNameToken)
{
    if (dataDir->idToken) {
        std::string fieldName = dataDir->idToken.value().lexeme;
        std::shared_ptr<DataVariableSymbol> dataVariableSymbol;
        if (strucNameToken) {
            std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(strucNameToken.value());
            auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol);
            dataVariableSymbol = structSymbol->fields[fieldName];
        } else {
            std::shared_ptr<Symbol> fieldSymbol = parseSess->symbolTable->findSymbol(fieldName);
            dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(fieldSymbol);
        }
        if (!dataVariableSymbol) {
            LOG_DETAILED_ERROR("no data variable symbol in the symbol table, when should be");
            return false;
        }
        dataVariableSymbol->value = -1; // TODO: calculate actual value
        dataVariableSymbol->wasDefined = true;
    }
    return visitDataItem(dataDir->dataItem);
}

bool SemanticAnalyzer::visitStructDir(const std::shared_ptr<StructDir> &structDir)
{
    for (const auto &field : structDir->fields) {
        if (INVALID(field)) {
            continue;
        }
        std::ignore = visitDataDir(field, structDir->firstIdToken);
    }
    std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(structDir->firstIdToken);
    auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol);
    structSymbol->size = structSymbol->sizeOf = -1; // TODO: calculate actual value
    structSymbol->wasDefined = true;
    return true;
}

bool SemanticAnalyzer::visitProcDir(const std::shared_ptr<ProcDir> &procDir)
{
    for (const auto &instruction : procDir->instructions) {
        std::ignore = visitInstruction(instruction);
    }

    std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(procDir->firstIdToken);
    auto procSymbol = std::dynamic_pointer_cast<ProcSymbol>(symbol);
    procSymbol->value = -1; // TODO: calculate actual value
    procSymbol->wasDefined = true;
    return true;
}

bool SemanticAnalyzer::visitRecordDir(const std::shared_ptr<RecordDir> &recordDir)
{
    int32_t width = 0;
    for (const auto &field : recordDir->fields) {
        bool success = visitRecordField(field);
        if (!success) {
            return false;
        }
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(field->fieldToken);
        auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol);
        width += recordFieldSymbol->width;
    }

    if (width > 32) {
        recordDir->diagnostic = reportRecordWidthTooBig(recordDir, width);
        return false;
    }

    std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(recordDir->idToken);
    auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol);
    recordSymbol->width = width;
    recordSymbol->wasDefined = true;

    return true;
}

bool SemanticAnalyzer::visitRecordField(const std::shared_ptr<RecordField> &recordField)
{
    std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(recordField->fieldToken);
    auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol);

    bool success = visitExpression(recordField->width, ExpressionContext(ExprCtxtFlags::None));
    if (!success) {
        return false;
    }

    if (!recordField->width->constantValue) {
        recordField->diagnostic = reportExpressionMustBeConstant(recordField->width);
        return false;
    }

    int64_t width = recordField->width->constantValue.value();
    if (width <= 0) {
        recordField->diagnostic = reportRecordFieldWidthMustBePositive(recordField, width);
        return false;
    }
    if (width > 31) {
        recordField->diagnostic = reportRecordFieldWidthTooBig(recordField, width);
        return false;
    }
    recordFieldSymbol->width = static_cast<int32_t>(width);

    auto initialValueExpr = recordField->initialValue;
    if (initialValueExpr) {
        success = visitExpression(initialValueExpr.value(), ExpressionContext(ExprCtxtFlags::None));
        if (!success) {
            return false;
        }

        if (!initialValueExpr.value()->constantValue) {
            recordField->diagnostic = reportExpressionMustBeConstant(initialValueExpr.value());
            return false;
        }
        // int64_t initialValue = initialValueExpr.value()->constantValue.value();
        // TODO: check that initialValue fits into `width` bytes
    }
    recordFieldSymbol->wasDefined = true;
    return true;
}

bool SemanticAnalyzer::visitEquDir(const std::shared_ptr<EquDir> &equDir)
{
    // TODO: can be a string
    std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(equDir->idToken);
    auto equSymbol = std::dynamic_pointer_cast<EquVariableSymbol>(symbol);
    bool success = visitExpression(equDir->value, ExpressionContext(ExprCtxtFlags::None));
    if (!success) {
        return false;
    }
    if (!equDir->value->constantValue) {
        equDir->diagnostic = reportExpressionMustBeConstant(equDir->value);
        return false;
    }
    // TODO: report when is' bigger than 32 bits
    equSymbol->value = static_cast<int32_t>(equDir->value->constantValue.value());
    equSymbol->wasDefined = true;
    return true;
}

bool SemanticAnalyzer::visitEqualDir(const std::shared_ptr<EqualDir> &equalDir)
{
    std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(equalDir->idToken);
    auto equalSymbol = std::dynamic_pointer_cast<EqualVariableSymbol>(symbol);
    bool success = visitExpression(equalDir->value, ExpressionContext(ExprCtxtFlags::None));
    if (!success) {
        return false;
    }
    if (!equalDir->value->constantValue) {
        equalDir->diagnostic = reportExpressionMustBeConstant(equalDir->value);
        return false;
    }
    // TODO: report when is' bigger than 32 bits
    equalSymbol->value = static_cast<int32_t>(equalDir->value->constantValue.value());
    equalSymbol->wasDefined = true;
    return true;
}

bool SemanticAnalyzer::visitEndDir(const std::shared_ptr<EndDir> &endDir)
{
    if (endDir->addressExpr) {
        bool success = visitExpression(endDir->addressExpr.value(), ExpressionContext(ExprCtxtFlags::None));
        if (!success) {
            return false;
        }
        // TODO: check it's address expression
    }
    return true;
}

bool SemanticAnalyzer::visitDataItem(const std::shared_ptr<DataItem> &dataItem)
{
    if (auto builtinInstance = std::dynamic_pointer_cast<BuiltinInstance>(dataItem)) {
        Token dataTypeToken = builtinInstance->dataTypeToken;
        std::string dataType = dataTypeToken.lexeme;

        // Visit initialization values
        return visitInitValue(builtinInstance->initValues, dataType);

    } else if (auto recordInstance = std::dynamic_pointer_cast<RecordInstance>(dataItem)) {
        // TODO: Implement record instance processing
        LOG_DETAILED_ERROR("Record instances are not yet implemented.");
    } else if (auto structInstance = std::dynamic_pointer_cast<StructInstance>(dataItem)) {
        // TODO: Implement struct instance processing
        LOG_DETAILED_ERROR("Struct instances are not yet implemented.");
    } else {
        LOG_DETAILED_ERROR("Unknown data item type.");
    }
    return true;
}

bool SemanticAnalyzer::visitInitValue(const std::shared_ptr<InitValue> &initValue, const std::string &dataType)
{
    if (auto dupOperator = std::dynamic_pointer_cast<DupOperator>(initValue)) {
        // Process DUP operator
        bool success = visitExpression(dupOperator->repeatCount, ExpressionContext(ExprCtxtFlags::None));
        if (!success) {
            return false;
        }
        // TODO check repeatCount is constant

        // Visit operands
        return visitInitValue(dupOperator->operands, dataType);

    } else if (auto questionMarkInitValue = std::dynamic_pointer_cast<QuestionMarkInitValue>(initValue)) {
        // Uninitialized data; no action needed

    } else if (auto expressionInitValue = std::dynamic_pointer_cast<ExpressionInitValue>(initValue)) {
        // Process expression init value
        return visitExpression(expressionInitValue->expr, ExpressionContext(ExprCtxtFlags::None | ExprCtxtFlags::AllowForwardReferences));

        // TODO: Implement type checking between expression and data type

    } else if (auto structOrRecordInitValue = std::dynamic_pointer_cast<StructOrRecordInitValue>(initValue)) {
        // Process struct or record initialization
        return visitInitValue(structOrRecordInitValue->fields, dataType);

    } else if (auto initList = std::dynamic_pointer_cast<InitializerList>(initValue)) {
        for (const auto &init : initList->fields) {
            bool success = visitInitValue(init, dataType);
            if (!success) {
                return false;
            }
        }
    } else {
        LOG_DETAILED_ERROR("Unknown initialization value type.");
    }
    return true;
}

bool SemanticAnalyzer::visitExpression(const ExpressionPtr &node, const ExpressionContext &context)
{
    expressionDepth = 0;
    return visitExpressionHelper(node, context);
}

bool SemanticAnalyzer::visitExpressionHelper(const ExpressionPtr &node, const ExpressionContext &context)
{
    bool result = false;
    expressionDepth++;
    if (auto brackets = std::dynamic_pointer_cast<Brackets>(node)) {
        result = visitBrackets(brackets, context);
    } else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(node)) {
        result = visitSquareBrackets(squareBrackets, context);
    } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(node)) {
        result = visitImplicitPlusOperator(implicitPlus, context);
    } else if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        result = visitBinaryOperator(binaryOp, context);
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        result = visitUnaryOperator(unaryOp, context);
    } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        result = visitLeaf(leaf, context);
    } else {
        LOG_DETAILED_ERROR("Unknown expression ptr node");
    }
    expressionDepth--;
    return result;
}

bool SemanticAnalyzer::visitBrackets(const std::shared_ptr<Brackets> &node, const ExpressionContext &context)
{
    bool success = visitExpressionHelper(node->operand, context);
    if (!success) {
        return false;
    }

    auto operand = node->operand;
    node->unresolvedSymbols = operand->unresolvedSymbols;

    node->constantValue = operand->constantValue;
    node->isRelocatable = operand->isRelocatable;
    node->type = operand->type;
    node->size = operand->size;
    node->registers = operand->registers;
    return true;
}

bool SemanticAnalyzer::visitSquareBrackets(const std::shared_ptr<SquareBrackets> &node, const ExpressionContext &context)
{
    bool success = visitExpressionHelper(node->operand, context);
    if (!success) {
        return false;
    }

    auto operand = node->operand;
    node->unresolvedSymbols = operand->unresolvedSymbols;

    node->constantValue = operand->constantValue;
    node->isRelocatable = operand->isRelocatable;
    if (operand->type == OperandType::UnfinishedMemoryOperand) {
        // check we only have 32 bit regsiters, and dont have 2 esp
        // delay checking for this, because for (esp + esp) we want to report can have registers in expressions
        // dont want to call this (reportInvalidAddressExpression())
        bool implicit = false;
        ExpressionPtr expr;
        std::shared_ptr<BinaryOperator> binOp;
        std::shared_ptr<ImplicitPlusOperator> implicitPlusOp;
        if ((binOp = std::dynamic_pointer_cast<BinaryOperator>(operand))) {
            implicit = false;
            // Check that we have [eax * 5] in []
            if (binOp->op.lexeme == "+") {
                bool firstIsRegisterWithOptionalScale = binOp->left->type == OperandType::RegisterOperand;
                if (auto binOpLeft = std::dynamic_pointer_cast<BinaryOperator>(binOp->left)) {
                    if (binOpLeft->op.lexeme == "*" &&
                        (binOpLeft->left->type == OperandType::RegisterOperand || binOpLeft->right->type == OperandType::RegisterOperand)) {
                        firstIsRegisterWithOptionalScale = true;
                    }
                }
                bool firstIsConstant = bool(binOp->left->constantValue);
                bool secondIsRegisterWithOptionalScale = binOp->right->type == OperandType::RegisterOperand;
                if (auto binOpRight = std::dynamic_pointer_cast<BinaryOperator>(binOp->right)) {
                    if (binOpRight->op.lexeme == "*" &&
                        (binOpRight->left->type == OperandType::RegisterOperand || binOpRight->right->type == OperandType::RegisterOperand)) {
                        secondIsRegisterWithOptionalScale = true;
                    }
                }
                bool secondIsConstant = bool(binOp->right->constantValue);
                if ((firstIsConstant && secondIsRegisterWithOptionalScale) || (firstIsRegisterWithOptionalScale && secondIsConstant)) {
                    // allowed
                } else {
                    node->diagnostic = reportNonRegisterInSquareBrackets(binOp);
                    return false;
                }
            } else if (binOp->op.lexeme != "*") {
                node->diagnostic = reportNonRegisterInSquareBrackets(binOp);
                return false;
            }
            expr = binOp;
        } else if ((implicitPlusOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(operand))) {
            // report error, beacuse can't have [[eax][ebx]]
            node->diagnostic = reportNonRegisterInSquareBrackets(binOp);
            return false;
        } else {
            LOG_DETAILED_ERROR("Unexpected operand type!\n");
        }
        bool non32bitRegister = false;
        int espCount = 0;
        for (const auto &[regToken, scale] : operand->registers) {
            if (registerSizes[stringToUpper(regToken.lexeme)] != 4) {
                non32bitRegister = true;
            }
            if (stringToUpper(regToken.lexeme) == "ESP") {
                espCount += 1;
            }
        }

        if (non32bitRegister) {
            node->diagnostic = reportNon32bitRegister(expr, implicit);
            return false;
        }
        if (espCount == 2) {
            node->diagnostic = reportTwoEsp(expr, implicit);
            return false;
        }
        node->type = OperandType::MemoryOperand;
    } else if (operand->type == OperandType::RegisterOperand) {
        bool non32bitRegister = false;
        for (const auto &[regToken, scale] : operand->registers) {
            if (registerSizes[stringToUpper(regToken.lexeme)] != 4) {
                non32bitRegister = true;
            }
        }
        if (non32bitRegister) {
            node->diagnostic = reportNon32bitRegister(operand, true);
            return false;
        }
        node->type = OperandType::MemoryOperand;
    } else {
        node->type = operand->type;
    }
    if (operand->registers.empty()) {
        node->size = operand->size;
    } else {
        // modificators reset known size
        node->size = std::nullopt;
    }
    node->registers = operand->registers;
    return true;
}

bool SemanticAnalyzer::visitImplicitPlusOperator(const std::shared_ptr<ImplicitPlusOperator> &node, const ExpressionContext &context)
{
    bool successLeft = visitExpressionHelper(node->left, context);
    bool successRight = visitExpressionHelper(node->right, context);
    if (!successLeft || !successRight) {
        return false;
    }

    auto left = node->left;
    auto right = node->right;
    node->unresolvedSymbols = left->unresolvedSymbols || right->unresolvedSymbols;

    if (left->isRelocatable && right->isRelocatable) {
        node->diagnostic = reportCantAddVariables(node, true);
        return false;
    }
    // check that we have not more then 2 registers, and only one has scale (eax * 1) is counted as having scale
    if (left->registers.size() + right->registers.size() > 2) {
        node->diagnostic = reportMoreThanTwoRegistersAfterAdd(node, true);
        return false;
    }

    auto newRegisters = left->registers;
    for (const auto &[regToken, scale] : right->registers) {
        newRegisters[regToken] = scale;
    }
    // ensure only one has scale
    int scaleCount = 0;
    for (const auto &[regToken, scale] : newRegisters) {
        if (newRegisters[regToken].has_value()) {
            scaleCount += 1;
        }
    }
    if (scaleCount > 1) {
        node->diagnostic = reportMoreThanOneScaleAfterAdd(node, true);
        return false;
    }

    // check we only have 32 bit regsiters, and dont have 2 esp
    // in implicit plus - need to check immediately
    // to detect [esp][esp]
    // for esp + esp - want to have error - can't have regsiters in expressions
    bool non32bitRegister = false;
    int espCount = 0;
    for (const auto &[regToken, scale] : newRegisters) {
        if (registerSizes[stringToUpper(regToken.lexeme)] != 4) {
            non32bitRegister = true;
        }
        if (stringToUpper(regToken.lexeme) == "ESP") {
            espCount += 1;
        }
    }

    if (non32bitRegister) {
        node->diagnostic = reportNon32bitRegister(node, true);
        return false;
    }
    if (espCount == 2) {
        node->diagnostic = reportTwoEsp(node, true);
        return false;
    }

    if (left->constantValue && right->constantValue) {
        node->constantValue = left->constantValue.value() + right->constantValue.value();
    } else {
        node->constantValue = std::nullopt;
    }
    node->isRelocatable = left->isRelocatable || right->isRelocatable;

    if (left->type == OperandType::ImmediateOperand && right->type == OperandType::ImmediateOperand) {
        node->type = OperandType::ImmediateOperand;
    } else if (left->type == OperandType::RegisterOperand || right->type == OperandType::RegisterOperand) {
        node->type = OperandType::UnfinishedMemoryOperand;
    } else if (left->type == OperandType::UnfinishedMemoryOperand || right->type == OperandType::UnfinishedMemoryOperand) {
        node->type = OperandType::UnfinishedMemoryOperand;
    } else if (left->type == OperandType::MemoryOperand || right->type == OperandType::MemoryOperand) {
        node->type = OperandType::MemoryOperand;
    } else {
        LOG_DETAILED_ERROR("Unhandled operand type combination in ImplicitPlus");
    }

    if (!left->size && !right->size) {
        node->size = std::nullopt;
    } else if (left->size && !right->size) {
        node->size = left->size;
    } else if (!left->size && right->size) {
        node->size = right->size;
    } else {
        // TODO: report that operands have different sizes
        // for example (byte PTR testvar)(dword PTR [eax])
        node->size = left->size;
    }
    node->registers = newRegisters;
    return true;
}

bool SemanticAnalyzer::visitBinaryOperator(const std::shared_ptr<BinaryOperator> &node, const ExpressionContext &context)
{
    ExpressionContext contextLeft = context;
    ExpressionContext contextRight = context;
    if (stringToUpper(node->op.lexeme) == "PTR") {
        contextLeft.isPTROperand = true;
    }
    if (node->op.lexeme == ".") {
        contextRight.isStructField = true;
    }

    bool successLeft = visitExpressionHelper(node->left, contextLeft);
    bool successRight = visitExpressionHelper(node->right, contextRight);
    if (!successLeft || !successRight) {
        return false;
    }

    std::string op = stringToUpper(node->op.lexeme);
    auto left = node->left;
    auto right = node->right;
    node->unresolvedSymbols = left->unresolvedSymbols || right->unresolvedSymbols;

    if (op == ".") {
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            node->diagnostic = reportCantHaveRegistersInExpression(left);
            return false;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            node->diagnostic = reportCantHaveRegistersInExpression(right);
            return false;
        }
        if (left->type != OperandType::MemoryOperand) {
            node->diagnostic = reportDotOperatorIncorrectArgument(node);
            return false;
        }
        std::shared_ptr<Leaf> leafRight;
        if (!(leafRight = std::dynamic_pointer_cast<Leaf>(right)) || leafRight->token.type != TokenType::Identifier) {
            LOG_DETAILED_ERROR("After `.` encountered not identifier! (should be handled in parsing stage)");
            return false;
        }

        if (!left->size) {
            node->diagnostic = reportDotOperatorSizeNotSpecified(node);
            return false;
        }
        std::shared_ptr<Symbol> typeSymbol = parseSess->symbolTable->findSymbol(left->size->symbol);
        if (!typeSymbol || !std::dynamic_pointer_cast<StructSymbol>(typeSymbol)) {
            node->diagnostic = reportDotOperatorTypeNotStruct(node, left->size->symbol);
            return false;
        }
        auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(typeSymbol);
        if (!structSymbol->fields.contains(leafRight->token.lexeme)) {
            node->diagnostic = reportDotOperatorFieldDoesntExist(node, structSymbol->token.lexeme, leafRight->token.lexeme);
            return false;
        }

        node->constantValue = std::nullopt;
        node->isRelocatable = left->isRelocatable;
        node->type = OperandType::MemoryOperand;
        node->size = left->size;
        node->registers = left->registers;
        return true;
    } else if (op == "PTR") {
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            node->diagnostic = reportCantHaveRegistersInExpression(left);
            return false;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            node->diagnostic = reportCantHaveRegistersInExpression(right);
            return false;
        }
        std::shared_ptr<Leaf> leafLeft;
        if (!(leafLeft = std::dynamic_pointer_cast<Leaf>(left)) ||
            !(leafLeft->token.type == TokenType::Type || leafLeft->token.type == TokenType::Identifier) ||
            right->type == OperandType::RegisterOperand) {
            node->diagnostic = reportPtrOperatorIncorrectArgument(node);
            return false;
        }

        std::string typeOperand = leafLeft->token.lexeme;
        std::shared_ptr<Symbol> typeSymbol;
        if (!builtinTypes.contains(stringToUpper(typeOperand))) {
            typeSymbol = parseSess->symbolTable->findSymbol(leafLeft->token);
            if (!typeSymbol || !std::dynamic_pointer_cast<StructSymbol>(typeSymbol)) {
                node->diagnostic = reportPtrOperatorIncorrectArgument(node);
                return false;
            }
        }
        if (builtinTypes.contains(stringToUpper(typeOperand))) {
            typeOperand = stringToUpper(typeOperand);
        }

        node->constantValue = right->constantValue;
        node->isRelocatable = right->isRelocatable;
        node->type = right->type;
        if (builtinTypes.contains(typeOperand)) {
            node->size = OperandSize(typeOperand, sizeStrToValue[typeOperand]);
        } else {
            auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(typeSymbol);
            node->size = OperandSize(typeOperand, structSymbol->size);
        }

        node->registers = right->registers;
        return true;

    } else if (op == "*" || op == "/" || op == "MOD" || op == "SHL" || op == "SHR") {
        // can check this immediately cause eax * 4, eax is RegisterOperand
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            node->diagnostic = reportCantHaveRegistersInExpression(left);
            return false;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            node->diagnostic = reportCantHaveRegistersInExpression(right);
            return false;
        }
        // handle eax * 4
        if (op == "*") {
            if ((left->constantValue && right->type == OperandType::RegisterOperand) ||
                (right->constantValue && left->type == OperandType::RegisterOperand)) {
                int64_t value = 0;
                std::shared_ptr<Leaf> leafNode;
                if (left->constantValue) {
                    value = left->constantValue.value();
                    leafNode = std::dynamic_pointer_cast<Leaf>(right);
                } else {
                    value = right->constantValue.value();
                    leafNode = std::dynamic_pointer_cast<Leaf>(left);
                }
                if (value != 1 && value != 2 && value != 4 && value != 8 && !node->unresolvedSymbols) {
                    node->diagnostic = reportInvalidScaleValue(node);
                    return false;
                }
                if (stringToUpper(leafNode->token.lexeme) == "ESP") {
                    node->diagnostic = reportIncorrectIndexRegister(leafNode);
                    return false;
                }

                node->constantValue = std::nullopt;
                node->isRelocatable = false;
                node->type = OperandType::UnfinishedMemoryOperand;
                node->size = std::nullopt;
                if (left->type == OperandType::RegisterOperand) {
                    std::shared_ptr<Leaf> leftLeaf = std::dynamic_pointer_cast<Leaf>(left);
                    node->registers[leftLeaf->token] = static_cast<int32_t>(value);
                } else {
                    std::shared_ptr<Leaf> rightLeaf = std::dynamic_pointer_cast<Leaf>(right);
                    node->registers[rightLeaf->token] = static_cast<int32_t>(value);
                }
                return true;
            }
        }
        if (left->constantValue && right->constantValue) {
            if (op == "*") {
                node->constantValue = left->constantValue.value() * right->constantValue.value();
            } else if (op == "/") {
                if (right->constantValue.value() == 0 && !node->unresolvedSymbols) {
                    node->diagnostic = reportDivisionByZero(node);
                    return false;
                }
                if (node->unresolvedSymbols) {
                    node->constantValue = -1;
                } else {
                    node->constantValue = left->constantValue.value() / right->constantValue.value();
                }
            } else if (op == "MOD") {
                if (right->constantValue.value() == 0 && !node->unresolvedSymbols) {
                    node->diagnostic = reportDivisionByZero(node);
                    return false;
                }
                if (node->unresolvedSymbols) {
                    node->constantValue = -1;
                } else {
                    node->constantValue = left->constantValue.value() % right->constantValue.value();
                }
            } else if (op == "SHL") {
                // TODO: calculate SHL
                node->unresolvedSymbols = true;
                node->constantValue = -1;
            } else if (op == "SHR") {
                // TODO: calculate SHR
                node->unresolvedSymbols = true;
                node->constantValue = -1;
            }

            node->constantValue = right->constantValue;
            node->isRelocatable = right->isRelocatable;
            node->type = right->type;
            node->size = OperandSize("DWORD", 4);
            node->registers = left->registers;
            return true;
        }

        node->diagnostic = reportOtherBinaryOperatorIncorrectArgument(node);
        return false;
    } else if (op == "+") {
        if (left->isRelocatable && right->isRelocatable) {
            node->diagnostic = reportCantAddVariables(node, false);
            return false;
        }
        // check that we have not more then 2 registers, and only one has scale (eax * 1) is counted as having scale
        // Not needed now, because [eax + ebx] is forbidden (only one register inside [])
        // later the error CANT_HAVE_REGISTERS_IN_EXPRESSION will be emitted
        {
            // if (left->registers.size() + right->registers.size() > 2) {
            //     node->type = OperandType::InvalidOperand;
            //     reportMoreThanTwoRegistersAfterAdd(node, false);
            //     return;
            // }

            // auto newRegisters = left->registers;
            // for (const auto &[regToken, scale] : right->registers) {
            //     newRegisters[regToken] = scale;
            // }
            // // ensure only one has scale
            // int scaleCount = 0;
            // for (const auto &[regToken, scale] : newRegisters) {
            //     if (newRegisters[regToken].has_value()) {
            //         scaleCount += 1;
            //     }
            // }
            // if (scaleCount > 1) {
            //     node->type = OperandType::InvalidOperand;
            //     reportMoreThanOneScaleAfterAdd(node, false);
            //     return;
            // }
        }
        // check we only have 32 bit regsiters, and dont have 2 esp
        // checking for this is delayed until operator []
        if (left->constantValue && right->constantValue) {
            node->constantValue = left->constantValue.value() + right->constantValue.value();
        } else {
            node->constantValue = std::nullopt;
        }
        node->isRelocatable = left->isRelocatable || right->isRelocatable;

        if (left->type == OperandType::ImmediateOperand && right->type == OperandType::ImmediateOperand) {
            node->type = OperandType::ImmediateOperand;
        } else if (left->type == OperandType::RegisterOperand || right->type == OperandType::RegisterOperand) {
            node->type = OperandType::UnfinishedMemoryOperand;
        } else if (left->type == OperandType::UnfinishedMemoryOperand || right->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::UnfinishedMemoryOperand;
        } else if (left->type == OperandType::MemoryOperand || right->type == OperandType::MemoryOperand) {
            node->type = OperandType::MemoryOperand;
        } else {
            LOG_DETAILED_ERROR("Unhandled operand type combination in `+`");
        }

        if (!left->size && !right->size) {
            node->size = std::nullopt;
        } else if (left->size && !right->size) {
            node->size = left->size;
        } else if (!left->size && right->size) {
            node->size = right->size;
        } else {
            // TODO: report that operands have different sizes
            // for example (byte PTR testvar)(dword PTR [eax])
            node->size = left->size;
        }

        auto newRegisters = left->registers;
        for (const auto &[regToken, scale] : right->registers) {
            newRegisters[regToken] = scale;
        }
        node->registers = newRegisters;
        return true;
    } else if (op == "-") {
        if (!(left->registers.empty() && left->isRelocatable) && !right->constantValue) {
            // need to check this after, cause after `-` can still be UnsinishedMemoryOperand
            if (left->type == OperandType::UnfinishedMemoryOperand) {
                node->diagnostic = reportCantHaveRegistersInExpression(left);
                return false;
            } else if (right->type == OperandType::UnfinishedMemoryOperand) {
                node->diagnostic = reportCantHaveRegistersInExpression(right);
                return false;
            }
            node->diagnostic = reportBinaryMinusOperatorIncorrectArgument(node);
            return false;
        }

        if (left->registers.empty() && left->isRelocatable) {
            // left is an adress expression, right can be an address expression or constant
            if (right->constantValue) {
                node->constantValue = left->constantValue.value() - right->constantValue.value();
                node->isRelocatable = false;
                node->type = OperandType::ImmediateOperand;
                node->size = std::nullopt;
                node->registers = {};
                return true;
            } else if (right->isRelocatable && right->registers.empty()) {
                node->unresolvedSymbols = true;
                node->constantValue = -1; // TODO: calculate actual constant value of the differece between 2 relocations
                node->isRelocatable = false;
                node->type = OperandType::ImmediateOperand;
                node->size = std::nullopt;
                node->registers = {};
                return true;
            }
        }
        // left is not an adress expression, right must be constant
        else if (right->constantValue) {
            if (left->constantValue) {
                node->constantValue = left->constantValue.value() - right->constantValue.value();
            } else {
                node->constantValue = left->constantValue;
            }
            node->isRelocatable = left->isRelocatable;
            if (left->type == OperandType::RegisterOperand) {
                node->type = OperandType::UnfinishedMemoryOperand;
            } else {
                node->type = left->type;
            }
            node->size = left->size;
            node->registers = left->registers;
            return true;
        }
    } else {
        LOG_DETAILED_ERROR("Unknown binary operator!");
    }
    return true;
}

bool SemanticAnalyzer::visitUnaryOperator(const std::shared_ptr<UnaryOperator> &node, const ExpressionContext &context)
{
    ExpressionContext contextOperand = context;
    std::string op = stringToUpper(node->op.lexeme);
    if (op == "WIDTH" || op == "MASK") {
        contextOperand.isWidthOrMaskOperand = true;
    }
    if (op == "SIZE" || op == "SIZEOF") {
        contextOperand.isSizeOperand = true;
    }
    bool success = visitExpressionHelper(node->operand, contextOperand);
    if (!success) {
        return false;
    }

    auto operand = node->operand;
    node->unresolvedSymbols = operand->unresolvedSymbols;

    if (operand->type == OperandType::UnfinishedMemoryOperand) {
        node->diagnostic = reportCantHaveRegistersInExpression(operand);
        return false;
    }

    if (op == "LENGTH" || op == "LENGTHOF") {
        std::shared_ptr<Leaf> leaf;
        // operand must be identifier
        if (!(leaf = std::dynamic_pointer_cast<Leaf>(operand)) || leaf->token.type != TokenType::Identifier) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        // LENGTH(OF) doesn't work with StructSymbol, RecordSymbol, RecordFieldSymbol
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(leaf->token);
        if (std::dynamic_pointer_cast<StructSymbol>(symbol) || std::dynamic_pointer_cast<RecordSymbol>(symbol) ||
            std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        // TODO: calculate actual length
        node->unresolvedSymbols = true;
        node->constantValue = -1;
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = std::nullopt;
        node->registers = {};
    } else if (op == "SIZE" || op == "SIZEOF") {
        std::shared_ptr<Leaf> leaf;
        // operand must be identifier
        if (!(leaf = std::dynamic_pointer_cast<Leaf>(operand)) || leaf->token.type != TokenType::Identifier) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        // SIZE(OF) doesn't work with RecordFieldSymbol
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(leaf->token);
        if (std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        // TODO: calculate actual size
        node->unresolvedSymbols = true;
        node->constantValue = -1;
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = std::nullopt;
        node->registers = {};

    } else if (op == "WIDTH" || op == "MASK") {
        std::shared_ptr<Leaf> leaf;
        // operand must be identifier
        if (!(leaf = std::dynamic_pointer_cast<Leaf>(operand)) || leaf->token.type != TokenType::Identifier) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        // WIDTH and MASK work only with RecordSymbol or RecordFieldSymbol
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(leaf->token);
        if (!std::dynamic_pointer_cast<RecordSymbol>(symbol) && !std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        // TODO: calculate actual value
        node->unresolvedSymbols = true;
        node->constantValue = -1;
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = std::nullopt;
        node->registers = {};

    } else if (op == "OFFSET") {
        // operand must be adress expression (without registers)
        if (operand->constantValue || !operand->registers.empty()) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        node->constantValue = operand->constantValue;
        node->isRelocatable = operand->isRelocatable;
        node->type = OperandType::ImmediateOperand;
        node->size = operand->size;
        node->registers = operand->registers;

    } else if (op == "TYPE") {
        // work with everythings, but outputs 0 sometimes
        if (!operand->size) {
            node->constantValue = 0;
            warnTypeReturnsZero(node);
        } else {
            node->constantValue = operand->size.value().value;
        }
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = std::nullopt;
        node->registers = {};
        return true;
    } else if (op == "+" || op == "-") {
        // operand must be constant value
        if (!operand->constantValue) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        if (op == "-") {
            node->constantValue = -operand->constantValue.value();
        } else if (op == "+") {
            node->constantValue = operand->constantValue;
        }
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = std::nullopt;
        node->registers = {};
    } else {
        LOG_DETAILED_ERROR("Unknown unary operator!");
        return false;
    }
    return true;
}

bool SemanticAnalyzer::visitLeaf(const std::shared_ptr<Leaf> &node, const ExpressionContext &context)
{
    Token token = node->token;

    if (token.type == TokenType::Identifier) {
        if (context.isStructField) {
            // CRITICAL TODO: check whether this symbol is actually defined
            node->type = OperandType::Unspecified;
            node->unresolvedSymbols = true;
            return true;
        }

        if (!parseSess->symbolTable->findSymbol(token)) {
            node->diagnostic = reportUndefinedSymbol(token, false);
            return false;
        }
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(token);
        if (!symbol->wasDefined && !context.allowForwardReferences) {
            node->diagnostic = reportUndefinedSymbol(token, true);
            return false;
        }

        if (!symbol->wasDefined) {
            linesForSecondPass.push_back(currentLine);
            node->unresolvedSymbols = true;
        }

        if (auto dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(symbol)) {
            std::string dataType = dataVariableSymbol->dataType.lexeme;
            node->constantValue = std::nullopt;
            node->isRelocatable = true;
            node->type = OperandType::MemoryOperand;
            if (stringToUpper(dataType) == "DB") {
                node->size = OperandSize("BYTE", 1);
            } else if (stringToUpper(dataType) == "DW") {
                node->size = OperandSize("WORD", 2);
            } else if (stringToUpper(dataType) == "DD") {
                node->size = OperandSize("DWORD", 4);
            } else if (stringToUpper(dataType) == "DQ") {
                node->size = OperandSize("QWORD", 8);
            } else {
                symbol = parseSess->symbolTable->findSymbol(token);
                if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {
                    node->size = OperandSize(structSymbol->token.lexeme, structSymbol->size);
                } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
                    node->size = OperandSize(recordSymbol->token.lexeme, 4);
                }
            }
            node->registers = {};
        } else if (std::dynamic_pointer_cast<LabelSymbol>(symbol) || std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
            node->constantValue = std::nullopt;
            node->isRelocatable = true;
            node->type = OperandType::MemoryOperand;
            node->size = OperandSize("DWORD", 4);
            node->registers = {};
        } else if (auto equVariableSymbol = std::dynamic_pointer_cast<EquVariableSymbol>(symbol)) {
            node->constantValue = equVariableSymbol->value;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = std::nullopt;
            node->registers = {};
        } else if (auto equalVariableSymbol = std::dynamic_pointer_cast<EqualVariableSymbol>(symbol)) {
            node->constantValue = equalVariableSymbol->value;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = std::nullopt;
            node->registers = {};
        } else if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {
            if (!context.isPTROperand && !context.isSizeOperand) {
                node->diagnostic = reportTypeNotAllowed(token);
                return false;
            }
            node->type = OperandType::Unspecified;
        } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
            if (!context.isSizeOperand && !context.isWidthOrMaskOperand) {
                node->diagnostic = reportRecordNotAllowed(token);
                return false;
            }
            node->type = OperandType::Unspecified;
        } else if (auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            if (!context.isWidthOrMaskOperand) {
                node->diagnostic = reportRecordFieldNotAllowed(token);
            }
            node->type = OperandType::Unspecified;
        }

    } else if (token.type == TokenType::Number) {
        auto numberValue = parseNumber(token.lexeme);
        if (!numberValue) {
            node->diagnostic = reportNumberTooLarge(token);
            return false;
        }
        node->constantValue = static_cast<int64_t>(numberValue.value());
        node->isRelocatable = false;
        node->size = std::nullopt;
        node->type = OperandType::ImmediateOperand;
        node->registers = {};
    } else if (token.type == TokenType::StringLiteral) {
        // if expressionDepth == 1 in DataDefiniton context - string literal can be any length
        // any other - needs to be less than 4 bytes
        // size() + 2, because " " are in the lexeme
        if ((context.allowRegisters && token.lexeme.size() > 4 + 2) || (expressionDepth > 1 && token.lexeme.size() > 4 + 2)) {
            node->diagnostic = reportStringTooLarge(token);
            return false;
        }
        node->unresolvedSymbols = true;
        node->constantValue = -1; // TODO: convert string value to bytes and then to int32_t
        node->isRelocatable = false;
        node->size = std::nullopt;
        node->type = OperandType::ImmediateOperand;
        node->registers = {};
    } else if (token.type == TokenType::Register) {
        if (!context.allowRegisters) {
            node->diagnostic = reportRegisterNotAllowed(token);
            return false;
        }
        node->constantValue = std::nullopt;
        node->isRelocatable = false;
        int value = registerSizes[stringToUpper(token.lexeme)];
        node->size = OperandSize(sizeValueToStr[value], value);
        node->type = OperandType::RegisterOperand;
        node->registers[token] = std::nullopt;
    } else if (token.type == TokenType::Dollar) {
        node->constantValue = std::nullopt;
        node->isRelocatable = true;
        node->type = OperandType::ImmediateOperand;
        node->size = OperandSize("DWORD", 4);
        node->registers = {};
    } else if (token.type == TokenType::Type) {
        if (!context.isPTROperand && !context.isSizeOperand) {
            node->diagnostic = reportTypeNotAllowed(token);
            return false;
        }
        node->type = OperandType::Unspecified;
    } else {
        LOG_DETAILED_ERROR("Unkown leaf token!");
        return false;
    }
    return true;
}
