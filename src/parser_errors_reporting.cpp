#include "parser.h"
#include "log.h"
#include "tokenize.h"

std::shared_ptr<Diagnostic> Parser::reportUnclosedDelimiterError(const Token &closingDelimiter)
{
    if (panicLine) {
        return parseSess->dcx->getLastDiagnostic();
    }
    panicLine = true;
    if (delimitersStack.empty()) {
        LOG_DETAILED_ERROR("Empty demimiters stack!");
    } else {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNCLOSED_DELIMITER);
        diag.addPrimaryLabel(closingDelimiter.span, "");
        Token openingDelimiter = delimitersStack.top();
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
        delimitersStack.empty()) {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::UNEXPECTED_CLOSING_DELIMITER, token.lexeme);
        diag.addPrimaryLabel(token.span, "");
        parseSess->dcx->addDiagnostic(diag);
    } else {

        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::EXPECTED_EXPRESSION,
                        token.type == TokenType::EndOfLine ? "\\n" : token.lexeme);
        diag.addPrimaryLabel(token.span, "");
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