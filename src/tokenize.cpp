#include "tokenize.h"
#include "diagnostic.h"
#include "session.h"
#include "log.h"
#include "error_codes.h"

#include <cctype>
#include <algorithm>
#include <unordered_set>
#include <string>

static const std::unordered_set<std::string> directives = {
    "INCLUDE", "EQU", "DB",   "DW",     "DD",  "DQ",   "END",  ".STACK", ".DATA", ".CODE", "PROC",  "ENDP",
    "STRUC",   "ENDS", "RECORD", "PTR", "TYPE", "SIZE", "LENGTH", "WIDTH", "MASK",  "OFFSET"};

static const std::unordered_set<std::string> instructions = {
    "MOV",   "XCHG",  "MOVZX", "MOVSX", "DIV",   "IDIV",  "MUL",   "IMUL",  "ADD",   "ADC",   "INC",   "SUB",
    "SBB",   "DEC",   "NEG",   "JE",    "JNE",   "JA",    "JAE",   "JB",    "JBE",   "JL",    "JLE",   "JG",
    "JGE",   "JC",    "JNC",   "JZ",    "JNZ",   "JMP",   "CALL",  "RET",   "SHL",   "SHR",   "ROL",   "RCL",
    "ROR",   "RCR",   "AND",   "OR",    "XOR",   "REP",   "REPE",  "REPNE", "MOVSB", "MOVSW", "MOVSD", "LODSB",
    "LODSW", "LODSD", "STOSB", "STOSW", "STOSD", "SCASB", "SCASW", "SCASD", "CMPSB", "CMPSW", "CMPSD",
};

static const std::unordered_set<std::string> registers = {"AL", "AX",  "EAX", "BL",  "BX",  "EBX", "CL",
                                                          "CX", "ECX", "DL",  "DX",  "EDX", "SI",  "ESI",
                                                          "DI", "EDI", "BP",  "EBP", "SP",  "ESP"};

std::vector<Token> Tokenizer::tokenize()
{
    size_t length = src.size();
    std::shared_ptr<SyntaxContextData> context = std::make_shared<SyntaxContextData>();

    while (pos < length) {
        skipWhitespace();

        if (pos >= length)
            break;

        Token token = getNextToken();
        // if (token.type == TokenType::Invalid) {
        //     // Stop tokenizing on error
        //     break;
        // }
        tokens.push_back(token);

        if (pos < length && (src[pos] == '\n' || src[pos] == '\r')) {
            tokens.emplace_back(Token{TokenType::EndOfLine, "", Span(pos, pos + 1, context)});
            ++pos;
        }
    }

    tokens.emplace_back(Token{TokenType::EndOfFile, "", Span(pos, pos, context)});

    return tokens;
}

void Tokenizer::skipWhitespace()
{
    while (pos < src.size() && isspace(src[pos])) {
        ++pos;
    }
}

Token Tokenizer::getNextToken()
{
    char currentChar = src[pos];

    if (isValidNumberStart(currentChar)) {
        return getNumberToken();
    } else if (isValidIdentifierStart(currentChar)) {
        return getIdentifierOrKeywordToken();
    } else if (currentChar == '.' && isDotName()) {
        return getIdentifierOrKeywordToken();
    } else if (currentChar == '"' || currentChar == '\'' || currentChar == '<' || currentChar == '{') {
        return getStringLiteralToken();
    } else if (currentChar == '\\') {
        // Line continuations are not handled; report an error
        size_t errorStart = pos;
        ++pos;
        addDiagnostic(errorStart, pos, ErrorCode::LINE_CONTINUATION_NOT_SUPPORTED);
        return Token{TokenType::Invalid, "\\", Span(errorStart, pos, nullptr)};
    } else if (currentChar == ';') {
        size_t commentStart = pos;
        while (pos < src.size() && src[pos] != '\n') {
            ++pos;
        }
        std::string commentText = src.substr(commentStart, pos - commentStart);
        return Token{TokenType::Comment, commentText, Span(commentStart, pos, nullptr)};
    } else {
        return getSpecialSymbolToken();
    }
}

bool Tokenizer::isDotName()
{
    if (pos + 1 >= src.size())
        return false;
    char nextChar = src[pos + 1];

    if (!isValidIdentifierChar(nextChar))
        return false;

    // Check previous token
    if (tokens.empty()) {
        // No previous token, accept the dotted name
        return true;
    }

    TokenType prevType = tokens.back().type;
    std::string prevLexeme = tokens.back().lexeme;

    if (prevType == TokenType::Register || prevType == TokenType::Identifier || prevLexeme == ")" ||
        prevLexeme == "]") {
        // Previous token is a register, identifier, or closing bracket; dot is an operator
        return false;
    }

    return true;
}

bool Tokenizer::isValidNumberStart(char c)
{
    return isdigit(static_cast<unsigned char>(c)) || (tolower(c) >= 'a' && tolower(c) <= 'f');
}

