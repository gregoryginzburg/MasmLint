#include "semantic_analyzer.h"
#include "log.h"
#include "tokenize.h"

// TODO: think more about naming types for users
// dont rely on this in code
std::string getOperandType(const ASTExpressionPtr &node)
{
    std::shared_ptr<Leaf> leaf;
    if ((leaf = std::dynamic_pointer_cast<Leaf>(node))) {
        switch (leaf->token.type) {
        case TokenType::Identifier:
            return "identifier"; // TODO: check from symbolTable what kind of symbol
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

void SemanticAnalyzer::reportNumberTooLarge(const Token &number)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::CONSTANT_TOO_LARGE);
    diag.addPrimaryLabel(number.span, "");
    diag.addNoteMessage("maximum allowed size is 32 bits");
    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportStringTooLarge(const Token &string)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::CONSTANT_TOO_LARGE);
    diag.addPrimaryLabel(string.span, "");
    diag.addNoteMessage("maximum allowed size is 32 bits");
    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportUnaryOperatorIncorrectArgument(const std::shared_ptr<UnaryOperator> &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNARY_OPERATOR_INCORRECT_ARGUMENT,
                    stringToUpper(node->op.lexeme));

    std::string op = stringToUpper(node->op.lexeme);
    std::string expectedStr;
    if (op == "LENGTH" || op == "LENGTHOF" || op == "SIZE" || op == "SIZEOF") {
        expectedStr = "expected `identifier`";
    } else if (op == "WIDTH" || op == "MASK") {
        // TODO: change expected type?
        expectedStr = "expected `identifier`";
    } else if (op == "OFFSET") {
        expectedStr = "expected `address expression`";
    } else if (op == "TYPE") {
        expectedStr = "expected valid expression";
    } else if (op == "+" || op == "-") {
        expectedStr = "expected `constant expression`";
    }
    auto operand = node->operand;
    diag.addPrimaryLabel(node->op.span, "");
    diag.addSecondaryLabel(getExpressionSpan(operand),
                           fmt::format("{}, found `{}`", expectedStr, getOperandType(operand)));

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportDotOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DOT_OPERATOR_INCORRECT_ARGUMENT);

    auto left = node->left;
    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "");

    std::shared_ptr<Leaf> leaf;
    if (left->constantValue || left->type == OperandType::RegisterOperand) {
        diag.addSecondaryLabel(getExpressionSpan(left),
                               fmt::format("expected `address expression`, found `{}`", getOperandType(left)));
    }
    if (!(leaf = std::dynamic_pointer_cast<Leaf>(right)) || leaf->token.type != TokenType::Identifier) {
        diag.addSecondaryLabel(getExpressionSpan(right),
                               fmt::format("expected `identifier`, found `{}`", getOperandType(right)));
    }

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportPtrOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::PTR_OPERATOR_INCORRECT_ARGUMENT);

    auto left = node->left;
    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "");

    std::shared_ptr<Leaf> leaf;
    // CRITICAL TODO: left can also be identifier, if it's a type symbol
    if (!(leaf = std::dynamic_pointer_cast<Leaf>(left)) ||
        leaf->token.type != TokenType::Type /* !=TokenType::Identifier */) {
        diag.addSecondaryLabel(getExpressionSpan(left),
                               fmt::format("expected `type`, found `{}`", getOperandType(left)));
    }
    if (right->type == OperandType::UnfinishedMemoryOperand || right->type == OperandType::RegisterOperand) {
        // Change that we can expect also constants?
        diag.addSecondaryLabel(getExpressionSpan(right),
                               fmt::format("expected `address expression`, found `{}`", getOperandType(right)));
    }

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportDivisionByZero(const std::shared_ptr<BinaryOperator> &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::DIVISION_BY_ZERO_IN_EXPRESSION);

    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "");

    diag.addSecondaryLabel(getExpressionSpan(right), "this evaluates to `0`");

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportInvalidScaleValue(const std::shared_ptr<BinaryOperator> &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_SCALE_VALUE);

    auto left = node->left;
    auto right = node->right;

    if (left->constantValue) {
        diag.addPrimaryLabel(getExpressionSpan(left),
                             fmt::format("this evaluates to `{}`", left->constantValue.value()));
    } else {
        diag.addPrimaryLabel(getExpressionSpan(right),
                             fmt::format("this evaluates to `{}`", right->constantValue.value()));
    }

    diag.addNoteMessage("scale can only be {1, 2, 4, 8}");

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportIncorrectIndexRegister(const std::shared_ptr<Leaf> &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INCORRECT_INDEX_REGISTER);
    diag.addPrimaryLabel(node->token.span, "");

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportOtherBinaryOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{

    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::OTHER_BINARY_OPERATOR_INCORRECT_ARGUMENT, node->op.lexeme);

    auto left = node->left;
    auto right = node->right;

    if (left->type == OperandType::RegisterOperand && node->op.lexeme == "*") {
        diag.addPrimaryLabel(node->op.span, "");
        diag.addSecondaryLabel(getExpressionSpan(right),
                               fmt::format("expected `constant expression`, found `{}`", getOperandType(right)));
    } else if (right->type == OperandType::RegisterOperand && node->op.lexeme == "*") {
        diag.addPrimaryLabel(node->op.span, "");
        diag.addSecondaryLabel(getExpressionSpan(left),
                               fmt::format("expected `constant expression`, found `{}`", getOperandType(left)));
    } else {
        if (node->op.lexeme == "*") {
            diag.addPrimaryLabel(node->op.span, "can only multiply constant expressions or a register by the scale");
        } else {
            diag.addPrimaryLabel(node->op.span,
                                 fmt::format("operator `{}` supports only constant expressions", node->op.lexeme));
        }

        diag.addSecondaryLabel(getExpressionSpan(left), fmt::format("help: this has type `{}`", getOperandType(left)));
        diag.addSecondaryLabel(getExpressionSpan(right),
                               fmt::format("help: this has type `{}`", getOperandType(right)));
    }

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::reportInvalidAddressExpression(const ASTExpressionPtr &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::INVALID_ADDRESS_EXPRESSION);

    // find first thing that lead to UnfinishedMemoryOperand and print it
    ASTExpressionPtr errorNode;
    findInvalidExpressionCause(node, errorNode);

    if (!errorNode) {
        diag.addPrimaryLabel(getExpressionSpan(node), "need to add [] to create a valid address expression");
    } else {
        if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(errorNode)) {
            diag.addPrimaryLabel(getExpressionSpan(binaryOp),
                                 "can't have registers inside expressions"); // TODO: change label string

        } else if (auto implicitPlus = std::dynamic_pointer_cast<ImplicitPlusOperator>(errorNode)) {
            diag.addPrimaryLabel(getExpressionSpan(implicitPlus), "can't have registers inside expressions");
        }
    }

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::findInvalidExpressionCause(const ASTExpressionPtr &node, ASTExpressionPtr &errorNode)
{
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
    } else if (auto invalidExpr = std::dynamic_pointer_cast<InvalidExpression>(node)) {
        return;
    } else {
        LOG_DETAILED_ERROR("Unknown ASTExpression Node!\n");
        return;
    }
}

