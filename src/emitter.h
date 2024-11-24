#pragma once

#include "source_map.h"
#include "diagnostic.h"

#include <memory>
#include <ostream>
#include <iostream>
#include <fmt/color.h>

using LabelType = std::pair<Span, std::string>;

class Emitter {
public:
    explicit Emitter(const std::shared_ptr<SourceMap> &sourceMap, std::ostream &outStream = std::cout,
                     bool useColor = true);

    void emit(const std::shared_ptr<Diagnostic> &diag);
    void emitJSON(const std::vector<std::shared_ptr<Diagnostic>> &diagnostics);

private:
    std::shared_ptr<SourceMap> sourceMap;
    std::ostream &out;
    bool useColor;
    fmt::rgb whiteColor = fmt::rgb(200, 200, 200);
    fmt::rgb redColor = fmt::rgb(254, 89, 89);
    fmt::rgb yellowColor = fmt::rgb(255, 191, 0);
    fmt::rgb cyanColor = fmt::rgb(0, 200, 200);
    size_t spaceCount = 0xffffffff;

    template <typename... Args>
    std::string format(const fmt::text_style &ts, fmt::format_string<Args...> fmt_str, Args &&...args)
    {
        return fmt::format(useColor ? ts : fmt::text_style(), fmt_str, std::forward<Args>(args)...);
    }

    void printHeader(const std::shared_ptr<Diagnostic> &diag);
    void printDiagnosticBody(const std::shared_ptr<Diagnostic> &diag);
    void printNote(const std::shared_ptr<Diagnostic> &diag);
    void printHelp(const std::shared_ptr<Diagnostic> &diag);
    void printLabelsForLine(fmt::memory_buffer &buffer, const std::string &lineContent, size_t lineNumberZeroBased,
                            const std::optional<LabelType> &primaryLabel, std::vector<LabelType> &labels,
                            Diagnostic::Level level);

    std::string formatLevel(Diagnostic::Level level);
    std::string formatErrorCode(Diagnostic::Level level, ErrorCode code);
    int calculateDisplayWidth(const std::string &text);
    int calculateCodePoints(const std::string &text);
    void spanToLineChar(const Span &span, int &startLine, int &startChar, int &endLine, int &endChar) const;
};
