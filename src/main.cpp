#include "log.h"
#include "input.h"
#include "preprocessor.h"
#include "symbol_table.h"
#include "parser.h"
#include "session.h"

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
    // Set output mode to handle UTF-8 text properly
    // _setmode(_fileno(stdout), _O_U8TEXT);
#endif
}

int main(/* int argc, char *argv[]*/)
{
    setupConsoleForUtf8();
    Log::init();

    std::filesystem::path filename = "examples/test.asm";

    auto parseSess = std::make_shared<ParseSession>();
    auto sourceFile = parseSess->sourceMap->loadFile(filename);

    auto tokenizer = Tokenizer(parseSess, sourceFile->getSource(), 0);
    auto preprocessor = Preprocessor(parseSess);

    std::vector<Token> tokens = tokenizer.tokenize();
    tokens = preprocessor.preprocess(tokens);
    Parser parser(parseSess, tokens);
    parser.parse();

    // TESTING
    std::string errMsg = "hey";

    Diagnostic diag(Diagnostic::Level::Error, errMsg);
    std::string test_string = sourceFile->getSource();
    int idx = 0;
    while (idx < test_string.size()) {
        if (test_string[idx] == 'o') {
            break;
        }
        idx++;
    }
    diag.addLabel(Span(idx, idx + 1, nullptr), "Unexpected token here");
    // diag.addLabel(Span(idx+1, idx + 2, nullptr), "Unexpected token here");

    parseSess->dcx->addDiagnostic(diag);

    if (parseSess->dcx->hasErrors()) {
        parseSess->dcx->emitDiagnostics();
    } else {
        fmt::print("Parsing completed successfully with no errors.\n");
    }
    return 0;
}