Token Tokenizer::getIdentifierOrKeywordToken()
{
    size_t start = pos;

    // Handle optional starting dot
    if (src[pos] == '.') {
        ++pos;
    }

    while (pos < src.size() && isValidIdentifierChar(src[pos])) {
        ++pos;
    }

    std::string lexeme = src.substr(start, pos - start);
    std::string lexemeUpper = lexeme;
    std::transform(lexemeUpper.begin(), lexemeUpper.end(), lexemeUpper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    Span tokenSpan(start, pos, nullptr);

    if (directives.count(lexemeUpper)) {
        return Token{TokenType::Directive, lexeme, tokenSpan};
    } else if (instructions.count(lexemeUpper)) {
        return Token{TokenType::Instruction, lexeme, tokenSpan};
    } else if (registers.count(lexemeUpper)) {
        return Token{TokenType::Register, lexeme, tokenSpan};
    } else {
        return Token{TokenType::Identifier, lexeme, tokenSpan};
    }
}

Token Tokenizer::getNumberToken()
{
    size_t start = pos;
    size_t length = src.size();

    // Collect alphanumeric characters until whitespace or operator
    while (pos < length && isalnum(static_cast<unsigned char>(src[pos]))) {
        ++pos;
    }
    std::string lexeme = src.substr(start, pos - start);

    // Now check if the lexeme is a valid number
    if (isValidNumber(lexeme)) {
        return Token{TokenType::Number, lexeme, Span(start, pos, nullptr)};
    } else {
        // check in cases like fffrh
        if (isValidIdentifier(lexeme)) {
            pos = start;
            return getIdentifierOrKeywordToken();
        }
        addDiagnostic(start, pos, ErrorCode::INVALID_NUMBER_FORMAT);
        return Token{TokenType::Invalid, lexeme, Span(start, pos, nullptr)};
    }
}

bool Tokenizer::isValidNumber(const std::string &lexeme)
{
    if (lexeme.empty()) {
        return false;
    }

    size_t len = lexeme.size();
    char suffix = tolower(lexeme[len - 1]);
    std::string digits = lexeme.substr(0, len - 1);
    unsigned int base = 10;

    // Determine base from suffix
    switch (suffix) {
    case 'h':
        base = 16;
        break;
    case 'b':
    case 'y':
        base = 2;
        break;
    case 'o':
    case 'q':
        base = 8;
        break;
    case 'd':
    case 't':
        base = 10;
        break;
    default:
        // No valid suffix; include the last character
        digits = lexeme;
        suffix = '\0';
        break;
    }

    // check that digits are valid for the base
    for (char c : digits) {
        c = static_cast<char>(tolower(c));
        if (base == 16) {
            if (!isxdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        } else if (base == 10) {
            if (!isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        } else if (base == 8) {
            if (c < '0' || c > '7') {
                return false;
            }
        } else if (base == 2) {
            if (c != '0' && c != '1') {
                return false;
            }
        } else {
            return false; // Invalid base
        }
    }

    return true;
}

Token Tokenizer::getStringLiteralToken()
{
    char quoteChar = src[pos];
    size_t start = pos;
    ++pos; // skip the opening quote
    while (pos < src.size() && src[pos] != quoteChar) {
        if (src[pos] == '\n') {
            break;
        } else {
            ++pos;
        }
    }
    if (pos >= src.size() || src[pos] != quoteChar) {
        addDiagnostic(start, pos, ErrorCode::UNTERMINATED_STRING_LITERAL);
        return Token{TokenType::Invalid, src.substr(start, pos - start), Span(start, pos, nullptr)};
    }
    ++pos; // Skip the closing quote
    std::string lexeme = src.substr(start, pos - start);
    return Token{TokenType::StringLiteral, lexeme, Span(start, pos, nullptr)};
}

Token Tokenizer::getSpecialSymbolToken()
{
    size_t start = pos;
    char currentChar = src[pos];
    ++pos;

    std::string lexeme(1, currentChar);
    TokenType type = TokenType::Operator;

    switch (currentChar) {
    case '(':
        type = TokenType::OpenBracket;
        break;
    case ')':
        type = TokenType::CloseBracket;
        break;
    case '[':
        type = TokenType::OpenSquareBracket;
        break;
    case ']':
        type = TokenType::CloseSquareBracket;
        break;
    case ',':
        type = TokenType::Comma;
        break;
    case ':':
        type = TokenType::Colon;
        break;
    case '.':
        type = TokenType::Dot;
        break;
    case '%':
        type = TokenType::Percent;
        break;
    case '+':
    case '-':
    case '*':
    case '/':
    case '=':
    case '<':
    case '>':
    case '!':
    case '&':
    case '|':
    case '^':
    case '~':
        break;
    default:
        addDiagnostic(start, pos, ErrorCode::UNRECOGNIZED_SYMBOL);
        return Token{TokenType::Invalid, lexeme, Span(start, pos, nullptr)};
    }

    return Token{type, lexeme, Span(start, pos, nullptr)};
}

bool Tokenizer::isValidIdentifierStart(char c) { return isalpha(c) || c == '_' || c == '@' || c == '$' || c == '?'; }

bool Tokenizer::isValidIdentifierChar(char c) { return isalnum(c) || c == '_' || c == '@' || c == '$' || c == '?'; }

bool Tokenizer::isValidIdentifier(const std::string &lexeme)
{
    if (lexeme.empty())
        return false;

    if (!isValidIdentifierStart(lexeme[0])) {
        return false;
    }

    for (size_t i = 1; i < lexeme.size(); ++i) {
        if (!isValidIdentifierChar(lexeme[i])) {
            return false;
        }
    }

    return true;
}