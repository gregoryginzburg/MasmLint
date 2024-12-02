#include "parser.h"
#include "log.h"
#include "token.h"

// Program
std::shared_ptr<Diagnostic> Parser::reportExpectedEndOfLine(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_END_OF_LINE, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedEndDir(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_END_DIR);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

// Statement
std::shared_ptr<Diagnostic> Parser::reportMustBeInSegmentBlock(const Token &firstToken, const Token &lastToken)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::MUST_BE_IN_SEGMENT_BLOCK);
    Span span = Span::merge(firstToken.span, lastToken.span);
    diag.addPrimaryLabel(span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

// SegDir

// DataDir
std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierInDataDir(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER, token.lexeme);

    if (isReservedWord(token)) {
        diag.addPrimaryLabel(token.span, fmt::format("`{}` is a reserved word", token.lexeme));

    } else {
        diag.addPrimaryLabel(token.span, "");
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// StructDir
std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierBeforeStruc(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER_BEFORE_STRUC);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierInStrucDir(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER, token.lexeme);

    if (isReservedWord(token)) {
        diag.addPrimaryLabel(token.span, fmt::format("`{}` is a reserved word", token.lexeme));

    } else {
        diag.addPrimaryLabel(token.span, "");
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedDifferentIdentifierInStructDir(const Token &found,
                                                                                 const Token &expected)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_DIFFERENT_IDENTIFIER_STRUCT_DIR);
    diag.addPrimaryLabel(found.span, fmt::format("expected `{}`", expected.lexeme));
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedEnds(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_ENDS);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportMissingIdentifierBeforeEnds(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::MISSING_IDENTIFIER_BEFORE_ENDS);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

// ProcDir
std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierBeforeProc(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER_BEFORE_PROC);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierInProcDir(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER, token.lexeme);

    if (isReservedWord(token)) {
        diag.addPrimaryLabel(token.span, fmt::format("`{}` is a reserved word", token.lexeme));

    } else {
        diag.addPrimaryLabel(token.span, "");
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedDifferentIdentifierInProcDir(const Token &found,
                                                                               const Token &expected)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_DIFFERENT_IDENTIFIER_PROC_DIR);
    diag.addPrimaryLabel(found.span, fmt::format("expected `{}`", expected.lexeme));
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedEndp(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_ENDP);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportMissingIdentifierBeforeEndp(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::MISSING_IDENTIFIER_BEFORE_ENDP);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

// EqualDir
std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierInEquDir(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER, token.lexeme);

    if (isReservedWord(token)) {
        diag.addPrimaryLabel(token.span, fmt::format("`{}` is a reserved word", token.lexeme));

    } else {
        diag.addPrimaryLabel(token.span, "");
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierBeforeEqu(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER_BEFORE_EQU);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// EqualDir
std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierInEqualDir(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER, token.lexeme);

    if (isReservedWord(token)) {
        diag.addPrimaryLabel(token.span, fmt::format("`{}` is a reserved word", token.lexeme));

    } else {
        diag.addPrimaryLabel(token.span, "");
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierBeforeEqual(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER_BEFORE_EQUAL);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// Instruction
std::shared_ptr<Diagnostic> Parser::reportExpectedInstruction(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_INSTRUCTION);
    if (isReservedWord(token)) {
        diag.addPrimaryLabel(token.span, "this is a reserved word");
    } else if (token.type == TokenType::Identifier) {
        diag.addPrimaryLabel(token.span, "this instruction name is incorrect");
    } else {
        diag.addPrimaryLabel(token.span, "");
    }
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedCommaOrEndOfLine(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_COMMA_OR_END_OF_LINE, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierInLabel(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER, token.lexeme);

    if (isReservedWord(token)) {
        diag.addPrimaryLabel(token.span, fmt::format("`{}` is a reserved word", token.lexeme));

    } else {
        diag.addPrimaryLabel(token.span, "");
    }

    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// DataItem
std::shared_ptr<Diagnostic> Parser::reportExpectedVariableNameOrDataDirective(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_VARIABLE_NAME_OR_DATA_DIRECTIVE, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);

    return parseSess->dcx->getLastDiagnostic();
}

// InitValue
std::shared_ptr<Diagnostic> Parser::reportUnclosedDelimiterInDataInitializer(const Token &token)
{
    if (dataInitializerDelimitersStack.empty()) {
        LOG_DETAILED_ERROR("Empty demimiters stack!");
    } else {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNCLOSED_DELIMITER,
                        dataInitializerDelimitersStack.top().lexeme);
        diag.addPrimaryLabel(token.span, "");
        Token openingDelimiter = dataInitializerDelimitersStack.top();
        diag.addSecondaryLabel(openingDelimiter.span, "unclosed delimiter");
        parseSess->dcx->addDiagnostic(diag);
    }
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedCommaOrClosingDelimiter(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXCPECTED_COMMA_OR_CLOSING_DELIMITER, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedOpenBracket(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_OPEN_BRACKET, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

// Expression
std::shared_ptr<Diagnostic> Parser::reportUnclosedDelimiterError(const Token &closingDelimiter)
{
    if (expressionDelimitersStack.empty()) {
        LOG_DETAILED_ERROR("Empty demimiters stack!");
    } else {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNCLOSED_DELIMITER,
                        expressionDelimitersStack.top().lexeme);
        diag.addPrimaryLabel(closingDelimiter.span, "");
        Token openingDelimiter = expressionDelimitersStack.top();
        diag.addSecondaryLabel(openingDelimiter.span, "unclosed delimiter");
        parseSess->dcx->addDiagnostic(diag);
    }
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedExpression(const Token &token)
{
    if ((token.type == TokenType::CloseSquareBracket || token.type == TokenType::CloseBracket) &&
        expressionDelimitersStack.empty()) {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNEXPECTED_CLOSING_DELIMITER, token.lexeme);
        diag.addPrimaryLabel(token.span, "");
        parseSess->dcx->addDiagnostic(diag);
    } else {
        std::string lexeme;
        if (token.type == TokenType::EndOfLine) {
            lexeme = "\\n";
        } else if (token.type == TokenType::EndOfFile) {
            lexeme = "End Of File";
        } else {
            lexeme = token.lexeme;
        }
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_EXPRESSION, lexeme);
        diag.addPrimaryLabel(token.span, "");

        // 10 * MOD 3 - causes unexpected MOD
        // PTR [eax] - causes unexpected PTR
        // add note message, saying that MOD, SHL, SHR, PTR take 2 arguments
        // other binary operators are obvious enough
        std::string lexemeUpper = stringToUpper(token.lexeme);
        if (lexemeUpper == "MOD" || lexemeUpper == "SHL" || lexemeUpper == "SHR" || lexemeUpper == "PTR") {
            diag.addNoteMessage(fmt::format("{} operator takes 2 arguments", lexemeUpper));
        }
        parseSess->dcx->addDiagnostic(diag);
    }
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedOperatorOrClosingDelimiter(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXCPECTED_OPERATOR_OR_CLOSING_DELIMITER, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedIdentifierInExpression(const Token &token)
{
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_IDENTIFIER, token.lexeme);
    diag.addPrimaryLabel(token.span, "this needs to be a field name");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}
