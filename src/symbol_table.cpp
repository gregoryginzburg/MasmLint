#include "symbol_table.h"
#include "token.h"

void SymbolTable::addSymbol(const Symbol &symbol) { symbols[symbol.token.lexeme] = symbol; }

std::unique_ptr<Symbol> SymbolTable::findSymbol(const std::string &name)
{
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return std::unique_ptr<Symbol>(&(it->second));
    }
    return nullptr;
}
