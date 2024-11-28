// File: semantic_analyzer.cpp
#include "semantic_analyzer.h"
#include "log.h"
#include <set>

std::map<std::string, int> SemanticAnalyzer::registerSizes = {
    {"AL", 1}, {"AX", 2},  {"EAX", 4}, {"BL", 1},  {"BX", 2},  {"EBX", 4}, {"CL", 1},
    {"CX", 2}, {"ECX", 4}, {"DL", 1},  {"DX", 2},  {"EDX", 4}, {"SI", 2},  {"ESI", 4},
    {"DI", 2}, {"EDI", 4}, {"BP", 2},  {"EBP", 4}, {"SP", 2},  {"ESP", 4}};

std::map<int, std::string> SemanticAnalyzer::sizeValueToStr = {{1, "BYTE"}, {2, "WORD"}, {4, "DWORD"}};

SemanticAnalyzer::SemanticAnalyzer(std::shared_ptr<ParseSession> parseSession, ASTPtr ast)
    : parseSess(std::move(parseSession)), ast(std::move(ast))
{
}

void SemanticAnalyzer::analyze() { visit(ast); }

void SemanticAnalyzer::visit(const ASTPtr &node)
{
    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        for (const auto &statement : program->statements) {
            visitStatement(statement);
        }
        if (program->endDir) {
            visitDirective(program->endDir);
        }
    } else {
        LOG_DETAILED_ERROR("Unknown AST node type in visit.");
    }
}

void SemanticAnalyzer::visitStatement(const std::shared_ptr<Statement> &statement)
{
    panicLine = false;
    if (INVALID(statement)) {
        return;
    }
    if (auto instruction = std::dynamic_pointer_cast<Instruction>(statement)) {
        visitInstruction(instruction);
    } else if (auto directive = std::dynamic_pointer_cast<Directive>(statement)) {
        visitDirective(directive);
    } else if (auto labelDef = std::dynamic_pointer_cast<LabelDef>(statement)) {
        visitLabelDef(labelDef);
    } else {
        LOG_DETAILED_ERROR("Unknown statement type.");
    }
}

void SemanticAnalyzer::visitInstruction(const std::shared_ptr<Instruction> &instruction)
{
    if (instruction->label) {
        visitLabelDef(instruction->label.value());
    }
    for (const auto &operand : instruction->operands) {
        visitExpression(operand, ExpressionContext::InstructionOperand);
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(operand);
        }
    }

    // TODO: Perform operand type checking and instruction validation
}

void SemanticAnalyzer::visitDirective(const std::shared_ptr<Directive> &directive)
{
    if (auto segDir = std::dynamic_pointer_cast<SegDir>(directive)) {
        visitSegDir(segDir);
    } else if (auto dataDir = std::dynamic_pointer_cast<DataDir>(directive)) {
        visitDataDir(dataDir);
    } else if (auto structDir = std::dynamic_pointer_cast<StructDir>(directive)) {
        visitStructDir(structDir);
    } else if (auto equDir = std::dynamic_pointer_cast<EquDir>(directive)) {
        visitEquDir(equDir);
    } else if (auto equalDir = std::dynamic_pointer_cast<EqualDir>(directive)) {
        visitEqualDir(equalDir);
    } else if (auto endDir = std::dynamic_pointer_cast<EndDir>(directive)) {
        visitEndDir(endDir);
    } else {
        LOG_DETAILED_ERROR("Unknown directive type.");
    }
}

void SemanticAnalyzer::visitLabelDef(const std::shared_ptr<LabelDef> & /*labelDef*/)
{
    // Check if the label is already defined
}

void SemanticAnalyzer::visitSegDir(const std::shared_ptr<SegDir> &segDir)
{
    if (segDir->constExpr) {
        visitExpression(segDir->constExpr.value(), ExpressionContext::DataDefinition);
    }
}

void SemanticAnalyzer::visitDataDir(const std::shared_ptr<DataDir> &dataDir)
{
    visitDataItem(dataDir->dataItem);
}

void SemanticAnalyzer::visitStructDir(const std::shared_ptr<StructDir> &structDir)
{
    for (const auto &field : structDir->fields) {
        if (INVALID(field)) {
            continue;
        }
        panicLine = false;
        visitDataDir(field);
    }
}

void SemanticAnalyzer::visitEquDir(const std::shared_ptr<EquDir> &equDir)
{
    visitExpression(equDir->value, ExpressionContext::DataDefinition);
}

