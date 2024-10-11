#include "symbol_table.h"

static std::unordered_map<std::string, Symbol> symbols;

void SymbolTable::init() {}

void SymbolTable::addSymbol(const Symbol &symbol)
{
    if (symbols.find(symbol.name) == symbols.end()) {
        symbols[symbol.name] = symbol;
    } else {
        const Symbol &existingSymbol = symbols[symbol.name];
        ErrorReporter::reportError("Symbol redefinition: " + symbol.name + " (Previously defined in " +
                                      existingSymbol.fileName + " at line " +
                                      std::to_string(existingSymbol.lineNumber) + ")",
                                  symbol.lineNumber, 0, "no context", symbol.fileName);
    }
}

Symbol *SymbolTable::findSymbol(const std::string &name)
{
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return &(it->second);
    }
    return nullptr;
}
