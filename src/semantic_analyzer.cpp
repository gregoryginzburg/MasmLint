// File: semantic_analyzer.cpp
#include "semantic_analyzer.h"
#include "log.h"
#include <set>
#include <unordered_set>
#include <cstdint>
#include <ranges>

std::map<std::string, int> SemanticAnalyzer::registerSizes = {{"AL", 1}, {"AX", 2},  {"EAX", 4}, {"BL", 1},  {"BX", 2},  {"EBX", 4}, {"CL", 1},
                                                              {"CX", 2}, {"ECX", 4}, {"DL", 1},  {"DX", 2},  {"EDX", 4}, {"SI", 2},  {"ESI", 4},
                                                              {"DI", 2}, {"EDI", 4}, {"BP", 2},  {"EBP", 4}, {"SP", 2},  {"ESP", 4}};

std::map<int, std::string> SemanticAnalyzer::sizeValueToStr = {{1, "BYTE"}, {2, "WORD"}, {4, "DWORD"}, {8, "QWORD"}};

std::map<std::string, int> SemanticAnalyzer::sizeStrToValue = {{"BYTE", 1}, {"WORD", 2}, {"DWORD", 4}, {"QWORD", 8}};

std::map<std::string, std::string> SemanticAnalyzer::dataDirectiveToSizeStr = {{"DB", "BYTE"}, {"DW", "WORD"}, {"DD", "DWORD"}, {"DQ", "QWORD"}};

std::unordered_set<std::string> SemanticAnalyzer::builtinTypes = {"BYTE", "WORD", "DWORD", "QWORD"};

std::unordered_set<std::string> SemanticAnalyzer::dataDirectives = {"DB", "DW", "DD", "DQ"};

OperandSize SemanticAnalyzer::getSizeFromToken(const Token &dataTypeToken)
{
    if (dataDirectives.contains(stringToUpper(dataTypeToken.lexeme))) {
        std::string typeStr = dataDirectiveToSizeStr[stringToUpper(dataTypeToken.lexeme)];

        return OperandSize(typeStr, sizeStrToValue[typeStr]);

    } else {
        std::shared_ptr<Symbol> dataTypeSymbol = parseSess->symbolTable->findSymbol(dataTypeToken);
        if (!dataTypeSymbol || !dataTypeSymbol->wasDefined) {
            return OperandSize("", -1);
        }

        if (!std::dynamic_pointer_cast<StructSymbol>(dataTypeSymbol) && !std::dynamic_pointer_cast<RecordSymbol>(dataTypeSymbol)) {
            return OperandSize("", -1);
        }

        if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(dataTypeSymbol)) {
            return OperandSize(structSymbol->token.lexeme, structSymbol->size);
        } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(dataTypeSymbol)) {
            return OperandSize(recordSymbol->token.lexeme, 4);
        }
    }
    return OperandSize("", -1);
}

OperandSize SemanticAnalyzer::getMinimumSizeForConstant(int64_t value)
{
    if (value >= INT8_MIN && value <= static_cast<int64_t>(UINT8_MAX)) {
        return OperandSize("BYTE", 1);
    } else if (value >= INT16_MIN && value <= static_cast<int64_t>(UINT16_MAX)) {
        return OperandSize("WORD", 2);
    } else if (value >= INT32_MIN && value <= static_cast<int64_t>(UINT32_MAX)) {
        return OperandSize("DWORD", 4);
    } else {
        return OperandSize("DWORD", 8);
    }
}

SemanticAnalyzer::SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast) : parseSess(std::move(parseSession)), ast(std::move(ast))
{
}

void SemanticAnalyzer::analyze()
{
    visit(ast);
    // Do the second pass
    pass = 2;
    for (const auto &line : linesForSecondPass) {
        if (auto instruction = std::dynamic_pointer_cast<Instruction>(line)) {
            std::ignore = visitInstruction(instruction);
        } else if (auto dataDir = std::dynamic_pointer_cast<DataDir>(line)) {
            std::ignore = visitDataDir(dataDir);
        } else {
            LOG_DETAILED_ERROR(
                "lines for second pass can only be those with possible forward references - thus only instructions or datadir or enddir");
        }
    }
}