void SemanticAnalyzer::reportCantAddVariables(const ASTExpressionPtr &node, bool implicit)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    std::optional<Token> firstVar;
    std::optional<Token> secondVar;
    findRelocatableVariables(node, firstVar, secondVar);
    if (!firstVar || !secondVar) {
        LOG_DETAILED_ERROR("Can't find the 2 relocatable variables!\n");
        return;
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
}

void SemanticAnalyzer::findRelocatableVariables(const ASTExpressionPtr &node, std::optional<Token> &firstVar,
                                                std::optional<Token> &secondVar)
{
    if (auto binaryOp = std::dynamic_pointer_cast<BinaryOperator>(node)) {
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
    } else if (auto invalidExpr = std::dynamic_pointer_cast<InvalidExpression>(node)) {
        return;
    } else {
        LOG_DETAILED_ERROR("Unknown ASTExpression Node!\n");
        return;
    }
}

void SemanticAnalyzer::reportMoreThanTwoRegistersAfterAdd(const ASTExpressionPtr &node, bool implicit)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

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
}

void SemanticAnalyzer::reportMoreThanOneScaleAfterAdd(const ASTExpressionPtr &node, bool implicit)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

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
}

void SemanticAnalyzer::reportTwoEsp(const ASTExpressionPtr &node, bool implicit)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

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
}

void SemanticAnalyzer::reportNon32bitRegister(const ASTExpressionPtr &node, bool implicit)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::NON_32BIT_REGISTER);

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
}

void SemanticAnalyzer::reportBinaryMinusOperatorIncorrectArgument(const std::shared_ptr<BinaryOperator> &node)
{
    if (panicLine) {
        return;
    }
    panicLine = true;

    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::BINARY_MINUS_OPERATOR_INCORRECT_ARGUMENT);

    auto left = node->left;
    auto right = node->right;

    diag.addPrimaryLabel(node->op.span, "can only subtract constant expressions or 2 address expressions");

    diag.addSecondaryLabel(getExpressionSpan(left), fmt::format("help: this has type `{}`", getOperandType(left)));
    diag.addSecondaryLabel(getExpressionSpan(right), fmt::format("help: this has type `{}`", getOperandType(right)));

    parseSess->dcx->addDiagnostic(diag);
}

void SemanticAnalyzer::warnTypeReturnsZero(const std::shared_ptr<UnaryOperator> &node)
{
    // TODO: why underlines every line without this
    // if (panicLine) {
    //     return;
    // }
    // panicLine = true;
    // when reprting warnings don't need to set panicLine to true
    Diagnostic diag(Diagnostic::Level::Warning, ErrorCode::TYPE_RETURNS_ZERO);
    diag.addPrimaryLabel(node->op.span, "");
    diag.addSecondaryLabel(getExpressionSpan(node->operand), "this expression doesn't have a type");
    parseSess->dcx->addDiagnostic(diag);
}