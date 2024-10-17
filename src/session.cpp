#include "session.h"

#include <iostream>

ParseSession::ParseSession()
{
    sourceMap = std::make_shared<SourceMap>();
    // bool useColor = !isOutputRedirected(std::cout);
    bool useColor = true;
    auto emitter = std::make_shared<Emitter>(sourceMap, std::cout, useColor);
    dcx = std::make_shared<DiagCtxt>(emitter);
    symbolTable = std::make_shared<SymbolTable>();
}
