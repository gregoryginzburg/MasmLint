#include "symbol_table.h"
#include "token.h"
#include <iostream>

void SymbolTable::addSymbol(const std::shared_ptr<Symbol> &symbol) { symbols[symbol->token.lexeme] = symbol; }

// Returns nullptr when symbol is not found
std::shared_ptr<Symbol> SymbolTable::findSymbol(const Token &token)
{
    auto it = symbols.find(token.lexeme);
    if (it != symbols.end()) {
        return it->second;
    }
    return nullptr;
}

void SymbolTable::printSymbols()
{
    std::cout << "Symbol Table:\n";
    for (const auto &[name, symbol] : symbols) {
        if (auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(symbol)) {
            std::string type = "";
            switch (variableSymbol->type) {
            case VariableSymbol::Type::DataVariable:
                type = "Data Variable";
                break;
            case VariableSymbol::Type::EquVariable:
                type = "EQU Variable";
                break;
            case VariableSymbol::Type::EqualVariable:
                type = "`=` Variable";
                break;
            case VariableSymbol::Type::Label:
                type = "Label Variable";
                break;
            }
            std::cout << "Name: " << name << ", Type: " << type << "\n";
        } else if (auto structSymbol = std::dynamic_pointer_cast<StructSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "STRUC" << "\n";
        } else if (auto procSymbol = std::dynamic_pointer_cast<ProcSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "PROC" << "\n";
        } else if (auto recordSymbol = std::dynamic_pointer_cast<RecordSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "RECORD" << "\n";
        } else if (auto recordFieldSymbol = std::dynamic_pointer_cast<RecordFieldSymbol>(symbol)) {
            std::cout << "Name: " << name << ", Type: " << "Record Field" << "\n";
        }
    }
}
