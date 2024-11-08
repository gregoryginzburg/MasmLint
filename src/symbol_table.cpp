#include "symbol_table.h"

void SymbolTable::addSymbol(const Symbol &symbol) { symbols[symbol.name] = symbol; }

Symbol *SymbolTable::findSymbol(const std::string &name)
{
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return &(it->second);
    }
    return nullptr;
}
