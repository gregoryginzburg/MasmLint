#include "source_map.h"
#include "log.h"
#include "span.h"

#include <fstream>
#include <filesystem>

SourceFile::SourceFile(std::filesystem::path path, const std::string &src, std::size_t startPos)
    : path(std::move(path)), src(src), startPos(startPos), endPos(startPos + src.size())
{
    // Initialize lineStarts
    lineStarts.push_back(0);
    for (std::size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '\n') {
            lineStarts.push_back(i + 1);
        }
    }
}

const std::filesystem::path &SourceFile::getPath() const { return path; }

const std::string &SourceFile::getSource() const { return src; }

std::size_t SourceFile::getStartPos() const { return startPos; }

std::size_t SourceFile::getEndPos() const { return endPos; }

std::size_t SourceFile::getLineNumber(std::size_t pos) const
{
    if (pos < startPos || pos >= endPos) {
        LOG_DETAILED_ERROR("Position out of range in getLineNumber");
        return 0;
    }
    std::size_t localPos = pos - startPos;
    auto it = std::upper_bound(lineStarts.begin(), lineStarts.end(), localPos);
    return static_cast<size_t>((it - lineStarts.begin())) - 1;
}

// lineNumber is zero based
std::string SourceFile::getLine(std::size_t lineNumber) const
{
    if (lineNumber >= lineStarts.size()) {
        LOG_DETAILED_ERROR("Line number out of range in getLine");
        return "";
    }
    std::size_t start = lineStarts[lineNumber];
    std::size_t end = 0;
    if (lineNumber + 1 < lineStarts.size()) {
        end = lineStarts[lineNumber + 1];
    } else {
        end = src.size();
    }

    // Exclude the newline character at the end if present
    if (end > start && src[end - 1] == '\n') {
        end--;
    }
    return src.substr(start, end - start);
}

// lineNumber is zero based
std::size_t SourceFile::getLineStart(std::size_t lineNumber) const
{
    if (lineNumber >= lineStarts.size()) {
        LOG_DETAILED_ERROR("Line number out of range in getLineStart");
        return 0;
    }
    return lineStarts[lineNumber];
}

std::size_t SourceFile::countCodePoints(const std::string &str, std::size_t startByte, std::size_t endByte)
{
    std::size_t codePointCount = 0;
    std::size_t i = startByte;
    while (i < endByte) {
        auto c = static_cast<unsigned char>(str[i]);
        std::size_t charSize = 1;
        if ((c & 0x80U) == 0x00) {
            charSize = 1; // ASCII character
        } else if ((c & 0xE0U) == 0xC0) {
            charSize = 2; // 2-byte sequence
        } else if ((c & 0xF0U) == 0xE0) {
            charSize = 3; // 3-byte sequence
        } else if ((c & 0xF8U) == 0xF0) {
            charSize = 4; // 4-byte sequence
        } else {
            // Invalid UTF-8 start byte
            LOG_DETAILED_ERROR("Invalid UTF-8 encoding in countCodePoints");
            charSize = 1;
        }

        // Move to the next character
        i += charSize;
        codePointCount++;
    }
    return codePointCount;
}

std::size_t SourceFile::getColumnNumber(std::size_t pos) const
{
    std::size_t lineNumber = getLineNumber(pos);
    std::size_t lineStartPos = lineStarts[lineNumber];
    std::size_t localPos = pos - startPos;

    // Count code points between lineStartPos and localPos
    return countCodePoints(src, lineStartPos, localPos);
}

std::size_t SourceFile::getColumnPosition(std::size_t pos) const
{
    std::size_t lineNumber = getLineNumber(pos);
    std::size_t lineStartPos = lineStarts[lineNumber];
    std::size_t localPos = pos - startPos;

    return localPos - lineStartPos;
}

std::shared_ptr<SourceFile> SourceMap::newSourceFile(const std::filesystem::path &path, const std::string &src)
{
    std::size_t startPos = 0;
    if (!files.empty()) {
        startPos = files.back()->getEndPos();
    }
    auto file = std::make_shared<SourceFile>(path, src, startPos);
    files.push_back(file);
    return file;
}

