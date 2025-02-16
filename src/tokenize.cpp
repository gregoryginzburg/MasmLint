#include "tokenize.h"
#include "diagnostic.h"
#include "session.h"
#include "log.h"
#include "error_codes.h"

#include <cctype>
#include <algorithm>
#include <unordered_set>
#include <string>

#include <utf8proc.h>

static const std::unordered_set<std::string> directives = {"=",      ".CODE", ".DATA", ".STACK", "DB",     "DW",     "DD",    "DQ",     "ELSE",
                                                           "ELSEIF", "END",   "ENDIF", "ENDM",   "ENDP",   "ENDS",   "EQU",   "FOR",    "FORC",
                                                           "IF",     "IFE",   "IFB",   "IFNB",   "IFDIF",  "IFDIFI", "IFIDN", "IFIDNI", "LOCAL",
                                                           "MACRO",  "PROC",  "STRUC", "RECORD", "REPEAT", "INCLUDE"};

static const std::unordered_set<std::string> reservedWords = {"DUP"};

// delete DUP from here?
static const std::unordered_set<std::string> operators = {"+",    "-",    "*",      "/",      ".",        "MOD",   "SHL",  "SHR",    "PTR",
                                                          "TYPE", "SIZE", "SIZEOF", "LENGTH", "LENGTHOF", "WIDTH", "MASK", "OFFSET", "DUP"};

static const std::unordered_set<std::string> types = {"BYTE", "WORD", "DWORD", "QWORD"};

static const std::unordered_set<std::string> instructions = {
    "ADC",   "ADD",   "AND", "CALL",  "CBW",  "CDQ", "CMP",    "CWD",   "DEC",  "DIV",    "IDIV", "IMUL",   "INC",     "JA",     "JAE",  "JB",
    "JBE",   "JC",    "JE",  "JECXZ", "JG",   "JGE", "JL",     "JLE",   "JMP",  "JNC",    "JNE",  "JNZ",    "JZ",      "LEA",    "LOOP", "MOV",
    "MOVSX", "MOVZX", "MUL", "NEG",   "NOT",  "OR",  "POP",    "POPFD", "PUSH", "PUSHFD", "RCL",  "RCR",    "RET",     "ROL",    "ROR",  "SBB",
    "SHL",   "SHR",   "SUB", "TEST",  "XCHG", "XOR", "INCHAR", "ININT", "EXIT", "OUTI",   "OUTU", "OUTSTR", "OUTCHAR", "NEWLINE"};

static const std::unordered_set<std::string> registers = {"AL", "AX",  "EAX", "BL",  "BX", "EBX", "CL", "CX",  "ECX", "DL",
                                                          "DX", "EDX", "SI",  "ESI", "DI", "EDI", "BP", "EBP", "SP",  "ESP"};

void Tokenizer::advance()
{
    int len = getSymbolLength(pos);
    if (len < 0) {
        addDiagnostic(pos, pos + 1, ErrorCode::INVALID_UTF8_ENCODING);
        pos += 1;
    } else {
        pos += static_cast<uint32_t>(len);
    }
}

int Tokenizer::getSymbolLength(size_t symbolPos) const
{
    const char *str = src.c_str();
    auto len = static_cast<utf8proc_ssize_t>(src.size());
    auto idx = static_cast<utf8proc_ssize_t>(symbolPos);
    utf8proc_int32_t codepoint = 0;

    utf8proc_ssize_t charLen = utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t *>(str + idx), len - idx, &codepoint);

    return static_cast<int>(charLen);
}

std::vector<Token> Tokenizer::tokenize()
{
    size_t length = src.size();

    while (pos < length) {
        skipWhitespace();

        if (pos >= length) {
            break;
        }

        if (src[pos] == '\n') {
            tokens.emplace_back(Token{Token::Type::EndOfLine, "", Span(pos, pos + 1, nullptr)});
            advance();
            continue; // Skip calling getNextToken() after processing '\n'
        }

        Token token = getNextToken();
        // if (token.type == Token::Type::Invalid) {
        //     Stop tokenizing on error
        //     break;
        // }
        if (token.type != Token::Type::Comment) {
            tokens.push_back(token);
        }
    }

    // because files always ends with a '\n', we can make EndOfFile span equal to the last '\n'
    // to be able to underline EndOfFile correctly
    tokens.emplace_back(Token{Token::Type::EndOfFile, "", Span(pos - 1, pos, nullptr)});

    return tokens;
}

void Tokenizer::skipWhitespace()
{
    while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])) && src[pos] != '\n') {
        advance();
    }
}

Token Tokenizer::getNextToken()
{
    char currentChar = src[pos];

    if (isValidNumberStart(currentChar)) {
        return getNumberToken();
    } else if (isValidIdentifierStart(currentChar) || (currentChar == '.' && isDotName())) {
        return getIdentifierOrKeywordToken();
    } else if (currentChar == '"' || currentChar == '\'') {
        return getStringLiteralToken();
    } else if (currentChar == '\\') {
        // Line continuations are not handled; report an error
        size_t errorStart = pos;
        advance();
        addDiagnostic(errorStart, pos, ErrorCode::LINE_CONTINUATION_NOT_SUPPORTED);
        return Token{Token::Type::Invalid, "\\", Span(errorStart, pos, nullptr)};
    } else if (currentChar == ';') {
        size_t commentStart = pos;
        while (pos < src.size() && src[pos] != '\n') {
            advance();
        }
        std::string commentText = src.substr(commentStart, pos - commentStart);
        return Token{Token::Type::Comment, commentText, Span(commentStart, pos, nullptr)};
    } else {
        return getSpecialSymbolToken();
    }
}

