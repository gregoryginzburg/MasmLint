#define DOCTEST_CONFIG_COLORS_ANSI
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "add.h"
#include "tokenize.h"
#include "session.h"
#include "diagnostic.h"
#include <memory>

// int add(int a, int b) { return a + b; }

TEST_CASE("testing the add function") {
    CHECK(add(1, 2) == 3);
    CHECK(add(2, 5) == 7);
}


TEST_CASE("Tokenizer: Identifiers and Keywords") {
    auto parseSess = std::make_shared<ParseSession>();

    SUBCASE("Valid Identifier") {
        std::string source = "myVar";
        Tokenizer tokenizer(parseSess, source, 0);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens.size() == 2); // Identifier + EndOfFile
        CHECK(tokens[0].type == TokenType::Identifier);
        CHECK(tokens[0].lexeme == "myVar");
    }

    SUBCASE("Directive Keyword") {
        std::string source = "EQU";
        Tokenizer tokenizer(parseSess, source, 0);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == TokenType::Directive);
        CHECK(tokens[0].lexeme == "EQU");
    }

    SUBCASE("Instruction Keyword") {
        std::string source = "mov";
        Tokenizer tokenizer(parseSess, source, 0);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == TokenType::Instruction);
        CHECK(tokens[0].lexeme == "mov");
    }

    SUBCASE("Register Keyword") {
        std::string source = "AX";
        Tokenizer tokenizer(parseSess, source, 0);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == TokenType::Register);
        CHECK(tokens[0].lexeme == "AX");
    }

    SUBCASE("Identifier Starting with Dot") {
        std::string source = ".myLabel";
        Tokenizer tokenizer(parseSess, source, 0);
        auto tokens = tokenizer.tokenize();

        CHECK(tokens[0].type == TokenType::Identifier);
        CHECK(tokens[0].lexeme == ".myLabel");
    }
}

