#pragma once

#include "error_reporter.h"
#include <string>
#include <unordered_map>

struct Symbol {
    std::string name;
    enum class Type { Label, Variable, Macro, Segment };
    Type type;
    int lineNumber;
    std::string fileName;
};

class SymbolTable {
public:
    static void init();
    static void addSymbol(const Symbol &symbol);
    static Symbol *findSymbol(const std::string &name);

private:
    static std::unordered_map<std::string, Symbol> symbols;
};