void SemanticAnalyzer::visitEqualDir(const std::shared_ptr<EqualDir> &equalDir)
{
    visitExpression(equalDir->value, ExpressionContext::DataDefinition);
}

void SemanticAnalyzer::visitEndDir(const std::shared_ptr<EndDir> &endDir)
{
    if (endDir->addressExpr) {
        visitExpression(endDir->addressExpr.value(), ExpressionContext::DataDefinition);
    }
}

void SemanticAnalyzer::visitDataItem(const std::shared_ptr<DataItem> &dataItem)
{
    if (auto builtinInstance = std::dynamic_pointer_cast<BuiltinInstance>(dataItem)) {
        Token dataTypeToken = builtinInstance->dataTypeToken;
        std::string dataType = stringToUpper(dataTypeToken.lexeme);

        // Visit initialization values
        visitInitValue(builtinInstance->initValues, dataType);

    } else if (auto recordInstance = std::dynamic_pointer_cast<RecordInstance>(dataItem)) {
        // TODO: Implement record instance processing
        LOG_DETAILED_ERROR("Record instances are not yet implemented.");
    } else if (auto structInstance = std::dynamic_pointer_cast<StructInstance>(dataItem)) {
        // TODO: Implement struct instance processing
        LOG_DETAILED_ERROR("Struct instances are not yet implemented.");
    } else {
        LOG_DETAILED_ERROR("Unknown data item type.");
    }
}

void SemanticAnalyzer::visitInitValue(const std::shared_ptr<InitValue> &initValue, const std::string &dataType)
{
    if (auto dupOperator = std::dynamic_pointer_cast<DupOperator>(initValue)) {
        // Process DUP operator
        visitExpression(dupOperator->repeatCount, ExpressionContext::DataDefinition);

        // Visit operands
        visitInitValue(dupOperator->operands, dataType);

    } else if (auto questionMarkInitValue = std::dynamic_pointer_cast<QuestionMarkInitValue>(initValue)) {
        // Uninitialized data; no action needed

    } else if (auto expressionInitValue = std::dynamic_pointer_cast<ExpressionInitValue>(initValue)) {
        // Process expression init value
        visitExpression(expressionInitValue->expr, ExpressionContext::DataDefinition);

        // TODO: Implement type checking between expression and data type

    } else if (auto structOrRecordInitValue = std::dynamic_pointer_cast<StructOrRecordInitValue>(initValue)) {
        // Process struct or record initialization
        visitInitValue(structOrRecordInitValue->fields, dataType);

    } else if (auto initList = std::dynamic_pointer_cast<InitializerList>(initValue)) {
        for (const auto &init : initList->fields) {
            visitInitValue(init, dataType);
        }
    } else {
        LOG_DETAILED_ERROR("Unknown initialization value type.");
    }
}

void SemanticAnalyzer::visitExpression(const ExpressionPtr &node, ExpressionContext context)
{
    expressionDepth = 0;
    visitExpressionHelper(node, context);
}

void SemanticAnalyzer::visitExpressionHelper(const ExpressionPtr &node, ExpressionContext context)
{
    expressionDepth++;
    if (auto brackets = std::dynamic_pointer_cast<Brackets>(node)) {
        visitBrackets(brackets, context);
    } else if (auto squareBrackets = std::dynamic_pointer_cast<SquareBrackets>(node)) {
        visitSquareBrackets(squareBrackets, context);
    } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(node)) {
        visitImplicitPlusOperator(implicitPlus, context);
    } else if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
        visitBinaryOperator(binaryOp, context);
    } else if (auto unaryOp = std::dynamic_pointer_cast<UnaryOperator>(node)) {
        visitUnaryOperator(unaryOp, context);
    } else if (auto leaf = std::dynamic_pointer_cast<Leaf>(node)) {
        visitLeaf(leaf, context);
    } else {
        LOG_DETAILED_ERROR("Unknown expression ptr node");
    }
    expressionDepth--;
}

void SemanticAnalyzer::visitBrackets(const std::shared_ptr<Brackets> &node, ExpressionContext context)
{
    visitExpressionHelper(node->operand, context);

    auto operand = node->operand;
    node->constantValue = operand->constantValue;
    node->isRelocatable = operand->isRelocatable;
    node->type = operand->type;
    node->size = operand->size;
    node->registers = operand->registers;
}

