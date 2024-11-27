#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "token.h"

struct Symbol {
    Token token;
    enum class Type : std::uint8_t { Label, Variable, Macro, Segment };
    Type type{};
};

class SymbolTable {
public:
    void addSymbol(const Symbol &symbol);
    std::unique_ptr<Symbol> findSymbol(const std::string &name);

private:
    std::unordered_map<std::string, Symbol> symbols;
};