void SemanticAnalyzer::visit(const ASTPtr &node)
{
    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        for (const auto &statement : program->statements) {
            if (INVALID(statement)) {
                continue;
            }
            currentLine = statement;
            std::ignore = visitStatement(statement);
        }
        if (program->endDir) {
            currentLine = program->endDir;
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
            instruction->diagnostic = operand->diagnostic;
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

    if (instruction->label && pass == 1) {
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(instruction->label.value());
        auto labelSymbol = std::dynamic_pointer_cast<LabelSymbol>(symbol);
        if (!labelSymbol) {
            LOG_DETAILED_ERROR("no label symbol in the symbol table, when should be");
            return false;
        }
        labelSymbol->value = currentOffset;
        labelSymbol->wasVisited = true;
        labelSymbol->wasDefined = true;
    }

    if (!instruction->mnemonicToken) {
        // only label exist
        return true;
    }

    // When we have at least 1 operand with unresolved symbols - return early - to catch errors on the second pass
    for (const auto &operand : instruction->operands) {
        if (operand->unresolvedSymbols) {
            return true;
        }
    }

    Token mnemonicToken = instruction->mnemonicToken.value();
    std::string mnemonic = stringToUpper(mnemonicToken.lexeme);

    // TODO: delete if some instruction accepts 8 bytes operand
    for (const auto &operand : instruction->operands) {
        if (operand->size) {
            int opSize = operand->size.value().value;
            if (opSize != 1 && opSize != 2 && opSize != 4) {
                instruction->diagnostic = reportInvalidOperandSize(operand, "{1, 2, 4}", opSize);
                return false;
            }
        }
    }

    if (mnemonic == "ADC" || mnemonic == "ADD" || mnemonic == "AND" || mnemonic == "CMP" || mnemonic == "MOV" || mnemonic == "OR" ||
        mnemonic == "SBB" || mnemonic == "SUB" || mnemonic == "TEST" || mnemonic == "XOR") {
        if (instruction->operands.size() != 2) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "2");
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

        if (!firstOp->size && !secondOp->size) {
            instruction->diagnostic = reportOneOfOperandsMustHaveSize(instruction);
            return false;
        }

        // find out actual size of constantValue
        // TODO: constantValue can still have size (cause of PTR)
        if (secondOp->constantValue) {
            int64_t value = secondOp->constantValue.value();
            secondOp->size = getMinimumSizeForConstant(value); // don't modify the AST tree here?
        }

        if (firstOp->size && secondOp->size) {
            int firstOpSize = firstOp->size.value().value;
            int secondOpSize = secondOp->size.value().value;
            if (secondOp->constantValue && firstOpSize < secondOpSize) {
                instruction->diagnostic = reportImmediateOperandTooBigForOperand(instruction, firstOpSize, secondOpSize);
                return false;
            }
            if (!secondOp->constantValue && firstOpSize != secondOpSize) {
                instruction->diagnostic = reportOperandsHaveDifferentSize(instruction, firstOpSize, secondOpSize);
                return false;
            }
        }

    } else if (mnemonic == "CALL" || mnemonic == "JMP") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];

        std::shared_ptr<Leaf> leaf = std::dynamic_pointer_cast<Leaf>(operand);
        if (!leaf) {
            instruction->diagnostic = reportOperandMustBeLabel(operand);
            return false;
        }
        if (leaf->token.type != TokenType::Identifier) {
            instruction->diagnostic = reportOperandMustBeLabel(operand);
            return false;
        }
        auto symbol = parseSess->symbolTable->findSymbol(leaf->token);
        if (!std::dynamic_pointer_cast<LabelSymbol>(symbol) && !std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
            instruction->diagnostic = reportOperandMustBeLabel(operand);
            return false;
        }
    } else if (mnemonic == "POP") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->type != OperandType::MemoryOperand && operand->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(operand);
            return false;
        }
        if (!operand->size) {
            instruction->diagnostic = reportOperandMustHaveSize(operand);
            return false;
        }
        int operandSize = operand->size.value().value;
        if (operandSize != 4) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize);
            return false;
        }
    } else if (mnemonic == "CBW" || mnemonic == "CDQ" || mnemonic == "CWD" || mnemonic == "POPFD" || mnemonic == "PUSHFD") {
        if (instruction->operands.size() != 0) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "0");
            return false;
        }
    } else if (mnemonic == "DEC" || mnemonic == "DIV" || mnemonic == "IDIV" || mnemonic == "IMUL" || mnemonic == "INC" || mnemonic == "MUL" ||
               mnemonic == "NEG" || mnemonic == "NOT") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->type != OperandType::MemoryOperand && operand->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(operand);
            return false;
        }
        if (!operand->size) {
            instruction->diagnostic = reportOperandMustHaveSize(operand);
            return false;
        }
    } else if (mnemonic == "JA" || mnemonic == "JAE" || mnemonic == "JB" || mnemonic == "JBE" || mnemonic == "JC" || mnemonic == "JE" ||
               mnemonic == "JECXZ" || mnemonic == "JG" || mnemonic == "JGE" || mnemonic == "JL" || mnemonic == "JLE" || mnemonic == "JNC" ||
               mnemonic == "JNE" || mnemonic == "JNZ" || mnemonic == "JZ" || mnemonic == "LOOP") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];

        std::shared_ptr<Leaf> leaf = std::dynamic_pointer_cast<Leaf>(operand);
        if (!leaf) {
            instruction->diagnostic = reportOperandMustBeLabel(operand);
            return false;
        }
        if (leaf->token.type != TokenType::Identifier) {
            instruction->diagnostic = reportOperandMustBeLabel(operand);
            return false;
        }
        auto symbol = parseSess->symbolTable->findSymbol(leaf->token);
        if (!std::dynamic_pointer_cast<LabelSymbol>(symbol) && !std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
            instruction->diagnostic = reportOperandMustBeLabel(operand);
            return false;
        }
    } else if (mnemonic == "LEA") {
        if (instruction->operands.size() != 2) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "2");
            return false;
        }
        ExpressionPtr firstOp = instruction->operands[0];
        ExpressionPtr secondOp = instruction->operands[1];
        if (firstOp->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeRegister(firstOp);
            return false;
        }
        if (firstOp->size.value().value != 4) {
            instruction->diagnostic = reportInvalidOperandSize(firstOp, "4", firstOp->size.value().value);
            return false;
        }
        if (secondOp->type != OperandType::MemoryOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOperand(secondOp);
            return false;
        }
    } else if (mnemonic == "MOVSX" || mnemonic == "MOVZX") {
        if (instruction->operands.size() != 2) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "2");
            return false;
        }
        ExpressionPtr firstOp = instruction->operands[0];
        ExpressionPtr secondOp = instruction->operands[1];
        if (firstOp->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeRegister(firstOp);
            return false;
        }
        if (secondOp->type != OperandType::MemoryOperand && secondOp->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(secondOp);
            return false;
        }

        if (!secondOp->size) {
            instruction->diagnostic = reportOperandMustHaveSize(secondOp);
            return false;
        }

        // now each operand is guaranteed to have a size
        if (firstOp->size.value().value <= secondOp->size.value().value) {
            int firstOpSize = firstOp->size.value().value;
            int secondOpSize = secondOp->size.value().value;
            instruction->diagnostic = reportFirstOperandMustBeBiggerThanSecond(instruction, firstOpSize, secondOpSize);
            return false;
        }

    } else if (mnemonic == "PUSH") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->constantValue) {
            int64_t value = operand->constantValue.value();
            operand->size = getMinimumSizeForConstant(value); // don't modify the AST tree here?
        }

        if (!operand->size) {
            instruction->diagnostic = reportOperandMustHaveSize(operand);
            return false;
        }

        int operandSize = operand->size.value().value;
        if (operand->constantValue && operandSize > 4) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize); // TODO: change this too immediate operand two big?
            return false;
        }
        if (!operand->constantValue && operandSize != 4) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize);
            return false;
        }
    } else if (mnemonic == "RCL" || mnemonic == "RCR" || mnemonic == "ROL" || mnemonic == "ROR" || mnemonic == "SHL" || mnemonic == "SHR") {
        if (instruction->operands.size() != 2) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "2");
            return false;
        }
        ExpressionPtr firstOp = instruction->operands[0];
        ExpressionPtr secondOp = instruction->operands[1];

        if (firstOp->type != OperandType::MemoryOperand && firstOp->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(firstOp);
            return false;
        }

        if (!firstOp->size) {
            instruction->diagnostic = reportOperandMustHaveSize(firstOp);
        }

        if (!secondOp->constantValue && !isRegister(secondOp, "CL")) {
            instruction->diagnostic = reportOperandMustBeImmediateOrCLRegister(secondOp);
            return false;
        }
        if (secondOp->constantValue) {
            OperandSize secondOpSize = getMinimumSizeForConstant(secondOp->constantValue.value());
            if (secondOpSize.value > 1) {
                instruction->diagnostic = reportInvalidOperandSize(secondOp, "1", secondOpSize.value);
            }
        }
    } else if (mnemonic == "RET") {
        if (instruction->operands.size() > 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "{0, 1}");
            return false;
        }

        if (instruction->operands.size() == 1) {
            ExpressionPtr operand = instruction->operands[0];
            if (operand->type != OperandType::ImmediateOperand) {
                instruction->diagnostic = reportOperandMustBeImmediate(operand);
                return false;
            }

            if (operand->constantValue) {
                int64_t value = operand->constantValue.value();
                operand->size = getMinimumSizeForConstant(value); // don't modify the AST tree here?
            }

            int operandSize = operand->size.value().value;
            if (operandSize > 2) {
                instruction->diagnostic = reportInvalidOperandSize(operand, "2", operandSize);
                return false;
            }
        }

    } else if (mnemonic == "XCHG") {
        if (instruction->operands.size() != 2) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "2");
            return false;
        }
        ExpressionPtr firstOp = instruction->operands[0];
        ExpressionPtr secondOp = instruction->operands[1];
        if (firstOp->type == OperandType::MemoryOperand && secondOp->type == OperandType::MemoryOperand) {
            instruction->diagnostic = reportCantHaveTwoMemoryOperands(instruction);
            return false;
        }

        if (firstOp->type != OperandType::MemoryOperand && firstOp->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(firstOp);
            return false;
        }

        if (secondOp->type != OperandType::MemoryOperand && secondOp->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(secondOp);
            return false;
        }

        // Now both operands are memory or register, at least one is a register - that means at least one of them has a size
        // So don't need to check for reportOneOfOperandsMustHaveSize
        if (firstOp->size && secondOp->size) {
            int firstOpSize = firstOp->size.value().value;
            int secondOpSize = secondOp->size.value().value;
            if (firstOpSize != secondOpSize) {
                instruction->diagnostic = reportOperandsHaveDifferentSize(instruction, firstOpSize, secondOpSize);
                return false;
            }
        }

    } else if (mnemonic == "INCHAR") {
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->type != OperandType::MemoryOperand && operand->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(operand);
            return false;
        }
        if (!operand->size) {
            instruction->diagnostic = reportOperandMustHaveSize(operand);
            return false;
        }
        int operandSize = operand->size.value().value;
        if (operand->constantValue && operandSize > 1) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize); // TODO: change this too immediate operand two big?
            return false;
        }
        if (!operand->constantValue && operandSize != 1) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize);
            return false;
        }
    } else if (mnemonic == "ININT") { // like `CALL`
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->type != OperandType::MemoryOperand && operand->type != OperandType::RegisterOperand) {
            instruction->diagnostic = reportOperandMustBeMemoryOrRegisterOperand(operand);
            return false;
        }
        if (!operand->size) {
            instruction->diagnostic = reportOperandMustHaveSize(operand);
            return false;
        }
        int operandSize = operand->size.value().value;
        if (operand->constantValue && operandSize > 4) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize); // TODO: change this too immediate operand two big?
            return false;
        }
        if (!operand->constantValue && operandSize != 4) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize);
            return false;
        }
    } else if (mnemonic == "EXIT" || mnemonic == "NEWLINE") {
        if (instruction->operands.size() != 0) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "0");
            return false;
        }
    } else if (mnemonic == "OUTI" || mnemonic == "OUTU" || mnemonic == "OUTSTR") { // like `PUSH`
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->constantValue) {
            int64_t value = operand->constantValue.value();
            operand->size = getMinimumSizeForConstant(value); // don't modify the AST tree here?
        }

        if (!operand->size) {
            instruction->diagnostic = reportOperandMustHaveSize(operand);
            return false;
        }

        int operandSize = operand->size.value().value;
        if (operandSize != 4) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "4", operandSize);
            return false;
        }
    } else if (mnemonic == "OUTCHAR") { // like `PUSH` with 8 bit
        if (instruction->operands.size() != 1) {
            instruction->diagnostic = reportInvalidNumberOfOperands(instruction, "1");
            return false;
        }
        ExpressionPtr operand = instruction->operands[0];
        if (operand->constantValue) {
            int64_t value = operand->constantValue.value();
            operand->size = getMinimumSizeForConstant(value); // don't modify the AST tree here?
        }

        if (!operand->size) {
            instruction->diagnostic = reportOperandMustHaveSize(operand);
            return false;
        }

        int operandSize = operand->size.value().value;
        if (operandSize != 1) {
            instruction->diagnostic = reportInvalidOperandSize(operand, "1", operandSize);
            return false;
        }
    }

    currentOffset += 1;
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
        if (!success) {
            segDir->diagnostic = segDir->constExpr.value()->diagnostic;
            return false;
        }
        if (!segDir->constExpr.value()->constantValue) {
            segDir->diagnostic = reportExpressionMustBeConstant(segDir->constExpr.value());
            return false;
        }
    }
    return true;
}