void SemanticAnalyzer::visitSquareBrackets(const std::shared_ptr<SquareBrackets> &node, ExpressionContext context)
{
    visitExpressionHelper(node->operand, context);

    auto operand = node->operand;
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
                    if (binOpLeft->op.lexeme == "*" && (binOpLeft->left->type == OperandType::RegisterOperand ||
                                                        binOpLeft->right->type == OperandType::RegisterOperand)) {
                        firstIsRegisterWithOptionalScale = true;
                    }
                }
                bool firstIsConstant = bool(binOp->left->constantValue);
                bool secondIsRegisterWithOptionalScale = binOp->right->type == OperandType::RegisterOperand;
                if (auto binOpRight = std::dynamic_pointer_cast<BinaryOperator>(binOp->right)) {
                    if (binOpRight->op.lexeme == "*" && (binOpRight->left->type == OperandType::RegisterOperand ||
                                                         binOpRight->right->type == OperandType::RegisterOperand)) {
                        secondIsRegisterWithOptionalScale = true;
                    }
                }
                bool secondIsConstant = bool(binOp->right->constantValue);
                if ((firstIsConstant && secondIsRegisterWithOptionalScale) ||
                    (firstIsRegisterWithOptionalScale && secondIsConstant)) {
                    // allowed
                } else {
                    node->type = OperandType::InvalidOperand;
                    reportNonRegisterInSquareBrackets(binOp);
                    return;
                }
            } else if (binOp->op.lexeme != "*") {
                node->type = OperandType::InvalidOperand;
                reportNonRegisterInSquareBrackets(binOp);
                return;
            }
            expr = binOp;
        } else if ((implicitPlusOp = std::dynamic_pointer_cast<ImplicitPlusOperator>(operand))) {
            // report error, beacuse can't have [[eax][ebx]]
            node->type = OperandType::InvalidOperand;
            reportNonRegisterInSquareBrackets(binOp);
            return;
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
            node->type = OperandType::InvalidOperand;
            reportNon32bitRegister(expr, implicit);
            return;
        }
        if (espCount == 2) {
            node->type = OperandType::InvalidOperand;
            reportTwoEsp(expr, implicit);
            return;
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
            node->type = OperandType::InvalidOperand;
            reportNon32bitRegister(operand, true);
            return;
        }
        node->type = OperandType::MemoryOperand;
    } else {
        node->type = operand->type;
    }
    if (operand->registers.empty()) {
        node->size = operand->size;
    } else {
        // modifiactors reset known size
        node->size = std::nullopt;
    }
    node->registers = operand->registers;
}

void SemanticAnalyzer::visitImplicitPlusOperator(const std::shared_ptr<ImplicitPlusOperator> &node,
                                                 ExpressionContext context)
{
    visitExpressionHelper(node->left, context);
    visitExpressionHelper(node->right, context);

    auto left = node->left;
    auto right = node->right;

    if (left->isRelocatable && right->isRelocatable) {
        node->type = OperandType::InvalidOperand;
        reportCantAddVariables(node, true);
        return;
    }
    // check that we have not more then 2 registers, and only one has scale (eax * 1) is counted as having scale
    if (left->registers.size() + right->registers.size() > 2) {
        node->type = OperandType::InvalidOperand;
        reportMoreThanTwoRegistersAfterAdd(node, true);
        return;
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
        node->type = OperandType::InvalidOperand;
        reportMoreThanOneScaleAfterAdd(node, true);
        return;
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
        node->type = OperandType::InvalidOperand;
        reportNon32bitRegister(node, true);
        return;
    }
    if (espCount == 2) {
        node->type = OperandType::InvalidOperand;
        reportTwoEsp(node, true);
        return;
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
    } else if (left->type == OperandType::UnfinishedMemoryOperand ||
               right->type == OperandType::UnfinishedMemoryOperand) {
        node->type = OperandType::UnfinishedMemoryOperand;
    } else if (left->type == OperandType::MemoryOperand || right->type == OperandType::MemoryOperand) {
        node->type = OperandType::MemoryOperand;
    } else {
        // shouldn't happen (except in OperandType::InvalidOperand)
    }

    if (!left->size || !right->size) {
        node->size = std::nullopt;
    } else {
        // TODO: report that operands have different sizes
        node->size = left->size;
    }
    node->registers = newRegisters;
}

