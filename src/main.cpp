#include "log.h"
#include "input.h"
#include "preprocessor.h"
#include "symbol_table.h"
#include "context.h"
#include "parser.h"
#include <iostream>

int main(int argc, char *argv[])
{
    Log::init();

    std::string filename = "test.asm";

    ErrorReporter::init();
    SymbolTable::init();
    Context::init(filename);

    Parser parser;
    parser.parse();

    // Step 3: Check and display errors
    if (ErrorReporter::hasErrors()) {
        ErrorReporter::displayErrors();
    } else {
        std::cout << "Parsing completed successfully with no errors.\n";
    }

    return 0;
}
