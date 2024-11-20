#include "parser.h"
#include "log.h"
#include "tokenize.h"

std::shared_ptr<Diagnostic> Parser::reportUnclosedDelimiterError(const Token &closingDelimiter)
{
    if (panicLine) {
        return parseSess->dcx->getLastDiagnostic();
    }
    panicLine = true;
    if (expressionDelimitersStack.empty()) {
        LOG_DETAILED_ERROR("Empty demimiters stack!");
    } else {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNCLOSED_DELIMITER);
        diag.addPrimaryLabel(closingDelimiter.span, "");
        Token openingDelimiter = expressionDelimitersStack.top();
        diag.addSecondaryLabel(openingDelimiter.span, "unclosed delimiter");
        parseSess->dcx->addDiagnostic(diag);
    }
    return parseSess->dcx->getLastDiagnostic();
}

std::shared_ptr<Diagnostic> Parser::reportExpectedExpression(const Token &token)
{
    if (panicLine) {
        return parseSess->dcx->getLastDiagnostic();
    }
    panicLine = true;
    if ((token.type == TokenType::CloseSquareBracket || token.type == TokenType::CloseBracket) &&
        expressionDelimitersStack.empty()) {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNEXPECTED_CLOSING_DELIMITER, token.lexeme);
        diag.addPrimaryLabel(token.span, "");
        parseSess->dcx->addDiagnostic(diag);
    } else {

        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_EXPRESSION,
                        token.type == TokenType::EndOfLine ? "\\n" : token.lexeme);
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
    if (panicLine) {
        return parseSess->dcx->getLastDiagnostic();
    }
    panicLine = true;
    Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXCPECTED_OPERATOR_OR_CLOSING_DELIMITER, token.lexeme);
    diag.addPrimaryLabel(token.span, "");
    parseSess->dcx->addDiagnostic(diag);
    return parseSess->dcx->getLastDiagnostic();
}