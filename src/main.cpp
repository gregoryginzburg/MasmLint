#include "log.h"
#include "preprocessor.h"
#include "symbol_table.h"
#include "parser.h"
#include "session.h"
#include "error_codes.h"
#include "ast.h"
#include "semantic_analyzer.h"

#include <iostream>
#include <memory>
#include <fmt/core.h>
#include <filesystem>

#ifdef _WIN32
#    include <windows.h>
#endif

void setupConsoleForUtf8()
{
#ifdef _WIN32
    // Set console output code page to UTF-8 on Windows
    SetConsoleOutputCP(CP_UTF8);
#endif
}

int main(int argc, char *argv[])
{
    setupConsoleForUtf8();

    bool jsonOutput = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--json") {
            jsonOutput = true;
            break;
        }
    }
    // TODO: Remove in release
    std::filesystem::path new_path = "C:\\Users\\grigo\\Documents\\MasmLint";
    std::filesystem::current_path(new_path);

    std::filesystem::path filename = "examples/test1.asm";
    if (argc > 2) {
        filename = argv[2];
    }

    auto parseSess = std::make_shared<ParseSession>();
    auto sourceFile = parseSess->sourceMap->loadFile(filename);

    if (sourceFile) {
        Tokenizer tokenizer(parseSess, sourceFile->getSource());
        std::vector<Token> tokens = tokenizer.tokenize();

        Preprocessor preprocessor(parseSess, tokens);
        std::vector<Token> preprocessedTokens = preprocessor.preprocess();

        Parser parser(parseSess, preprocessedTokens);
        ASTPtr ast = parser.parse();

        SemanticAnalyzer semanticAnalyzer(parseSess, ast);
        semanticAnalyzer.analyze();

        printAST(ast, 0);
    } else {
        Diagnostic diag(Diagnostic::Level::Error, ErrorCode::FAILED_TO_OPEN_FILE, filename.string());
        parseSess->dcx->addDiagnostic(diag);
    }

    if (parseSess->dcx->hasErrors()) {
        if (jsonOutput) {
            parseSess->dcx->emitJsonDiagnostics();

        } else {
            parseSess->dcx->emitDiagnostics();
        }
    } else {
        if (jsonOutput) {
            fmt::print("[]");
        } else {
            fmt::print("Parsing completed successfully with no errors.\n");
        }
    }
    return 0;
}
