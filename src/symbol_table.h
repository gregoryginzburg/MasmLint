#pragma once

#include <string>
#include <unordered_map>
#include <memory>

struct Symbol {
    std::string name;
    enum class Type : std::uint8_t { Label, Variable, Macro, Segment };
    Type type{};
    int lineNumber = -1;
    std::string fileName;
};

class SymbolTable {
public:
    void addSymbol(const Symbol &symbol);
    std::unique_ptr<Symbol> findSymbol(const std::string &name);

private:
    std::unordered_map<std::string, Symbol> symbols;
};
