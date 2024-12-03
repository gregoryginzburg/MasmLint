#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "token.h"

struct Symbol {
    Token token;
};

struct VariableSymbol : public Symbol {
    enum class Type : std::uint8_t { Label, DataVariable, EqualVariable, EquVariable };
    Type type;
    std::string dataType;
    uint32_t value;
};

struct StructureSymbol : public Symbol {
    uint32_t size;
    uint32_t numMembers;
    struct Member {
        std::string name;
        uint32_t offset;
        std::string type;
    };
    std::unordered_map<std::string, Member> fields;
};

struct RecordSymbol : public Symbol {
    uint32_t width;
    uint32_t numFields;
    struct Field {
        std::string name;
        uint32_t shift;
        uint32_t fieldWidth;
        uint32_t mask;
        std::string initial;
    };
    std::unordered_map<std::string, Field> fields;
};

struct ProcedureSymbol : public Symbol {
    uint32_t value;
};

class SymbolTable {
public:
    void addSymbol(const Symbol &symbol);
    std::unique_ptr<Symbol> findSymbol(const std::string &name);

private:
    std::unordered_map<std::string, Symbol> symbols;
};
