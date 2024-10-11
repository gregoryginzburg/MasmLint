#include "parser.h"
#include "symbol_table.h"

Token Parser::currentToken;
std::vector<Token> Parser::tokens;
size_t Parser::currentIndex = 0;

bool Parser::getNextLine(std::string &line)
{
    while (!Context::emptyInputStack()) {
        InputSource *currentSource = Context::topInputStack().get();
        if (currentSource->getNextLine(line)) {
            if (!line.empty()) {
                return true;
            }
        } else {
            // End of current input source
            Context::popInputStack();
        }
    }
    return false;
}

void Parser::init()
{
    tokens.clear();
    currentIndex = 0;
}

void Parser::parse()
{
    std::string line;
    while (getNextLine(line)) {
        tokens = Tokenizer::tokenize(line);
        currentIndex = 0;

        while (currentIndex < tokens.size()) {
            advance();
            if (currentToken.type == TokenType::EndOfFile) {
                return; // End parsing when EOF is reached
            }
            parseLine();
        }
    }
}

void Parser::advance()
{
    if (currentIndex < tokens.size()) {
        currentToken = tokens[currentIndex++];
    } else {
        currentToken = {TokenType::EndOfFile, "", 0, 0, ""};
    }
}

void Parser::parseLine()
{
    if (currentToken.type == TokenType::Identifier) {
        // Possible label or instruction
        if (currentIndex < tokens.size() && tokens[currentIndex].lexeme == ":") {
            // It's a label
            SymbolTable::addSymbol({currentToken.lexeme, Symbol::Type::Label});
            advance();   // Move to the next token after ':'
            advance();   // Move to the next token after the label
            parseLine(); // Parse the rest of the line
        } else {
            // Treat as instruction or directive
            parseInstruction();
        }
    } else if (currentToken.type == TokenType::Directive) {
        parseDirective();
    } else if (currentToken.type == TokenType::Instruction) {
        parseInstruction();
    } else if (currentToken.type == TokenType::EndOfLine) {
        // Empty line; do nothing
    } else if (currentToken.type == TokenType::Comment) {
        // Comment line; do nothing
    } else {
        // Syntax error
        reportError("Unexpected token: " + currentToken.lexeme);
    }
}

void Parser::parseDirective()
{
    if (currentToken.lexeme == "DB" || currentToken.lexeme == "DW" || currentToken.lexeme == "DD") {
        advance();
        parseOperandList();
    } else {
        reportError("Unknown directive: " + currentToken.lexeme);
    }
}

void Parser::parseInstruction()
{
    std::string mnemonic = currentToken.lexeme;
    advance();
    parseOperandList();
    // TODO: Validate instruction operands based on the instruction set
}

void Parser::parseOperandList()
{
    while (currentToken.type != TokenType::EndOfLine && currentToken.type != TokenType::EndOfFile) {
        if (currentToken.type == TokenType::Identifier || currentToken.type == TokenType::Number ||
            currentToken.type == TokenType::Register || currentToken.type == TokenType::StringLiteral) {
            // Valid operand
            advance();
            if (currentToken.lexeme == ",") {
                advance();
            } else if (currentToken.type == TokenType::EndOfLine) {
                break;
            } else {
                reportError("Expected ',' or end of line, found: " + currentToken.lexeme);
                break;
            }
        } else {
            reportError("Invalid operand: " + currentToken.lexeme);
            advance();
        }
    }
}

void Parser::reportError(const std::string &message)
{
    ErrorReporter::reportError(message, currentToken.lineNumber, currentToken.columnNumber, currentToken.lexeme,
                               currentToken.fileName);
}
