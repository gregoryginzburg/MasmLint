#include "log.h"
#include "input.h"
#include "preprocessor.h"
#include "symbol_table.h"
#include "context.h"
#include "parser.h"

#include <iostream>
#include <fmt/core.h>

int main(int argc, char *argv[])
{
    Log::init();

    fmt::print("Hello, world!\n");

    std::string filename = "examples/test.asm";

    ErrorReporter::init();
    SymbolTable::init();
    Context::init(filename);

    Parser parser;
    parser.parse();

    if (ErrorReporter::hasErrors()) {
        ErrorReporter::displayErrors();
    } else {
        std::cout << "Parsing completed successfully with no errors.\n";
    }

    return 0;
}
