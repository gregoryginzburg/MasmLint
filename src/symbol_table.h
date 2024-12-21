#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "token.h"
#include "ast.h"

struct Symbol {
    Token token{};
    bool wasVisited = false;
    bool wasDefined = false; // when it's defined, its value is evaluated, also avoid visiting ExpressionPtr twice
    // bool wasEvaluated = false; // when error happens
    virtual ~Symbol() = default;
};

struct LabelSymbol : public Symbol {
    LabelSymbol(Token token) { this->token = std::move(token); }
    int32_t value = -1;

    int32_t size = 4;
    int32_t sizeOf = 4;
    int32_t length = 1;
    int32_t lengthOf = 1;
};

struct ProcSymbol : public Symbol {
    ProcSymbol(Token token) { this->token = std::move(token); }
    int32_t value = -1;

    int32_t size = 4;
    int32_t sizeOf = 4;
    int32_t length = 1;
    int32_t lengthOf = 1;
};

struct DataVariableSymbol : public Symbol {
    DataVariableSymbol(Token token, Token dataType) : dataType(std::move(dataType)) { this->token = std::move(token); }
    Token dataType;
    OperandSize dataTypeSize = OperandSize("", -1);
    int32_t value = -1;

    int32_t size = -1;
    int32_t sizeOf = -1;
    int32_t length = -1;
    int32_t lengthOf = -1;
};

struct EquVariableSymbol : public Symbol {
    EquVariableSymbol(Token token, std::shared_ptr<EquDir> equDir) : equDir(std::move(equDir)) { this->token = std::move(token); }
    std::shared_ptr<EquDir> equDir;
    int32_t value = -1;

    // size, sizeof, length, lengthof are allowed, dont't change the value of the constant
};

struct EqualVariableSymbol : public Symbol {
    EqualVariableSymbol(Token token, std::shared_ptr<EqualDir> equalDir) : equalDir(std::move(equalDir)) { this->token = std::move(token); }
    std::shared_ptr<EqualDir> equalDir;
    int32_t value = -1;

    // size, sizeof, length, lengthof are allowed, dont't change the value of the constant
};

struct StructSymbol : public Symbol {
    StructSymbol(Token token, std::shared_ptr<StructDir> structDir, std::unordered_map<std::string, std::shared_ptr<DataVariableSymbol>> namedFields)
        : structDir(std::move(structDir)), namedFields(std::move(namedFields))
    {
        this->token = std::move(token);
    }
    std::shared_ptr<StructDir> structDir;
    std::unordered_map<std::string, std::shared_ptr<DataVariableSymbol>> namedFields;

    int32_t size = -1;
    int32_t sizeOf = -1;
    // length and length of are not allowed
};

struct RecordSymbol : public Symbol {
    RecordSymbol(Token token, std::shared_ptr<RecordDir> recordDir, std::vector<std::shared_ptr<RecordField>> fields) : recordDir(std::move(recordDir)), fields(std::move(fields)) { this->token = std::move(token); }
    std::vector<std::shared_ptr<RecordField>> fields;
    std::shared_ptr<RecordDir> recordDir;

    int32_t width = -1;
    int32_t mask = -1;

    int32_t size = 4;
    int32_t sizeOf = 4;
    // length and length of are not allowed
};

struct RecordFieldSymbol : public Symbol {
    RecordFieldSymbol(Token token, std::shared_ptr<RecordField> recordField) : recordField(std::move(recordField)) { this->token = std::move(token); }
    std::shared_ptr<RecordField> recordField;

    int32_t width = -1;
    std::optional<int32_t> initial;
    int32_t shift = -1;
    int32_t mask = -1;

    // size, sizeof, length, lengthof are not allowed
};

class SymbolTable {
public:
    void addSymbol(const std::shared_ptr<Symbol> &symbol);

    void removeSymbol(const std::shared_ptr<Symbol> &symbol);
    std::shared_ptr<Symbol> findSymbol(const Token &token);
    std::shared_ptr<Symbol> findSymbol(const std::string &name);
    void printSymbols();

private:
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
};