void SemanticAnalyzer::visitBinaryOperator(const std::shared_ptr<BinaryOperator> &node, ExpressionContext context)
{
    visitExpressionHelper(node->left, context);
    visitExpressionHelper(node->right, context);

    std::string op = stringToUpper(node->op.lexeme);
    auto left = node->left;
    auto right = node->right;

    if (op == ".") {
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(right);
            return;
        }
        if (left->type == OperandType::MemoryOperand) {
            std::shared_ptr<Leaf> leaf;
            if ((leaf = std::dynamic_pointer_cast<Leaf>(right)) && leaf->token.type == TokenType::Identifier) {
                // TODO: check that left type has field and that field is valid
                node->constantValue = std::nullopt;
                node->isRelocatable = left->isRelocatable;
                node->type = OperandType::MemoryOperand;
                node->size = left->size;
                node->registers = left->registers;
                return;
            }
        }
        reportDotOperatorIncorrectArgument(node);
    } else if (op == "PTR") {
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(right);
            return;
        }
        std::shared_ptr<Leaf> leaf;
        // CRITICAL TODO: left can also be identifier, if it's a type symbol
        if ((leaf = std::dynamic_pointer_cast<Leaf>(left)) &&
            leaf->token.type == TokenType::Type /* !=TokenType::Identifier */) {
            if (right->type != OperandType::RegisterOperand) {
                node->constantValue = right->constantValue;
                node->isRelocatable = right->isRelocatable;
                node->type = right->type;
                // TODO: get size from right id symbol table
                std::string typeOperand = stringToUpper(leaf->token.lexeme);
                if (typeOperand == "BYTE") {
                    node->size = OperandSize("BYTE", 1);
                } else if (typeOperand == "WORD") {
                    node->size = OperandSize("WORD", 2);
                } else if (typeOperand == "DWORD") {
                    node->size = OperandSize("DWORD", 4);
                } else if (typeOperand == "QWORD") {
                    node->size = OperandSize("QWORD", 8);
                }
                node->registers = right->registers;
                return;
            }
        }
        reportPtrOperatorIncorrectArgument(node);
    } else if (op == "*" || op == "/" || op == "MOD" || op == "SHL" || op == "SHR") {
        // can check this immediately cause eax * 4, eax is RegisterOperand
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(right);
            return;
        }
        // handle eax * 4
        if (op == "*") {
            if ((left->constantValue && right->type == OperandType::RegisterOperand) ||
                (right->constantValue && left->type == OperandType::RegisterOperand)) {
                int32_t value = 0;
                std::shared_ptr<Leaf> leafNode;
                if (left->constantValue) {
                    value = left->constantValue.value();
                    leafNode = std::dynamic_pointer_cast<Leaf>(right);
                } else {
                    value = right->constantValue.value();
                    leafNode = std::dynamic_pointer_cast<Leaf>(left);
                }
                if (value != 1 && value != 2 && value != 4 && value != 8) {
                    node->type = OperandType::InvalidOperand;
                    reportInvalidScaleValue(node);
                    return;
                }
                if (stringToUpper(leafNode->token.lexeme) == "ESP") {
                    node->type = OperandType::InvalidOperand;
                    reportIncorrectIndexRegister(leafNode);
                    return;
                }

                node->constantValue = std::nullopt;
                node->isRelocatable = false;
                node->type = OperandType::UnfinishedMemoryOperand;
                node->size = std::nullopt;
                if (left->type == OperandType::RegisterOperand) {
                    std::shared_ptr<Leaf> leftLeaf = std::dynamic_pointer_cast<Leaf>(left);
                    node->registers[leftLeaf->token] = value;
                } else {
                    std::shared_ptr<Leaf> rightLeaf = std::dynamic_pointer_cast<Leaf>(right);
                    node->registers[rightLeaf->token] = value;
                }
                return;
            }
        }
        if (left->constantValue && right->constantValue) {
            if (op == "*") {
                node->constantValue = left->constantValue.value() * right->constantValue.value();
            } else if (op == "/") {
                if (right->constantValue.value() == 0) {
                    node->type = OperandType::InvalidOperand;
                    reportDivisionByZero(node);
                    return;
                }
                node->constantValue = left->constantValue.value() / right->constantValue.value();
            } else if (op == "MOD") {
                if (right->constantValue.value() == 0) {
                    node->type = OperandType::InvalidOperand;
                    reportDivisionByZero(node);
                    return;
                }
                node->constantValue = left->constantValue.value() % right->constantValue.value();
            } else if (op == "SHL") {
                // TODO: calculate SHL
                node->constantValue = 0;
            } else if (op == "SHR") {
                // TODO: calculate SHR
                node->constantValue = 0;
            }

            node->constantValue = right->constantValue;
            node->isRelocatable = right->isRelocatable;
            node->type = right->type;
            node->size = OperandSize("DWORD", 4);
            node->registers = left->registers;
            return;
        }

        reportOtherBinaryOperatorIncorrectArgument(node);
    } else if (op == "+") {
        if (left->isRelocatable && right->isRelocatable) {
            node->type = OperandType::InvalidOperand;
            reportCantAddVariables(node, false);
            return;
        }
        // check that we have not more then 2 registers, and only one has scale (eax * 1) is counted as having scale
        if (left->registers.size() + right->registers.size() > 2) {
            node->type = OperandType::InvalidOperand;
            reportMoreThanTwoRegistersAfterAdd(node, false);
            return;
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
            node->type = OperandType::InvalidOperand;
            reportMoreThanOneScaleAfterAdd(node, false);
            return;
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
        } else if (left->type == OperandType::UnfinishedMemoryOperand ||
                   right->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::UnfinishedMemoryOperand;
        } else if (left->type == OperandType::MemoryOperand || right->type == OperandType::MemoryOperand) {
            node->type = OperandType::MemoryOperand;
        } else {
            // shouldn't happen (except in OperandType::InvalidOperand)
        }

        if (!left->size || !right->size) {
            node->size = std::nullopt;
        } else {
            // TODO: report that operands have different sizes
            node->size = left->size;
        }
        node->registers = newRegisters;
        return;

    } else if (op == "-") {
        if (!(left->registers.empty() && left->isRelocatable) && !right->constantValue) {
            node->type = OperandType::InvalidOperand;

            // need to check this after, cause after `-` can still be UnsinishedMemoryOperand
            if (left->type == OperandType::UnfinishedMemoryOperand) {
                reportInvalidAddressExpression(left);
                return;
            } else if (right->type == OperandType::UnfinishedMemoryOperand) {
                reportInvalidAddressExpression(right);
                return;
            }
            node->type = OperandType::InvalidOperand;
            reportBinaryMinusOperatorIncorrectArgument(node);
            return;
        }

        if (left->registers.empty() && left->isRelocatable) {
            // left is an adress expression, right can be an address expression or constant
            if (right->constantValue) {
                node->constantValue = left->constantValue.value() - right->constantValue.value();
                node->isRelocatable = false;
                node->type = OperandType::ImmediateOperand;
                node->size = OperandSize("DWORD", 4);
                node->registers = {};
                return;
            } else if (right->isRelocatable && right->registers.empty()) {
                node->constantValue = 1; // TODO: calculate actual constant value of the differece between 2 relocations
                node->isRelocatable = false;
                node->type = OperandType::ImmediateOperand;
                node->size = OperandSize("DWORD", 4);
                node->registers = {};
                return;
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
            return;
        }

    } else {
        LOG_DETAILED_ERROR("Unknown binary operator!");
    }
}

