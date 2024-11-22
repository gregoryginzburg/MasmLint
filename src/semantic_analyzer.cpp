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
    if (auto program = std::dynamic_pointer_cast<Program>(node)) {
        for (auto expr : program->expressions) {
            panicLine = false;
            expressionDepth = 0;
            visitExpression(expr, ExpressionContext::InstructionOperand);
            if (expr->type == OperandType::UnfinishedMemoryOperand) {
                reportInvalidAddressExpression(expr);
            }
        }
    }
}

void SemanticAnalyzer::visitExpression(ASTExpressionPtr node, ExpressionContext context)
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
    } else if (auto invalidExpr = std::dynamic_pointer_cast<InvalidExpression>(node)) {
        visitInvalidExpression(invalidExpr, context);
    } else {
        LOG_DETAILED_ERROR("Unknown expression ptr node");
    }
    expressionDepth--;
}

void SemanticAnalyzer::visitBrackets(std::shared_ptr<Brackets> node, ExpressionContext context)
{
    visitExpression(node->operand, context);

    auto operand = node->operand;
    node->constantValue = operand->constantValue;
    node->isRelocatable = operand->isRelocatable;
    node->type = operand->type;
    node->size = operand->size;
    node->registers = operand->registers;
}

void SemanticAnalyzer::visitSquareBrackets(std::shared_ptr<SquareBrackets> node, ExpressionContext context)
{
    visitExpression(node->operand, context);

    auto operand = node->operand;
    node->constantValue = operand->constantValue;
    node->isRelocatable = operand->isRelocatable;
    if (operand->type == OperandType::UnfinishedMemoryOperand) {
        node->type = OperandType::MemoryOperand;
    } else if (operand->type == OperandType::RegisterOperand) {
        node->type = OperandType::MemoryOperand;
    } else {
        node->type = operand->type;
    }
    node->size = std::nullopt;
    node->registers = operand->registers;
}

void SemanticAnalyzer::visitImplicitPlusOperator(std::shared_ptr<ImplicitPlusOperator> node, ExpressionContext context)
{
    visitExpression(node->left, context);
    visitExpression(node->right, context);

    auto left = node->left;
    auto right = node->right;

    if (!(left->isRelocatable && right->isRelocatable)) {
        // check that we have not more then 2 registers, and only one has scale (eax * 1) is counted as having scale
        if (left->registers.size() + right->registers.size() <= 2) {
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
            if (scaleCount < 2) {
                // everything now is correct
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
            }
            node->type = OperandType::InvalidOperand;
            reportMoreThanOneScaleAfterAdd(node, true);
            return;
        }
        node->type = OperandType::InvalidOperand;
        reportMoreThanTwoRegistersAfterAdd(node, true);
        return;
    }
    node->type = OperandType::InvalidOperand;
    if (left->type == OperandType::UnfinishedMemoryOperand) {
        reportInvalidAddressExpression(left);
        return;
    } else if (right->type == OperandType::UnfinishedMemoryOperand) {
        reportInvalidAddressExpression(right);
        return;
    }
    reportCantAddVariables(node, true);
}