std::shared_ptr<SourceFile> SourceMap::loadFile(const std::filesystem::path &path)
{
    auto existingFile = getSourceFile(path);
    if (existingFile) {
        return existingFile;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    // TODO: remove this hack to handle EndOfFile drawing
    content += "\n";

    return newSourceFile(path, content);
}

std::shared_ptr<SourceFile> SourceMap::lookupSourceFile(std::size_t pos) const
{
    for (const auto &file : files) {
        if (file->getStartPos() <= pos && pos < file->getEndPos()) {
            return file;
        }
    }
    return nullptr;
}

std::shared_ptr<SourceFile> SourceMap::getSourceFile(const std::filesystem::path &path) const
{
    for (const auto &file : files) {
        if (file->getPath() == path) {
            return file;
        }
    }
    return nullptr;
}

std::pair<std::size_t, std::size_t> SourceMap::lookupLineColumn(std::size_t pos) const
{
    auto file = lookupSourceFile(pos);
    if (file) {
        std::size_t lineNumber = file->getLineNumber(pos);
        std::size_t columnNumber = file->getColumnNumber(pos);
        return {lineNumber + 1, columnNumber + 1}; // Lines and columns are 1-based
    } else {
        return {0, 0};
    }
}

void SourceMap::spanToLocation(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                               std::size_t &outColumn) const
{
    // check if span corresponds to EndOfFile
    // if (files->())
    auto file = lookupSourceFile(span.lo);
    if (file) {
        outPath = file->getPath();
        outLine = file->getLineNumber(span.lo);     // Zero-based
        outColumn = file->getColumnNumber(span.lo); // Zero-based
    } else {
        outPath.clear();
        outLine = 0;
        outColumn = 0;
    }
}

void SourceMap::spanToEndLocation(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                                  std::size_t &outColumn) const
{
    auto file = lookupSourceFile(span.hi - 1);
    if (file) {
        outPath = file->getPath();
        outLine = file->getLineNumber(span.hi - 1);         // Zero-based
        outColumn = file->getColumnNumber(span.hi - 1) + 1; // Zero-based
    } else {
        outPath.clear();
        outLine = 0;
        outColumn = 0;
    }
}

void SourceMap::spanToStartPosition(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                                    std::size_t &outColumn) const
{
    auto file = lookupSourceFile(span.lo);
    if (file) {
        outPath = file->getPath();
        outLine = file->getLineNumber(span.lo); // Zero-based
        outColumn = file->getColumnPosition(span.lo);
    } else {
        outPath.clear();
        outLine = 0;
        outColumn = 0;
    }
}

void SourceMap::spanToEndPosition(const Span &span, std::filesystem::path &outPath, std::size_t &outLine,
                                  std::size_t &outColumn) const
{
    auto file = lookupSourceFile(span.hi - 1);
    if (file) {
        outPath = file->getPath();
        outLine = file->getLineNumber(span.hi - 1); // Zero-based
        // (need to add one because in the span (4, 5) startLocation is column 4 but endLocation column should be 5)
        outColumn = file->getColumnPosition(span.hi - 1) + 1; // Zero-based
    } else {
        outPath.clear();
        outLine = 0;
        outColumn = 0;
    }
}

std::string SourceMap::spanToSnippet(const Span &span) const
{
    auto sourceFile = lookupSourceFile(span.lo);
    if (!sourceFile) {
        LOG_DETAILED_ERROR("Span does not belong to any source file");
        return "";
    }

    std::size_t start_pos = span.lo - sourceFile->getStartPos();
    std::size_t end_pos = span.hi - sourceFile->getStartPos();

    if (end_pos > sourceFile->getSource().size()) {
        LOG_DETAILED_ERROR("Span end position out of range");
        return "";
    }

    return sourceFile->getSource().substr(start_pos, end_pos - start_pos);
}