bool Tokenizer::isDotName()
{
    if (pos + 1 >= src.size()) {
        return false;
    }

    size_t newPos = pos + 1;
    while (newPos < src.size() && isValidIdentifierChar(src[newPos])) {
        ++newPos;
    }

    std::string lexeme = src.substr(pos, newPos - pos);
    std::string lexemeUpper = stringToUpper(lexeme);
    return directives.contains(lexemeUpper);

    // char nextChar = src[pos + 1];

    // if (!isValidIdentifierChar(nextChar))
    //     return false;

    // // Check previous token
    // if (tokens.empty()) {
    //     // No previous token, accept the dotted name
    //     return true;
    // }

    // Token::Type prevType = tokens.back().type;
    // std::string prevLexeme = tokens.back().lexeme;

    // if (prevType == Token::Type::Register || prevType == Token::Type::Identifier || prevLexeme == ")" ||
    //     prevLexeme == "]") {
    //     // Previous token is a register, identifier, or closing bracket; dot is an operator
    //     return false;
    // }

    // return true;
}

bool Tokenizer::isValidNumberStart(char c) { return isdigit(static_cast<unsigned char>(c)); }

Token Tokenizer::getIdentifierOrKeywordToken()
{
    size_t start = pos;

    // Handle optional starting dot
    if (src[pos] == '.') {
        advance();
    }

    while (pos < src.size() && isValidIdentifierChar(src[pos])) {
        advance();
    }

    std::string lexeme = src.substr(start, pos - start);
    std::string lexemeUpper = stringToUpper(lexeme);

    Span tokenSpan(start, pos, nullptr);

    if (directives.contains(lexemeUpper)) {
        return Token{Token::Type::Directive, lexeme, tokenSpan};
    } else if (instructions.contains(lexemeUpper)) {
        return Token{Token::Type::Instruction, lexeme, tokenSpan};
    } else if (registers.contains(lexemeUpper)) {
        return Token{Token::Type::Register, lexeme, tokenSpan};
    } else if (operators.contains(lexemeUpper)) {
        return Token{Token::Type::Operator, lexeme, tokenSpan};
    } else if (types.contains(lexemeUpper)) {
        return Token{Token::Type::Type, lexeme, tokenSpan};
    } else {
        return Token{Token::Type::Identifier, lexeme, tokenSpan};
    }
}

Token Tokenizer::getNumberToken()
{
    size_t start = pos;
    size_t length = src.size();

    // Collect alphanumeric characters until whitespace or operator
    while (pos < length && std::isalnum(static_cast<unsigned char>(src[pos]))) {
        advance();
    }
    std::string lexeme = src.substr(start, pos - start);

    // Now check if the lexeme is a valid number
    if (isValidNumber(lexeme)) {
        return Token{Token::Type::Number, lexeme, Span(start, pos, nullptr)};
    } else {
        addDiagnostic(start, pos, ErrorCode::INVALID_NUMBER_FORMAT);
        return Token{Token::Type::Invalid, lexeme, Span(start, pos, nullptr)};
    }
}

bool Tokenizer::isValidNumber(const std::string &lexeme)
{
    if (lexeme.empty()) {
        return false;
    }

    size_t len = lexeme.size();
    char suffix = static_cast<char>(tolower(lexeme[len - 1]));
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
        break;
    }

    // check that digits are valid for the base
    if (digits.empty()) {
        return false;
    }
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
    advance(); // skip the opening quote
    while (pos < src.size() && src[pos] != quoteChar) {
        if (src[pos] == '\n') {
            break;
        } else {
            advance();
        }
    }
    if (pos >= src.size() || src[pos] != quoteChar) {
        addDiagnostic(start, pos, ErrorCode::UNTERMINATED_STRING_LITERAL);
        return Token{Token::Type::Invalid, src.substr(start, pos - start), Span(start, pos, nullptr)};
    }
    advance(); // Skip the closing quote
    std::string lexeme = src.substr(start, pos - start);
    return Token{Token::Type::StringLiteral, lexeme, Span(start, pos, nullptr)};
}

Token Tokenizer::getSpecialSymbolToken()
{
    size_t start = pos;
    char currentChar = src[pos];
    advance();

    std::string lexeme = src.substr(start, pos - start);
    Token::Type type = Token::Type::Operator;

    switch (currentChar) {
    case '(':
        type = Token::Type::OpenBracket;
        break;
    case ')':
        type = Token::Type::CloseBracket;
        break;
    case '[':
        type = Token::Type::OpenSquareBracket;
        break;
    case ']':
        type = Token::Type::CloseSquareBracket;
        break;
    case ',':
        type = Token::Type::Comma;
        break;
    case ':':
        type = Token::Type::Colon;
        break;
    case '+':
    case '-':
    case '*':
    case '/':
    case '.':
        type = Token::Type::Operator;
        break;
    case '=':
        type = Token::Type::Directive;
        break;
    case '<':
        type = Token::Type::OpenAngleBracket;
        break;
    case '>':
        type = Token::Type::CloseAngleBracket;
        break;
    case '?':
        type = Token::Type::QuestionMark;
        break;
    case '$':
        type = Token::Type::Dollar;
        break;
    default:
        addDiagnostic(start, pos, ErrorCode::UNKNOWN_CHARACTER);
        return Token{Token::Type::Invalid, lexeme, Span(start, pos, nullptr)};
    }

    return Token{type, lexeme, Span(start, pos, nullptr)};
}

bool Tokenizer::isValidIdentifierStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '@' || c == '$' || c == '?'; }

bool Tokenizer::isValidIdentifierChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '@' || c == '$' || c == '?'; }

bool Tokenizer::isValidIdentifier(const std::string &lexeme)
{
    if (lexeme.empty()) {
        return false;
    }

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