bool SemanticAnalyzer::visitDataDir(const std::shared_ptr<DataDir> &dataDir, const std::optional<Token> &strucNameToken)
{
    if (dataDir->idToken && pass == 1) {
        std::string fieldName = dataDir->idToken.value().lexeme;
        std::shared_ptr<DataVariableSymbol> dataVariableSymbol;
        if (strucNameToken) {
            auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(parseSess->symbolTable->findSymbol(strucNameToken.value()));
            dataVariableSymbol = structSymbol->namedFields[fieldName];
            dataVariableSymbol->wasVisited = true;
        } else {
            std::shared_ptr<Symbol> fieldSymbol = parseSess->symbolTable->findSymbol(fieldName);
            dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(fieldSymbol);
            dataVariableSymbol->wasVisited = true;
        }
        if (!dataVariableSymbol) {
            LOG_DETAILED_ERROR("no data variable symbol in the symbol table, when should be");
            return false;
        }
        dataVariableSymbol->value = currentOffset;
        bool success = visitDataItem(dataDir->dataItem, dataVariableSymbol);
        if (!success) {
            dataDir->diagnostic = dataDir->dataItem->diagnostic;
            return false;
        }
        dataVariableSymbol->wasDefined = true;
        return true;
    }
    return visitDataItem(dataDir->dataItem, std::nullopt);
}

