#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <memory>

struct SyntaxContextData {
    std::vector<std::string> macroStack;

    void pushMacro(const std::string &macroName);

    void popMacro();

    std::string currentMacro() const;
};

struct Span {
    Span() : lo(0), hi(0), context(nullptr) {};
    Span(std::size_t start, std::size_t end, const std::shared_ptr<SyntaxContextData> &ctxt)
        : lo(start), hi(end), context(ctxt) {};

    bool contains(std::size_t pos) const;
    bool overlaps(const Span &other) const;

    static Span merge(const Span &first, const Span &second);

    bool operator==(const Span &other) const;
    bool operator!=(const Span &other) const;
    bool operator<(const Span &other) const;
    bool operator>(const Span &other) const;
    bool operator<=(const Span &other) const;
    bool operator>=(const Span &other) const;

    // absolute offsets in bytes from SourceMap
    // the range is [lo, hi) bytes
    std::size_t lo;
    std::size_t hi;

    std::shared_ptr<SyntaxContextData> context;
};