void SemanticAnalyzer::visitUnaryOperator(const std::shared_ptr<UnaryOperator> &node, ExpressionContext context)
{
    visitExpressionHelper(node->operand, context);

    std::string op = stringToUpper(node->op.lexeme);
    auto operand = node->operand;

    if (op == "LENGTH" || op == "LENGTHOF") {
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(operand);
            return;
        }
        std::shared_ptr<Leaf> leaf;
        // operand must be identifier
        if (!(leaf = std::dynamic_pointer_cast<Leaf>(operand)) || leaf->token.type != TokenType::Identifier) {
            node->type = OperandType::InvalidOperand;
            reportUnaryOperatorIncorrectArgument(node);
            return;
        }
        // TODO: calculate actual size
        node->constantValue = 1;
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = OperandSize("DWORD", 4);
        node->registers = {};
    } else if (op == "SIZE" || op == "SIZEOF") {
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(operand);
            return;
        }
        std::shared_ptr<Leaf> leaf;
        // operand must be identifier
        if (!(leaf = std::dynamic_pointer_cast<Leaf>(operand)) || leaf->token.type != TokenType::Identifier) {
            node->type = OperandType::InvalidOperand;
            reportUnaryOperatorIncorrectArgument(node);
            return;
        }
        // TODO: calculate actual size
        node->constantValue = 1;
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = OperandSize("DWORD", 4);
        node->registers = {};

    } else if (op == "WIDTH" || op == "MASK") {
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(operand);
            return;
        }
        std::shared_ptr<Leaf> leaf;
        // operand must be identifier
        if (!(leaf = std::dynamic_pointer_cast<Leaf>(operand)) || leaf->token.type != TokenType::Identifier) {
            node->type = OperandType::InvalidOperand;
            reportUnaryOperatorIncorrectArgument(node);
            return;
        }
        // TODO: check that leaf->token id is a record type (or field of record)
        // TODO: calculate actual size
        node->constantValue = 1;
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = OperandSize("DWORD", 4);
        node->registers = {};

    } else if (op == "OFFSET") {
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(operand);
            return;
        }
        // operand must be adress expression
        if (operand->constantValue || !operand->registers.empty()) {
            node->type = OperandType::InvalidOperand;
            reportUnaryOperatorIncorrectArgument(node);
            return;
        }
        node->constantValue = operand->constantValue;
        node->isRelocatable = operand->isRelocatable;
        node->type = OperandType::ImmediateOperand;
        node->size = operand->size;
        node->registers = operand->registers;

    } else if (op == "TYPE") {
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(operand);
            return;
        }

        if (!operand->size) {
            node->constantValue = 0;
            warnTypeReturnsZero(node);
        } else {
            node->constantValue = operand->size.value().value;
        }
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = OperandSize("DWORD", 4);
        node->registers = {};
        return;
    } else if (op == "+" || op == "-") {
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            node->type = OperandType::InvalidOperand;
            reportInvalidAddressExpression(operand);
            return;
        }
        // operand must be constant value
        if (!operand->constantValue) {
            node->type = OperandType::InvalidOperand;
            reportUnaryOperatorIncorrectArgument(node);
            return;
        }
        if (op == "-") {
            node->constantValue = -operand->constantValue.value();
        } else if (op == "+") {
            node->constantValue = operand->constantValue;
        }
        node->isRelocatable = false;
        node->type = OperandType::ImmediateOperand;
        node->size = OperandSize("DWORD", 4);
        node->registers = {};
    } else {
        LOG_DETAILED_ERROR("Unknown unary operator!");
    }
}