bool SemanticAnalyzer::visitStructDir(const std::shared_ptr<StructDir> &structDir)
{
    auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(parseSess->symbolTable->findSymbol(structDir->firstIdToken));
    structSymbol->wasVisited = true;

    uint32_t startOffset = currentOffset;

    for (const auto &field : structDir->fields) {
        if (INVALID(field)) {
            continue;
        }
        currentLine = field;
        std::ignore = visitDataDir(field, structDir->firstIdToken);
    }

    uint32_t endOffset = currentOffset;

    structSymbol->size = structSymbol->sizeOf = endOffset - startOffset;
    structSymbol->wasDefined = true;
    return true;
}

bool SemanticAnalyzer::visitProcDir(const std::shared_ptr<ProcDir> &procDir)
{
    auto procSymbol = std::dynamic_pointer_cast<ProcSymbol>(parseSess->symbolTable->findSymbol(procDir->firstIdToken));
    procSymbol->wasVisited = true;
    procSymbol->value = currentOffset;
    procSymbol->wasDefined = true;

    for (const auto &instruction : procDir->instructions) {
        currentLine = instruction;
        std::ignore = visitInstruction(instruction);
    }

    return true;
}

bool SemanticAnalyzer::visitRecordDir(const std::shared_ptr<RecordDir> &recordDir)
{
    auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(parseSess->symbolTable->findSymbol(recordDir->idToken));
    recordSymbol->wasVisited = true;

    int32_t width = 0;
    for (const auto &field : recordDir->fields) {
        bool success = visitRecordField(field);
        if (!success) {
            recordDir->diagnostic = field->diagnostic;
            return false;
        }
        auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(parseSess->symbolTable->findSymbol(field->fieldToken));
        width += recordFieldSymbol->width;
    }

    if (width > 32) {
        recordDir->diagnostic = reportRecordWidthTooBig(recordDir, width);
        return false;
    }

    int32_t curWidth = 0;
    for (const auto &field : std::ranges::reverse_view(recordDir->fields)) {
        auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(parseSess->symbolTable->findSymbol(field->fieldToken));
        recordFieldSymbol->shift = curWidth;
        recordFieldSymbol->mask = 1 << (recordFieldSymbol->width - 1);
        recordFieldSymbol->wasDefined = true;
        curWidth += recordFieldSymbol->width;
    }

    recordSymbol->width = width;
    recordSymbol->wasDefined = true;
    recordSymbol->mask = 1 << (width - 1);

    return true;
}

bool SemanticAnalyzer::visitRecordField(const std::shared_ptr<RecordField> &recordField)
{
    auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(parseSess->symbolTable->findSymbol(recordField->fieldToken));
    recordFieldSymbol->wasVisited = true;

    bool success = visitExpression(recordField->width, ExpressionContext(ExprCtxtFlags::None));
    if (!success) {
        recordField->diagnostic = recordField->width->diagnostic;
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
            recordField->diagnostic = initialValueExpr.value()->diagnostic;
            return false;
        }

        if (!initialValueExpr.value()->constantValue) {
            recordField->diagnostic = reportExpressionMustBeConstant(initialValueExpr.value());
            return false;
        }
        // int64_t initialValue = initialValueExpr.value()->constantValue.value();
        // TODO: check that initialValue fits into `width` bytes and ask why masm doesnt check it
        uint32_t initialValue = static_cast<uint32_t>(initialValueExpr.value()->constantValue.value());
        recordFieldSymbol->initial = initialValue;
    }
    return true;
}

