#pragma once

#include <unordered_map>
#include <filesystem>
#include "span.h"

class SourceFile {
public:
    SourceFile(std::filesystem::path path, const std::string &src, std::size_t startPos);

    const std::filesystem::path &getPath() const;
    const std::string &getSource() const;

    std::size_t getStartPos() const;
    std::size_t getEndPos() const;

    // Maps a byte position to a line number (zer based)
    std::size_t getLineNumber(std::size_t pos) const;

    std::string getLine(std::size_t lineNumber) const;

    // Maps a byte position to a column number within its line (zero based)
    std::size_t getColumnNumber(std::size_t pos) const;

    std::size_t getColumnPosition(std::size_t pos) const;

    std::size_t getLineStart(std::size_t lineNumber) const;

    std::size_t countCodePoints(std::size_t startByte, std::size_t endByte) const;

private:
    std::filesystem::path path;
    std::string src;                     // Source code content
    std::size_t startPos;                // Starting position in the global source map (including startPos)
    std::size_t endPos;                  // Ending position in the global source map (excluding endPos)
    std::vector<std::size_t> lineStarts; // Byte positions where each line starts
};

class SourceMap {
public:
    SourceMap() = default;

    std::shared_ptr<SourceFile> newSourceFile(const std::filesystem::path &path, const std::string &src);

    std::shared_ptr<SourceFile> loadFile(const std::filesystem::path &path);

    std::shared_ptr<SourceFile> lookupSourceFile(std::size_t pos) const;

    std::shared_ptr<SourceFile> getSourceFile(const std::filesystem::path &path) const;

    // Maps a global byte position to line and column (zero based)
    std::pair<std::size_t, std::size_t> lookupLineColumn(std::size_t pos) const;

    // Maps a span to file path, line, and column (zero based)
    void spanToLocation(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                        std::size_t &outColumn) const;

    void spanToEndLocation(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                           std::size_t &outColumn) const;

    // Column is returned as a byte offset, TODO: refactor?
    void spanToStartPosition(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                             std::size_t &outColumn) const;

    void spanToEndPosition(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                           std::size_t &outColumn) const;

    // Retrieves the source code snippet corresponding to a span
    std::string spanToSnippet(const Span &span) const;

private:
    std::vector<std::shared_ptr<SourceFile>> files;
};