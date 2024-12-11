#include "symbol_table.h"
#include "token.h"
#include <iostream>

void SymbolTable::addSymbol(const std::shared_ptr<Symbol> &symbol) { symbols[symbol->token.lexeme] = symbol; }

void SymbolTable::removeSymbol(const std::shared_ptr<Symbol> &symbol) { symbols.erase(symbol->token.lexeme); }

// Returns nullptr when symbol is not found
std::shared_ptr<Symbol> SymbolTable::findSymbol(const Token &token)
{
    auto it = symbols.find(token.lexeme);
    if (it != symbols.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Symbol> SymbolTable::findSymbol(const std::string &name)
{
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second;
    }
    return nullptr;
}

void SymbolTable::printSymbols()
{
    std::cout << "Symbol Table:\n";
    for (const auto &[name, symbol] : symbols) {
        if (auto dataVariableSymbol = std::dynamic_pointer_cast<DataVariableSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "Data Variable" << "\n";
        } else if (auto equVariableSymbol = std::dynamic_pointer_cast<EquVariableSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "EQU Variable" << "\n";
        } else if (auto equalVariableSymbol = std::dynamic_pointer_cast<EqualVariableSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "`=` Variable" << "\n";
        } else if (auto labelSymbol = std::dynamic_pointer_cast<LabelSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "Label Variable" << "\n";
        } else if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "STRUC" << "\n";
            for (const auto &[field, variable] : structSymbol->namedFields) {
                std::cout << "  Field name: " << field << "\n";
            }
        } else if (auto procSymbol = std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "PROC" << "\n";
        } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "RECORD" << "\n";
        } else if (auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "Record Field" << "\n";
        }
    }
}
