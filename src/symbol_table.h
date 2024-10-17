#pragma once

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
    void addSymbol(const Symbol &symbol);
    Symbol *findSymbol(const std::string &name);

private:
    std::unordered_map<std::string, Symbol> symbols;
};
