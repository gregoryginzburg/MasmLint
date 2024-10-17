#pragma once

#include <unordered_map>
#include <filesystem>
#include "span.h"

class SourceFile {
public:
    SourceFile(const std::filesystem::path &path, const std::string &src, std::size_t startPos);

    const std::filesystem::path &getPath() const;
    const std::string &getSource() const;

    std::size_t getStartPos() const;
    std::size_t getEndPos() const;

    // Maps a byte position to a line number
    std::size_t getLineNumber(std::size_t pos) const;

    // Retrieves the content of a specific line
    std::string getLine(std::size_t lineNumber) const;

    // Maps a byte position to a column number within its line
    std::size_t getColumnNumber(std::size_t pos) const;

    std::size_t getLineStart(std::size_t lineNumber) const;

    static std::size_t countCodePoints(const std::string &str, std::size_t startByte, std::size_t endByte);

private:
    std::filesystem::path path;          // File path
    std::string src;                     // Source code content
    std::size_t startPos;                // Starting position in the global source map
    std::size_t endPos;                  // Ending position in the global source map
    std::vector<std::size_t> lineStarts; // Byte positions where each line starts

};

class SourceMap {
public:
    SourceMap() {};

    // Adds a new source file to the map
    std::shared_ptr<SourceFile> newSourceFile(const std::filesystem::path &path, const std::string &src);

    // Loads a file from disk and adds it to the source map
    std::shared_ptr<SourceFile> loadFile(const std::filesystem::path &path);

    // Finds the source file containing a given position
    std::shared_ptr<SourceFile> lookupSourceFile(std::size_t pos) const;

    // Retrieves a source file by its path
    std::shared_ptr<SourceFile> getSourceFile(const std::filesystem::path &path) const;

    // Maps a global byte position to line and column
    std::pair<std::size_t, std::size_t> lookupLineColumn(std::size_t pos) const;

    // Maps a span to file path, line, and column
    void spanToLocation(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                        std::size_t &outColumn) const;

    // Retrieves the source code snippet corresponding to a span
    std::string spanToSnippet(const Span &span) const;

private:
    std::vector<std::shared_ptr<SourceFile>> files;
};