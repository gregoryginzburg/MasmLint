#pragma once

#include "source_map.h"
#include "diagnostic.h"

#include <memory>
#include <ostream>
#include <iostream>


class Emitter {
public:
    Emitter(std::shared_ptr<SourceMap> sourceMap, std::ostream &outStream = std::cout, bool useColor = true);

    void emit(const Diagnostic &diag);

private:
    std::shared_ptr<SourceMap> sourceMap;
    std::ostream &out;
    bool useColor;

    void printHeader(const Diagnostic &diag);
    void printLabels(const Diagnostic &diag);
    void printNotes(const Diagnostic &diag);

    std::string formatLevel(Diagnostic::Level level);
    std::string formatMessage(const std::string &message);
    std::string formatLocation(const std::filesystem::path &path, std::size_t line, std::size_t column);
    std::string createUnderline(const std::string &lineContent, std::size_t byteOffsetInLine, std::size_t byteLength);
    int calculateDisplayWidth(const std::string &text);
};

