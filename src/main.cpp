#include "log.h"
#include "preprocessor.h"
#include "symbol_table.h"
#include "parser.h"
#include "session.h"
#include "error_codes.h"
#include "ast.h"
#include "semantic_analyzer.h"
#include "tokenize.h"

#include <iostream>
#include <memory>
#include <fmt/core.h>
#include <filesystem>

#ifdef _WIN32
#    include <windows.h>
#    include <io.h>
#    include <fcntl.h>
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

    std::filesystem::path filename = "examples/test1.asm";
    bool jsonOutput = false;
    bool readFromStdin = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--json") {
            jsonOutput = true;
        } else if (std::string(argv[i]) == "--stdin") {
            readFromStdin = true;
        } else {
            filename = std::string(argv[i]);
        }
    }
    // TODO: Remove in release
    std::filesystem::path new_path = R"(C:\Users\grigo\Documents\MasmLint)";
    std::filesystem::current_path(new_path);

    auto parseSess = std::make_shared<ParseSession>();
    std::shared_ptr<SourceFile> sourceFile;
    if (readFromStdin) {
#ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
#endif
        std::string sourceContent;
        sourceContent.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
        // turn of the hack for json, because vscode can underline EOF
        if (!jsonOutput) {
            sourceContent += "\n"; // hack for not having to underline EOF
        }
        sourceFile = parseSess->sourceMap->newSourceFile(filename, sourceContent);
    } else {
        sourceFile = parseSess->sourceMap->loadFile(filename);
    }

    if (sourceFile) {
        Tokenizer tokenizer(parseSess, sourceFile->getSource());
        std::vector<Token> tokens = tokenizer.tokenize();

        Preprocessor preprocessor(parseSess, tokens);
        std::vector<Token> preprocessedTokens = preprocessor.preprocess();

        Parser parser(parseSess, preprocessedTokens);
        ASTPtr ast = parser.parse();

        SemanticAnalyzer semanticAnalyzer(parseSess, ast);
        // add step for creating a symbol table
        semanticAnalyzer.analyze();

        if (!jsonOutput) {
            printAST(ast, 0);
        }
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
