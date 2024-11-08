#pragma once

#include <memory>
#include <string>
#include <filesystem>

#include "symbol_table.h"
#include "source_map.h"
#include "diag_ctxt.h"

class ParseSession {
public:
    ParseSession();

public:
    std::shared_ptr<DiagCtxt> dcx;
    std::shared_ptr<SourceMap> sourceMap;
    std::shared_ptr<SymbolTable> symbolTable;
};