void SemanticAnalyzer::visitLeaf(const std::shared_ptr<Leaf> &node, ExpressionContext context)
{
    Token token = node->token;

    if (token.type == TokenType::Identifier) {
        // TODO: check if symbol is defined
        node->constantValue = std::nullopt;
        node->isRelocatable = true;
        node->size = OperandSize("DWORD", 4); // TODO: get size from symbol table
        node->type = OperandType::MemoryOperand;
        node->registers = {};
        // TODO: check if symbol is type and other properties from symbol table
    } else if (token.type == TokenType::Number) {
        auto numberValue = parseNumber(token.lexeme);
        if (!numberValue) {
            node->type = OperandType::InvalidOperand;
            reportNumberTooLarge(token);
            return;
        }
        node->constantValue = numberValue;
        node->isRelocatable = false;
        node->size = OperandSize("DWORD", 4);
        node->type = OperandType::ImmediateOperand;
        node->registers = {};
    } else if (token.type == TokenType::StringLiteral) {
        // if expressionDepth == 1 in DataDefiniton context - string literal can be any length
        // any other - needs to be less than 4 bytes
        // size() + 2, because " " are in the lexeme
        if (context == ExpressionContext::InstructionOperand && token.lexeme.size() > 4 + 2) {
            node->type = OperandType::InvalidOperand;
            reportStringTooLarge(token);
            return;
        }
        node->constantValue = 0; // TODO: convert string value to bytes and then to int32_t
        node->isRelocatable = false;
        node->size = OperandSize("DWORD", 4);
        node->type = OperandType::ImmediateOperand;
        node->registers = {};
    } else if (token.type == TokenType::Register) {
        if (context == ExpressionContext::DataDefinition) {
            node->type = OperandType::InvalidOperand;
            reportRegisterNotAllowed(token);
            return;
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
        // TODO if expressionDepth == 1 - report error (operand can't be a type)
        node->constantValue = std::nullopt;
        node->isRelocatable = false;
        node->type = OperandType::InvalidOperand;
        node->size = std::nullopt;
        node->registers = {};
    } else {
        LOG_DETAILED_ERROR("Unkown leaf token!");
    }
}
