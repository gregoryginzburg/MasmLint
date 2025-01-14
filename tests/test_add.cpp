#include "add.h"
#include "tokenize.h"
#include "session.h"
#include "diagnostic.h"
#include <memory>

#define DOCTEST_CONFIG_COLORS_ANSI
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// int add(int a, int b) { return a + b; }

TEST_CASE("testing the add function")
{
    CHECK(add(1, 2) == 3);
    CHECK(add(2, 5) == 7);
}

TEST_CASE("Tokenizer: Identifiers and Keywords")
{
    auto parseSess = std::make_shared<ParseSession>();

    SUBCASE("Valid Identifier")
    {
        std::string source = "myVar";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens.size() == 2); // Identifier + EndOfFile
        CHECK(tokens[0].type == Token::Type::Identifier);
        CHECK(tokens[0].lexeme == "myVar");
    }

    SUBCASE("Directive Keyword")
    {
        std::string source = "EQU";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Directive);
        CHECK(tokens[0].lexeme == "EQU");
    }

    SUBCASE("Instruction Keyword")
    {
        std::string source = "mov";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Instruction);
        CHECK(tokens[0].lexeme == "mov");
    }

    SUBCASE("Register Keyword")
    {
        std::string source = "AX";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Register);
        CHECK(tokens[0].lexeme == "AX");
    }

    SUBCASE("Identifier Starting with Dot")
    {
        std::string source = ".myLabel";
        Tokenizer tokenizer(parseSess, source);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == Token::Type::Identifier);
        CHECK(tokens[0].lexeme == ".myLabel");
    }
}

// TEST_CASE("Tokenizer: Numbers") {
//     auto parseSess = std::make_shared<ParseSession>();

//     SUBCASE("Decimal Number") {
//         std::string source = "12345";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Number);
//         CHECK(tokens[0].lexeme == "12345");
//     }

//     SUBCASE("Hexadecimal Number with 'h' Suffix") {
//         std::string source = "0FFh";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Number);
//         CHECK(tokens[0].lexeme == "0FFh");
//     }

//     SUBCASE("Binary Number with 'b' Suffix") {
//         std::string source = "1010b";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Number);
//         CHECK(tokens[0].lexeme == "1010b");
//     }

//     SUBCASE("Octal Number with 'o' Suffix") {
//         std::string source = "77o";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Number);
//         CHECK(tokens[0].lexeme == "77o");
//     }

//     SUBCASE("Floating Point Number") {
//         std::string source = "3.14";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Number);
//         CHECK(tokens[0].lexeme == "3.14");
//     }

//     SUBCASE("Invalid Number Format") {
//         std::string source = "123XYZ";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Invalid);
//         CHECK(parseSess->dcx->hasErrors());
//     }
// }

// TEST_CASE("Tokenizer: Strings") {
//     auto parseSess = std::make_shared<ParseSession>();

//     SUBCASE("Double-Quoted String") {
//         std::string source = "\"Hello, World!\"";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::StringLiteral);
//         CHECK(tokens[0].lexeme == "\"Hello, World!\"");
//     }

//     SUBCASE("Single-Quoted String") {
//         std::string source = "'Hello, MASM'";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::StringLiteral);
//         CHECK(tokens[0].lexeme == "'Hello, MASM'");
//     }

//     SUBCASE("String with Escaped Quote") {
//         std::string source = "\"She said, \\\"Hello\\\"\"";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::StringLiteral);
//         CHECK(tokens[0].lexeme == "\"She said, \\\"Hello\\\"\"");
//     }

//     SUBCASE("Unterminated String") {
//         std::string source = "\"This string is not closed";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Invalid);
//         CHECK(parseSess->dcx->hasErrors());
//     }
// }

// TEST_CASE("Tokenizer: Comments") {
//     auto parseSess = std::make_shared<ParseSession>();

//     SUBCASE("Single-Line Comment") {
//         std::string source = "; This is a comment";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Comment);
//         CHECK(tokens[0].lexeme == "; This is a comment");
//     }

//     SUBCASE("Code with Comment") {
//         std::string source = "MOV AX, BX ; Move BX into AX";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens.size() == 7); // MOV, AX, ,, BX, Comment, EndOfFile
//         CHECK(tokens[0].type == Token::Type::Instruction);
//         CHECK(tokens[0].lexeme == "MOV");
//         CHECK(tokens[5].type == Token::Type::Comment);
//         CHECK(tokens[5].lexeme == "; Move BX into AX");
//     }
// }

// TEST_CASE("Tokenizer: Operators and Special Symbols") {
//     auto parseSess = std::make_shared<ParseSession>();

//     SUBCASE("Single-Character Operators") {
//         std::string source = "+ - * / % = < > & | ^ ~";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens.size() == 14); // 12 operators + EndOfFile
//         CHECK(tokens[0].lexeme == "+");
//         CHECK(tokens[2].lexeme == "-");
//         CHECK(tokens[4].lexeme == "*");
//         CHECK(tokens[6].lexeme == "/");
//         // ... continue checking other operators
//     }

//     SUBCASE("Multi-Character Operators") {
//         std::string source = "== != <= >= && || ::";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens.size() == 8); // 7 operators + EndOfFile
//         CHECK(tokens[0].lexeme == "==");
//         CHECK(tokens[1].lexeme == "!=");
//         CHECK(tokens[2].lexeme == "<=");
//         CHECK(tokens[3].lexeme == ">=");
//         CHECK(tokens[4].lexeme == "&&");
//         CHECK(tokens[5].lexeme == "||");
//         CHECK(tokens[6].lexeme == "::");
//     }

//     SUBCASE("Unrecognized Symbol") {
//         std::string source = "@";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         // '@' can be part of identifiers in MASM, so it should be recognized
//         CHECK(tokens[0].type == Token::Type::Identifier);
//         CHECK(tokens[0].lexeme == "@");
//     }
// }

// TEST_CASE("Tokenizer: Line Continuations") {
//     auto parseSess = std::make_shared<ParseSession>();

//     SUBCASE("Simple Line Continuation") {
//         std::string source = "MOV AX, \\\nBX";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens.size() == 6); // MOV, AX, ,, BX, EndOfFile
//         CHECK(tokens[0].type == Token::Type::Instruction);
//         CHECK(tokens[0].lexeme == "MOV");
//         CHECK(tokens[3].lexeme == "BX");
//     }

//     SUBCASE("Continuation with Comments") {
//         std::string source = "MOV AX, \\ ; continue\nBX";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens.size() == 7); // MOV, AX, ,, BX, EndOfFile
//         CHECK(tokens[0].type == Token::Type::Instruction);
//         CHECK(tokens[0].lexeme == "MOV");
//         CHECK(tokens[5].type == Token::Type::Identifier);
//         CHECK(tokens[5].lexeme == "BX");
//     }
// }

// TEST_CASE("Tokenizer: Error Handling") {
//     auto parseSess = std::make_shared<ParseSession>();

//     SUBCASE("Unrecognized Character") {
//         std::string source = "#";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         CHECK(tokens[0].type == Token::Type::Invalid);
//         CHECK(parseSess->dcx->hasErrors());
//     }

//     SUBCASE("Recovery After Error") {
//         std::string source = "# MOV AX, BX";
//         Tokenizer tokenizer(parseSess, source);
//         auto tokens = tokenizer.tokenize();

//         // Should report error but continue tokenizing
//         CHECK(tokens[0].type == Token::Type::Invalid);
//         CHECK(tokens[1].type == Token::Type::Instruction);
//         CHECK(tokens[1].lexeme == "MOV");
//         CHECK(parseSess->dcx->hasErrors());
//     }
// }