void SemanticAnalyzer::visitBinaryOperator(std::shared_ptr<BinaryOperator> node, ExpressionContext context)
{
    visitExpression(node->left, context);
    visitExpression(node->right, context);

    std::string op = stringToUpper(node->op.lexeme);
    auto left = node->left;
    auto right = node->right;

    if (op == ".") {
        if (!left->constantValue && left->type != OperandType::RegisterOperand &&
            left->type != OperandType::UnfinishedMemoryOperand) {
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
        node->type = OperandType::InvalidOperand;
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(right);
            return;
        }
        reportDotOperatorIncorrectArgument(node);

    } else if (op == "PTR") {
        std::shared_ptr<Leaf> leaf;
        // CRITICAL TODO: left can also be identifier, if it's a type symbol
        if ((leaf = std::dynamic_pointer_cast<Leaf>(left)) &&
            leaf->token.type == TokenType::Type /* !=TokenType::Identifier */) {
            if (right->type != OperandType::UnfinishedMemoryOperand && right->type != OperandType::RegisterOperand) {
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
        node->type = OperandType::InvalidOperand;
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(right);
            return;
        }
        reportPtrOperatorIncorrectArgument(node);
    } else if (op == "*" || op == "/" || op == "MOD" || op == "SHL" || op == "SHR") {
        // handle eax * 4
        if (op == "*") {
            if (left->constantValue && right->type == OperandType::RegisterOperand ||
                right->constantValue && left->type == OperandType::RegisterOperand) {
                int32_t value;
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
        node->type = OperandType::InvalidOperand;
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(right);
            return;
        }
        reportOtherBinaryOperatorIncorrectArgument(node);
    } else if (op == "+") {
        if (!(left->isRelocatable && right->isRelocatable)) {
            // check that we have not more then 2 registers, and only one has scale (eax * 1) is counted as having scale
            if (left->registers.size() + right->registers.size() <= 2) {
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
                if (scaleCount < 2) {
                    // everything now is correct
                    if (left->constantValue && right->constantValue) {
                        node->constantValue = left->constantValue.value() + right->constantValue.value();
                    } else {
                        node->constantValue = std::nullopt;
                    }
                    node->isRelocatable = left->isRelocatable || right->isRelocatable;

                    if (left->type == OperandType::ImmediateOperand && right->type == OperandType::ImmediateOperand) {
                        node->type = OperandType::ImmediateOperand;
                    } else if (left->type == OperandType::RegisterOperand ||
                               right->type == OperandType::RegisterOperand) {
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
                }
                node->type = OperandType::InvalidOperand;
                reportMoreThanOneScaleAfterAdd(node, false);
                return;
            }
            node->type = OperandType::InvalidOperand;
            reportMoreThanTwoRegistersAfterAdd(node, false);
            return;
        }
        node->type = OperandType::InvalidOperand;
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(right);
            return;
        }
        reportCantAddVariables(node, false);
    } else if (op == "-") {
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
        if (right->constantValue) {
            // everything fine
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
        node->type = OperandType::InvalidOperand;
        if (left->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(left);
            return;
        } else if (right->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(right);
            return;
        }
        reportBinaryMinusOperatorIncorrectArgument(node);
    } else {
        LOG_DETAILED_ERROR("Unknown binary operator!");
    }
}

void SemanticAnalyzer::visitUnaryOperator(std::shared_ptr<UnaryOperator> node, ExpressionContext context)
{
    visitExpression(node->operand, context);

    std::string op = stringToUpper(node->op.lexeme);
    auto operand = node->operand;

    if (op == "LENGTH" || op == "LENGTHOF") {
        std::shared_ptr<Leaf> leaf;
        if ((leaf = std::dynamic_pointer_cast<Leaf>(operand)) && leaf->token.type == TokenType::Identifier) {
            // TODO: calculate actual size
            node->constantValue = 1;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = OperandSize("DWORD", 4);
            node->registers = {};
            return;
        }
        node->type = OperandType::InvalidOperand;
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(operand);
            return;
        }
        reportUnaryOperatorIncorrectArgument(node);
    } else if (op == "SIZE" || op == "SIZEOF") {
        std::shared_ptr<Leaf> leaf;
        if ((leaf = std::dynamic_pointer_cast<Leaf>(operand)) && leaf->token.type == TokenType::Identifier) {
            // TODO: calculate actual size
            node->constantValue = 1;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = OperandSize("DWORD", 4);
            node->registers = {};
            return;
        }
        node->type = OperandType::InvalidOperand;
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(operand);
            return;
        }
        reportUnaryOperatorIncorrectArgument(node);
    } else if (op == "WIDTH" || op == "MASK") {
        std::shared_ptr<Leaf> leaf;
        if ((leaf = std::dynamic_pointer_cast<Leaf>(operand)) && leaf->token.type == TokenType::Identifier) {
            // TODO: check that leaf->token id is a record type (or field of record)
            // TODO: calculate actual size
            node->constantValue = 1;
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = OperandSize("DWORD", 4);
            node->registers = {};
            return;
        }
        node->type = OperandType::InvalidOperand;
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(operand);
            return;
        }
        reportUnaryOperatorIncorrectArgument(node);
    } else if (op == "OFFSET") {
        if (operand->constantValue || operand->registers.empty()) {
            node->constantValue = operand->constantValue;
            node->isRelocatable = operand->isRelocatable;
            node->type = OperandType::ImmediateOperand;
            node->size = operand->size;
            node->registers = operand->registers;
            return;
        }
        node->type = OperandType::InvalidOperand;
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(operand);
            return;
        }
        reportUnaryOperatorIncorrectArgument(node);
    } else if (op == "TYPE") {
        if (operand->type != OperandType::UnfinishedMemoryOperand) {
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
        }
        node->type = OperandType::InvalidOperand;
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(operand);
            return;
        }
        reportUnaryOperatorIncorrectArgument(node);
    } else if (op == "+" || op == "-") {
        if (operand->constantValue) {
            if (op == "-") {
                node->constantValue = -operand->constantValue.value();
            } else if (op == "+") {
                node->constantValue = operand->constantValue.value();
            }
            node->isRelocatable = false;
            node->type = OperandType::ImmediateOperand;
            node->size = OperandSize("DWORD", 4);
            node->registers = {};
            return;
        }
        node->type = OperandType::InvalidOperand;
        if (operand->type == OperandType::UnfinishedMemoryOperand) {
            reportInvalidAddressExpression(operand);
            return;
        }
        reportUnaryOperatorIncorrectArgument(node);
    } else {
        LOG_DETAILED_ERROR("Unknown unary operator!");
    }
}

void SemanticAnalyzer::visitLeaf(std::shared_ptr<Leaf> node, ExpressionContext context)
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
        // TODO if context == DataDefinition - report error
        node->constantValue = std::nullopt;
        node->isRelocatable = false;
        std::map<std::string, int> registerSizes = {{"AL", 1},  {"AX", 2},  {"EAX", 4}, {"BL", 1},  {"BX", 2},
                                                    {"EBX", 4}, {"CL", 1},  {"CX", 2},  {"ECX", 4}, {"DL", 1},
                                                    {"DX", 2},  {"EDX", 4}, {"SI", 2},  {"ESI", 4}, {"DI", 2},
                                                    {"EDI", 4}, {"BP", 2},  {"EBP", 4}, {"SP", 2},  {"ESP", 4}};

        std::map<int, std::string> sizeValueToStr = {{1, "BYTE"}, {2, "WORD"}, {4, "DWORD"}};
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

void SemanticAnalyzer::visitInvalidExpression(std::shared_ptr<InvalidExpression> node,
                                              [[maybe_unused]] ExpressionContext context)
{
    panicLine = true;
    node->type = OperandType::InvalidOperand;
}