bool SemanticAnalyzer::visitEquDir(const std::shared_ptr<EquDir> &equDir)
{
    // TODO: can be a string
    auto equSymbol = std::dynamic_pointer_cast<EquVariableSymbol>(parseSess->symbolTable->findSymbol(equDir->idToken));
    equSymbol->wasVisited = true;

    bool success = visitExpression(equDir->value, ExpressionContext(ExprCtxtFlags::None));
    if (!success) {
        equDir->diagnostic = equDir->value->diagnostic;
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
    auto equalSymbol = std::dynamic_pointer_cast<EqualVariableSymbol>(parseSess->symbolTable->findSymbol(equalDir->idToken));
    equalSymbol->wasVisited = true;

    bool success = visitExpression(equalDir->value, ExpressionContext(ExprCtxtFlags::None));
    if (!success) {
        equalDir->diagnostic = equalDir->value->diagnostic;
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
            endDir->diagnostic = endDir->addressExpr.value()->diagnostic;
            return false;
        }
        // Must be address expression already, because registers aren't allowed
    }
    return true;
}

bool SemanticAnalyzer::visitDataItem(const std::shared_ptr<DataItem> &dataItem,
                                     const std::optional<std::shared_ptr<DataVariableSymbol>> &dataVariableSymbol)
{
    Token dataTypeToken = dataItem->dataTypeToken;

    if (dataDirectives.contains(stringToUpper(dataTypeToken.lexeme))) {
        if (dataVariableSymbol) {
            dataVariableSymbol.value()->dataTypeSize = getSizeFromToken(dataTypeToken);
        }
    } else {
        std::shared_ptr<Symbol> dataTypeSymbol = parseSess->symbolTable->findSymbol(dataTypeToken);
        if (!dataTypeSymbol) {
            dataItem->diagnostic = reportUndefinedSymbol(dataTypeToken, false);
            return false;
        }
        if (!dataTypeSymbol->wasVisited) {
            dataItem->diagnostic = reportUndefinedSymbol(dataTypeToken, true);
            return false;
        }
        if (!dataTypeSymbol->wasDefined) {
            dataItem->diagnostic = reportUndefinedSymbol(dataTypeToken, false);
            return false;
        }

        if (!std::dynamic_pointer_cast<StructSymbol>(dataTypeSymbol) && !std::dynamic_pointer_cast<RecordSymbol>(dataTypeSymbol)) {
            dataItem->diagnostic = reportInvalidDataType(dataItem);
            return false;
        }

        if (dataVariableSymbol) {
            dataVariableSymbol.value()->dataTypeSize = getSizeFromToken(dataTypeToken);
        }
    }
    bool success = visitInitValue(dataItem->initValues, dataVariableSymbol, dataTypeToken);
    if (!success) {
        dataItem->diagnostic = dataItem->initValues->diagnostic;
        return false;
    }
    uint32_t initValueSize = dataInitializerSize;
    currentOffset += initValueSize;

    int32_t dataTypeSizeInt = getSizeFromToken(dataTypeToken).value;
    if (dataVariableSymbol) {
        dataVariableSymbol.value()->sizeOf = initValueSize;
        if (dataTypeSizeInt == 0) {
            dataVariableSymbol.value()->lengthOf =
                0; // when dataTypeSizeInt = 0, then sizeof = 0 -> lengthof = 0 (but in masm empty STRUC cannot be instatiated)
        } else {
            dataVariableSymbol.value()->lengthOf = dataVariableSymbol.value()->sizeOf / dataTypeSizeInt;
        }
    }
    return true;
}

bool SemanticAnalyzer::visitInitValue(const std::shared_ptr<InitValue> &initValue,
                                      const std::optional<std::shared_ptr<DataVariableSymbol>> &dataVariableSymbol, const Token &expectedTypeToken)
{
    dataInitializerDepth = 0;
    dataInitializerSize = 0;
    return visitInitValueHelper(initValue, dataVariableSymbol, expectedTypeToken, 1);
}

