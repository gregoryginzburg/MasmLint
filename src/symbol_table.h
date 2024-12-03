#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "token.h"

struct Symbol {
    Token token{};
    bool wasDefined = false;
    virtual ~Symbol() = default;
};

struct VariableSymbol : public Symbol {
    enum class Type : std::uint8_t { Label, DataVariable, EqualVariable, EquVariable };
    VariableSymbol(Token token, Type type) : type(type) { this->token = std::move(token); }
    Type type;
    std::string dataType;
    int32_t value;
};

struct StructSymbol : public Symbol {
    StructSymbol(Token token) { this->token = std::move(token); }
    int32_t size;
    int32_t numMembers;
    struct Member {
        std::string name;
        int32_t offset;
        std::string type;
    };
    std::unordered_map<std::string, Member> fields;
};

struct RecordSymbol : public Symbol {
    RecordSymbol(Token token) { this->token = std::move(token); }
    int32_t totalSize; // total size in bits
    int32_t numFields;
    std::vector<std::string> fields;
};

struct RecordFieldSymbol : public Symbol {
    RecordFieldSymbol(Token token) { this->token = std::move(token); }
    int32_t shift;
    int32_t width;
    int32_t mask;
    int32_t initial;
};

struct ProcSymbol : public Symbol {
    ProcSymbol(Token token) { this->token = std::move(token); }
    uint32_t value;
};

class SymbolTable {
public:
    void addSymbol(const std::shared_ptr<Symbol> &symbol);
    std::shared_ptr<Symbol> findSymbol(const Token &token);
    void printSymbols();

private:
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
};