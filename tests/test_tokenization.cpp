#include "tokenize.h"
#include "session.h"
#include "diagnostic.h"

#include <memory>

#include <doctest/doctest.h>

TEST_CASE("Tokenizer: Identifiers and Keywords")
{

    SUBCASE("Valid Identifier")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "myVar";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens.size() == 2); // Identifier + EndOfFile
        CHECK(tokens[0].type == Token::Type::Identifier);
        CHECK(tokens[0].lexeme == "myVar");
    }

    SUBCASE("Directive Keyword")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "EQU";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Directive);
        CHECK(tokens[0].lexeme == "EQU");
    }

    SUBCASE("Instruction Keyword")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "mov";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Instruction);
        CHECK(tokens[0].lexeme == "mov");
    }

    SUBCASE("Register Keyword")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "AX";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Register);
        CHECK(tokens[0].lexeme == "AX");
    }

    SUBCASE("Identifier Starting with Dot")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = ".myLabel";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Operator);
        CHECK(tokens[0].lexeme == ".");

        CHECK(tokens[1].type == Token::Type::Identifier);
        CHECK(tokens[1].lexeme == "myLabel");
    }
}

TEST_CASE("Tokenizer: Numbers")
{

    SUBCASE("Decimal Number")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "12345";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Number);
        CHECK(tokens[0].lexeme == "12345");
    }

    SUBCASE("Hexadecimal Number with 'h' Suffix")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "0FFh";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Number);
        CHECK(tokens[0].lexeme == "0FFh");
    }

    SUBCASE("Hexadecimal Number with 'h' Suffix that deosn't start with a number")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "FFh";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Identifier);
        CHECK(tokens[0].lexeme == "FFh");
    }

    SUBCASE("Binary Number with 'b' Suffix")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "1010b";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Number);
        CHECK(tokens[0].lexeme == "1010b");
    }

    SUBCASE("Octal Number with 'o' Suffix")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "77o";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Number);
        CHECK(tokens[0].lexeme == "77o");
    }

    SUBCASE("Invalid Number Format")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "123XYZ";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Invalid);
        CHECK(parseSess->dcx->hasErrors());
    }

    SUBCASE("Invalid Number Format")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "123b";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Invalid);
        CHECK(parseSess->dcx->hasErrors());
    }
}

TEST_CASE("Tokenizer: Strings")
{

    SUBCASE("Double-Quoted String")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "\"Hello, World!\"";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::StringLiteral);
        CHECK(tokens[0].lexeme == "\"Hello, World!\"");
    }

    SUBCASE("Single-Quoted String")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "'Hello, World!'";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::StringLiteral);
        CHECK(tokens[0].lexeme == "'Hello, World!'");
    }

    SUBCASE("Unterminated String")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "\"This string is not closed";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Invalid);
        CHECK(parseSess->dcx->hasErrors());
    }
}

TEST_CASE("Tokenizer: Comments")
{

    SUBCASE("Single-Line Comment")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "; This is a comment";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        // comments are omitted
        CHECK_EQ(tokens.size(), 1);
        CHECK_EQ(tokens[0].type, Token::Type::EndOfFile);
    }

    SUBCASE("Code with Comment")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "MOV AX, BX ; Move BX into AX";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens.size() == 5); // `MOV`, `AX`, `,`, `BX`, `EndOfFile`
        CHECK(tokens[0].type == Token::Type::Instruction);
        CHECK(tokens[0].lexeme == "MOV");

        CHECK_EQ(tokens[1].type, Token::Type::Register);
        CHECK_EQ(tokens[1].lexeme, "AX");

        CHECK_EQ(tokens[2].type, Token::Type::Comma);
        CHECK_EQ(tokens[2].lexeme, ",");

        CHECK_EQ(tokens[3].type, Token::Type::Register);
        CHECK_EQ(tokens[3].lexeme, "BX");

        CHECK_EQ(tokens[4].type, Token::Type::EndOfFile);
        CHECK_EQ(tokens[4].lexeme, "");
    }
}

TEST_CASE("Tokenizer: Operators and Special Symbols")
{

    SUBCASE("Single-Character Operators")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "+ - * / . MOD SHL SHR PTR TYPE SIZE SIZEOF LENGTH LENGTHOF WIDTH MASK OFFSET DUP";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens.size() == 19);
        CHECK(tokens[0].lexeme == "+");
        CHECK(tokens[1].lexeme == "-");
        CHECK(tokens[2].lexeme == "*");
        CHECK(tokens[3].lexeme == "/");
        CHECK_EQ(tokens[4].lexeme, ".");
    }
}

TEST_CASE("Tokenizer: Error Handling")
{

    SUBCASE("Unrecognized Character")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "# \\ Д";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Invalid);
        CHECK_EQ(tokens[1].type, Token::Type::Invalid);
        CHECK_EQ(tokens[2].type, Token::Type::Invalid);
        CHECK(parseSess->dcx->hasErrors());
    }

    SUBCASE("Recovery After Error")
    {
        auto parseSess = std::make_shared<ParseSession>();
        std::string source = "# MOV AX, BX";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        // Should report error but continue tokenizing
        CHECK(tokens[0].type == Token::Type::Invalid);
        CHECK(tokens[1].type == Token::Type::Instruction);
        CHECK(tokens[1].lexeme == "MOV");
        CHECK(parseSess->dcx->hasErrors());
    }
}