bool SemanticAnalyzer::visitInitValueHelper(const std::shared_ptr<InitValue> &initValue,
                                            const std::optional<std::shared_ptr<DataVariableSymbol>> &dataVariableSymbol,
                                            const Token &expectedTypeToken, int dupMultiplier)
{
    dataInitializerDepth++;
    if (auto dupOperator = std::dynamic_pointer_cast<DupOperator>(initValue)) {
        bool success = visitExpression(dupOperator->repeatCount, ExpressionContext(ExprCtxtFlags::None));
        if (!success) {
            initValue->diagnostic = dupOperator->repeatCount->diagnostic;
            return false;
        }
        if (!dupOperator->repeatCount->constantValue) {
            initValue->diagnostic = reportExpressionMustBeConstant(dupOperator->repeatCount);
            return false;
        }
        // TODO: check that repeatCount is positive (or not negative)
        dupMultiplier = dupMultiplier * static_cast<int32_t>(dupOperator->repeatCount->constantValue.value());
        return visitInitValueHelper(dupOperator->operands, dataVariableSymbol, expectedTypeToken, dupMultiplier);

    } else if (auto questionMarkInitValue = std::dynamic_pointer_cast<QuestionMarkInitValue>(initValue)) {
        dataInitializerSize += getSizeFromToken(expectedTypeToken).value * dupMultiplier;
    } else if (auto expressionInitValue = std::dynamic_pointer_cast<ExpressionInitValue>(initValue)) {
        if (!dataDirectives.contains(stringToUpper(expectedTypeToken.lexeme))) {
            initValue->diagnostic = reportExpectedStrucOrRecordDataInitializer(expressionInitValue, expectedTypeToken);
            return false;
        }

        bool success;
        if (stringToUpper(expectedTypeToken.lexeme) == "DB") {
            success = visitExpression(expressionInitValue->expr,
                                      ExpressionContext(ExprCtxtFlags::AllowForwardReferences | ExprCtxtFlags::IsDBDirectiveOperand));
        } else if (stringToUpper(expectedTypeToken.lexeme) == "DQ") {
            success = visitExpression(expressionInitValue->expr,
                                      ExpressionContext(ExprCtxtFlags::AllowForwardReferences | ExprCtxtFlags::IsDQDirectiveOperand));
        } else {
            success = visitExpression(expressionInitValue->expr, ExpressionContext(ExprCtxtFlags::AllowForwardReferences));
        }
        if (!success) {
            initValue->diagnostic = expressionInitValue->expr->diagnostic;
            return false;
        }

        ExpressionPtr expr = expressionInitValue->expr;
        // find out actual size of constantValue
        if (expr->constantValue) {
            int64_t value = expr->constantValue.value();
            expr->size = getMinimumSizeForConstant(value); // don't modify the AST tree here?
        } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(expr)) {
            if (leaf->token.type == TokenType::StringLiteral) {
                // db "awdawd" - can fit any size (as an array)
                // dw, dd, dq - cant fit only a single value
                if (stringToUpper(expectedTypeToken.lexeme) == "DB") {
                    expr->size = OperandSize("BYTE", 1);
                } else {
                    expr->size = OperandSize("", static_cast<int>(leaf->token.lexeme.size()) - 2);
                }
            }
        }

        std::string expectedSizeStr = dataDirectiveToSizeStr[stringToUpper(expectedTypeToken.lexeme)];
        OperandSize expectedSize = OperandSize(expectedSizeStr, sizeStrToValue[expectedSizeStr]);
        if (!expr->size) {
            LOG_DETAILED_ERROR("no size can't happen without registers");
            return false;
        }

        if (expectedSize.value < expr->size->value && !expr->unresolvedSymbols) {
            initValue->diagnostic = reportInitializerTooLargeForSpecifiedSize(expressionInitValue, expectedTypeToken, expr->size->value);
            return false;
        }
        std::shared_ptr<Leaf> leaf;
        if ((leaf = std::dynamic_pointer_cast<Leaf>(expr)) && leaf->token.type == TokenType::StringLiteral &&
            stringToUpper(expectedTypeToken.lexeme) == "DB") {
            dataInitializerSize += static_cast<int32_t>((leaf->token.lexeme.size() - 2)) * dupMultiplier;
        } else {
            dataInitializerSize += getSizeFromToken(expectedTypeToken).value * dupMultiplier;
        }

    } else if (auto structOrRecordInitValue = std::dynamic_pointer_cast<StructOrRecordInitValue>(initValue)) {
        if (dataDirectives.contains(stringToUpper(expectedTypeToken.lexeme))) {
            initValue->diagnostic = reportExpectedSingleItemDataInitializer(structOrRecordInitValue, expectedTypeToken);
            return false;
        }

        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(expectedTypeToken);

        if (auto expectedRecordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
            if (structOrRecordInitValue->initList->fields.size() > expectedRecordSymbol->fields.size()) {
                initValue->diagnostic = reportTooManyInitialValuesForRecord(structOrRecordInitValue, expectedRecordSymbol);
                return false;
            }
            int i = 0;
            for (const auto &init : structOrRecordInitValue->initList->fields) {
                std::shared_ptr<ExpressionInitValue> exprInitValue;
                if (!(exprInitValue = std::dynamic_pointer_cast<ExpressionInitValue>(init))) {
                    initValue->diagnostic = reportExpectedSingleItemDataInitializer(init, expectedTypeToken);
                    return false;
                }
                // TODO: check that for size
                i += 1;
            }
            // add currentOffset for not initialized fields
            for (; i < structOrRecordInitValue->initList->fields.size(); ++i) {
                dataInitializerSize += 4 * dupMultiplier;
            }

        } else if (auto expectedStructSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {

            if (structOrRecordInitValue->initList->fields.size() > expectedStructSymbol->structDir->fields.size()) {
                initValue->diagnostic = reportTooManyInitialValuesForStruc(structOrRecordInitValue, expectedStructSymbol);
                return false;
            }
            int i = 0;
            for (const auto &init : structOrRecordInitValue->initList->fields) {
                Token newExpectedTypeToken = expectedStructSymbol->structDir->fields[i]->dataItem->dataTypeToken;

                if (std::dynamic_pointer_cast<DupOperator>(init)) {
                    if (dataDirectives.contains(stringToUpper(newExpectedTypeToken.lexeme))) {
                        initValue->diagnostic = reportExpectedSingleItemDataInitializer(init, newExpectedTypeToken);
                        return false;
                    } else {
                        initValue->diagnostic = reportExpectedStrucOrRecordDataInitializer(init, newExpectedTypeToken);
                        return false;
                    }
                }
                bool success = visitInitValueHelper(init, dataVariableSymbol, newExpectedTypeToken, dupMultiplier);
                if (!success) {
                    initValue->diagnostic = init->diagnostic;
                    return false;
                }
                i += 1;
            }
            // add currentOffset for not initialized fields
            for (; i < structOrRecordInitValue->initList->fields.size(); ++i) {
                Token newExpectedTypeToken = expectedStructSymbol->structDir->fields[i]->dataItem->dataTypeToken;
                dataInitializerSize += getSizeFromToken(newExpectedTypeToken).value * dupMultiplier;
            }
        }

    } else if (auto initList = std::dynamic_pointer_cast<InitializerList>(initValue)) {
        bool first = true;
        for (const auto &init : initList->fields) {
            uint32_t startOffset = dataInitializerSize;
            bool success = visitInitValueHelper(init, dataVariableSymbol, expectedTypeToken, dupMultiplier);
            if (!success) {
                initValue->diagnostic = init->diagnostic;
                return false;
            }
            uint32_t endOffset = dataInitializerSize;
            if (dataInitializerDepth == 1 && first && dataVariableSymbol) {
                dataVariableSymbol.value()->size = endOffset - startOffset;
                if (getSizeFromToken(expectedTypeToken).value == 0) {
                    dataVariableSymbol.value()->length =
                        0; // when dataTypeSizeInt = 0, then size = 0 -> length = 0 (but in masm empty STRUC cannot be instatiated)
                } else {
                    dataVariableSymbol.value()->length = dataVariableSymbol.value()->size / getSizeFromToken(expectedTypeToken).value;
                }
            }
            first = false;
        }
    } else {
        LOG_DETAILED_ERROR("Unknown initialization value type.");
        return false;
    }
    dataInitializerDepth--;
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
        node->diagnostic = node->operand->diagnostic;
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
        node->diagnostic = node->operand->diagnostic;
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
            if (binOp->registers.size() > 1) {
                node->diagnostic = reportMoreThanOneRegisterInSquareBrackets(binOp);
                return false;
            }
            expr = binOp;
        } else if ((implicitPlusOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(operand))) {
            // report error, beacuse can't have [[eax][ebx]]
            node->diagnostic = reportMoreThanOneRegisterInSquareBrackets(binOp);
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
    bool success = visitExpressionHelper(node->left, context);
    if (!success) {
        node->diagnostic = node->left->diagnostic;
        return false;
    }
    success = visitExpressionHelper(node->right, context);
    if (!success) {
        node->diagnostic = node->right->diagnostic;
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
    if (node->op.lexeme == ".") {
        contextRight.isStructField = true;
    }

    bool success = visitExpressionHelper(node->left, contextLeft);
    if (!success) {
        node->diagnostic = node->left->diagnostic;
        return false;
    }
    success = visitExpressionHelper(node->right, contextRight);
    if (!success) {
        node->diagnostic = node->right->diagnostic;
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
            LOG_DETAILED_ERROR("After `.` encountered not an identifier! (should be handled in the parsing stage)");
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
        if (!structSymbol->namedFields.contains(leafRight->token.lexeme)) {
            node->diagnostic = reportDotOperatorFieldDoesntExist(node, structSymbol->token.lexeme, leafRight->token.lexeme);
            return false;
        }

        node->constantValue = std::nullopt;
        node->isRelocatable = left->isRelocatable;
        node->type = OperandType::MemoryOperand;

        // Forward references must be allowed (when not allowed, the expression also must be constant
        // and for `.` error expression must be constant will be emitted)
        // TODO: test this...
        if (!structSymbol->wasDefined && pass == 1) {
            linesForSecondPass.push_back(currentLine);
            node->unresolvedSymbols = true;
        } else {
            node->unresolvedSymbols = false;
        }
        node->size = structSymbol->namedFields[leafRight->token.lexeme]->dataTypeSize;
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
            right->type == OperandType::RegisterOperand || right->type == OperandType::ImmediateOperand) {
            node->diagnostic = reportPtrOperatorIncorrectArgument(node);
            return false;
        }

        std::string typeOperand = leafLeft->token.lexeme;
        std::shared_ptr<Symbol> typeSymbol;
        if (!builtinTypes.contains(stringToUpper(typeOperand))) {
            typeSymbol = parseSess->symbolTable->findSymbol(leafLeft->token);
            // TODO: allow RecordSymbol too? (if so don't forget to change reportPtrOperatorIncorrectArgument(...) function)
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
        if (right->type == OperandType::ImmediateOperand && right->isRelocatable) {
            // OFFSET var - is immediate & relocatable
            // in this case PTR doesnt change the size
            node->size = right->size;
        } else {
            if (builtinTypes.contains(typeOperand)) {
                node->size = OperandSize(typeOperand, sizeStrToValue[typeOperand]);
            } else {
                auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(typeSymbol);
                node->size = OperandSize(typeOperand, structSymbol->size);
            }
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
                    node->constantValue = -1; // to avoid division by 0
                } else {
                    node->constantValue = left->constantValue.value() / right->constantValue.value();
                }
            } else if (op == "MOD") {
                if (right->constantValue.value() == 0 && !node->unresolvedSymbols) {
                    node->diagnostic = reportDivisionByZero(node);
                    return false;
                }
                if (node->unresolvedSymbols) {
                    node->constantValue = -1; // to avoid division by 0
                } else {
                    node->constantValue = left->constantValue.value() % right->constantValue.value();
                }
            } else if (op == "SHL") {
                node->constantValue = left->constantValue.value() << right->constantValue.value();
            } else if (op == "SHR") {
                node->constantValue = left->constantValue.value() >> right->constantValue.value();
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
            // TODO: report that operands have different sizes (or make sure it can't happen)
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
                std::optional<Token> firstVar;
                std::optional<Token> secondVar;
                findRelocatableVariables(node, firstVar, secondVar);
                if (!firstVar || !secondVar) {
                    LOG_DETAILED_ERROR("Can't find the 2 relocatable variables!\n");
                    return false;
                }
                auto firstVarSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(parseSess->symbolTable->findSymbol(firstVar.value()));
                auto secondVarSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(parseSess->symbolTable->findSymbol(secondVar.value()));
                node->constantValue = firstVarSymbol->value - secondVarSymbol->value;
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
    std::string op = stringToUpper(node->op.lexeme);
    bool success = visitExpressionHelper(node->operand, context);
    if (!success) {
        node->diagnostic = node->operand->diagnostic;
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
        // LENGTH(OF) doesn't work with StructSymbol, RecordSymbol, RecordFieldSymbol (TODO: add others?)
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(leaf->token);
        if (std::dynamic_pointer_cast<StructSymbol>(symbol) || std::dynamic_pointer_cast<RecordSymbol>(symbol) ||
            std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        if (auto dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(symbol)) {
            if (op == "LENGTH") {
                node->constantValue = dataVariableSymbol->length;
            } else if (op == "LENGTHOF") {
                node->constantValue = dataVariableSymbol->lengthOf;
            }
        } else {
            node->constantValue = operand->constantValue; // TODO: warn about this?
        }

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
        // SIZE(OF) doesn't work with RecordFieldSymbol (TODO: add others?)
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(leaf->token);
        if (std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            node->diagnostic = reportUnaryOperatorIncorrectArgument(node);
            return false;
        }
        if (auto dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(symbol)) {
            if (op == "SIZE") {
                node->constantValue = dataVariableSymbol->size;
            } else if (op == "SIZEOF") {
                node->constantValue = dataVariableSymbol->sizeOf;
            }
        } else {
            node->constantValue = operand->constantValue; // TODO: warn about this?
        }
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
        if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
            if (op == "WIDTH") {
                node->constantValue = recordSymbol->width;
            } else if (op == "MASK") {
                node->constantValue = recordSymbol->mask;
            }
        } else if (auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            if (op == "WIDTH") {
                node->constantValue = recordFieldSymbol->width;
            } else if (op == "MASK") {
                node->constantValue = recordFieldSymbol->mask;
            }
        }
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
        // OFFSET var is always 32 bit, OFFSET 1 has undefined size (std::nullopt)
        if (operand->size) {
            node->size = OperandSize("DWORD", 4);
        }
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
            // this is handled higher in the `.`
            node->type = OperandType::Unspecified;
            return true;
        }

        if (!parseSess->symbolTable->findSymbol(token)) {
            node->diagnostic = reportUndefinedSymbol(token, false);
            return false;
        }
        std::shared_ptr<Symbol> symbol = parseSess->symbolTable->findSymbol(token);
        if (!symbol->wasVisited && !context.allowForwardReferences) {
            node->diagnostic = reportUndefinedSymbol(token, true);
            return false;
        }
        if (!symbol->wasDefined && !context.allowForwardReferences) {
            node->diagnostic = reportUndefinedSymbol(token, false);
            return false;
        }

        if (!symbol->wasDefined && pass == 1) {
            linesForSecondPass.push_back(currentLine);
            node->unresolvedSymbols = true;
        } else {
            node->unresolvedSymbols = false;
        }

        if (auto dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(symbol)) {
            std::string dataType = dataVariableSymbol->dataType.lexeme;
            node->constantValue = std::nullopt;
            node->isRelocatable = true;
            if (context.allowRegisters) {
                node->type = OperandType::MemoryOperand; // data variable in instruction setting is a memory operand
                node->size = dataVariableSymbol->dataTypeSize;
            } else {
                node->type = OperandType::ImmediateOperand; // data variable in any other setting is immediate with size = 4
                node->size = OperandSize("DWORD", 4);
            }
            node->registers = {};
        } else if (std::dynamic_pointer_cast<LabelSymbol>(symbol) || std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
            node->constantValue = std::nullopt;
            node->isRelocatable = true;
            node->type = OperandType::ImmediateOperand;
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
            node->constantValue = structSymbol->size;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = std::nullopt;
            node->registers = {};
        } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
            node->constantValue = recordSymbol->mask;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = std::nullopt;
            node->registers = {};
        } else if (auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            node->constantValue = recordFieldSymbol->shift;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = std::nullopt;
            node->registers = {};
        }

    } else if (token.type == TokenType::Number) {
        if (context.isDQDirectiveOperand && expressionDepth == 1) {
            auto numberValue = parseNumber64bit(token.lexeme);
            if (!numberValue) {
                node->diagnostic = reportNumberTooLarge(token, 64);
                return false;
            }
            node->constantValue = static_cast<int32_t>(numberValue.value());
        } else {
            auto numberValue = parseNumber32bit(token.lexeme);
            if (!numberValue) {
                node->diagnostic = reportNumberTooLarge(token, 32);
                return false;
            }
            node->constantValue = numberValue.value();
        }

        node->isRelocatable = false;
        node->size = std::nullopt;
        node->type = OperandType::ImmediateOperand;
        node->registers = {};
    } else if (token.type == TokenType::StringLiteral) {
        // if expressionDepth == 1 in DataDefiniton context with db data directive - string literal can be any length
        // any other - needs to be less than 4 bytes
        // size() + 2, because " " are in the lexeme
        if ((context.allowRegisters || expressionDepth > 1 || !context.isDBDirectiveOperand) && token.lexeme.size() > 4 + 2) {
            node->diagnostic = reportStringTooLarge(token);
            return false;
        }
        if (!context.allowRegisters && expressionDepth == 1) {
            // Handle logic higher (visitInitValue)
            node->constantValue = std::nullopt;
            node->size = std::nullopt;
        } else {
            // Convert string value to bytes and then to int32_t
            std::string strValue = token.lexeme.substr(1, token.lexeme.size() - 2); // Remove quotes
            int32_t intValue = 0;
            size_t byteCount = strValue.size();

            // Pack characters into int32_t
            for (size_t i = 0; i < byteCount; ++i) {
                intValue |= static_cast<uint8_t>(strValue[i]) << (8 * i);
            }

            node->constantValue = intValue;
            node->size = std::nullopt;
        }
        node->isRelocatable = false;
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
        node->constantValue = sizeStrToValue[stringToUpper(token.lexeme)];
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = std::nullopt;
        node->registers = {};
    } else {
        LOG_DETAILED_ERROR("Unkown leaf token!");
        return false;
    }
    return true;
}
