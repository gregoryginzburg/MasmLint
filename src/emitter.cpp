#include "emitter.h"
#include "log.h"

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <utf8proc.h>

Emitter::Emitter(std::shared_ptr<SourceMap> sourceMap, std::ostream &outStream, bool useColor)
    : sourceMap(sourceMap), out(outStream), useColor(useColor)
{
}

void Emitter::emit(const Diagnostic &diag)
{
    printHeader(diag);
    printLabels(diag);
}

void Emitter::printHeader(const Diagnostic &diag)
{
    auto levelStr = formatLevel(diag.getLevel());
    auto codeStr = formatErrorCode(diag.getCode());
    auto message = fmt::format(fmt::emphasis::bold, "{}", diag.getMessage());

    std::string result = fmt::format(fmt::emphasis::bold, "{}[{}]: {}\n", levelStr, codeStr, message);
    out.write(result.data(), result.size());
}

std::string Emitter::formatErrorCode(ErrorCode code)
{
    return fmt::format(fmt::emphasis::bold, "E{:02d}", static_cast<int>(code));
}

void Emitter::printLabels(const Diagnostic &diag)
{
    for (const auto &[span, labelMsg] : diag.getLabels()) {
        std::filesystem::path filePath;
        std::size_t lineNumberZeroBased, columnNumberZeroBased;
        sourceMap->spanToLocation(span, filePath, lineNumberZeroBased, columnNumberZeroBased);

        auto sourceFile = sourceMap->lookupSourceFile(span.lo);
        if (sourceFile) {
            std::string lineContent = sourceFile->getLine(lineNumberZeroBased);
            std::size_t lineStartByte = sourceFile->getLineStart(lineNumberZeroBased);
            std::size_t byteOffsetInLine = span.lo - lineStartByte;
            std::size_t byteLength = span.hi - span.lo;

            fmt::memory_buffer buffer;
            if (useColor) {
                fmt::format_to(std::back_inserter(buffer), "{} {}:{}:{}\n", fmt::format(fg(fmt::color::cyan), "  -->"),
                               filePath.string(), lineNumberZeroBased + 1, columnNumberZeroBased + 1);
            } else {
                fmt::format_to(std::back_inserter(buffer), "  --> {}:{}:{}\n", filePath.string(),
                               lineNumberZeroBased + 1, columnNumberZeroBased + 1);
            }

            if (useColor) {
                fmt::format_to(std::back_inserter(buffer), "   {} {} {}\n",
                               fmt::format(fg(fmt::color::cyan), "{}", lineNumberZeroBased + 1),
                               fmt::format(fg(fmt::color::cyan), "|"), lineContent);

            } else {
                fmt::format_to(std::back_inserter(buffer), "   {} | {}\n", lineNumberZeroBased + 1, lineContent);
            }

            // Print underline
            std::string underline = createUnderline(lineContent, byteOffsetInLine, byteLength);
            if (useColor) {
                fmt::format_to(std::back_inserter(buffer), "     {} {}\n", fmt::format(fg(fmt::color::cyan), "|"),
                               underline);
            } else {
                fmt::format_to(std::back_inserter(buffer), "     | {}\n", underline);
            }

            // Print label message
            if (!labelMsg.empty()) {
                fmt::format_to(std::back_inserter(buffer), "     = {}\n", labelMsg);
            }

            out.write(buffer.data(), buffer.size());
        }
    }
}

std::string Emitter::formatLevel(Diagnostic::Level level)
{
    if (useColor) {
        switch (level) {
        case Diagnostic::Level::Error:
            return fmt::format(fmt::emphasis::bold | fg(fmt::color::red), "error");
        case Diagnostic::Level::Warning:
            return fmt::format(fmt::emphasis::bold | fg(fmt::color::yellow), "warning");
        case Diagnostic::Level::Note:
            return fmt::format(fmt::emphasis::bold | fg(fmt::color::cyan), "note");
        }
    } else {
        switch (level) {
        case Diagnostic::Level::Error:
            return "error";
        case Diagnostic::Level::Warning:
            return "warning";
        case Diagnostic::Level::Note:
            return "note";
        }
    }
    return "unknown";
}

int Emitter::calculateDisplayWidth(const std::string &text)
{
    int width = 0;
    const char *str = text.c_str();
    utf8proc_ssize_t len = text.size();
    utf8proc_ssize_t idx = 0;
    utf8proc_int32_t codepoint;
    while (idx < len) {
        utf8proc_ssize_t charLen = utf8proc_iterate((const utf8proc_uint8_t *)(str + idx), len - idx, &codepoint);
        if (charLen <= 0) {
            LOG_DETAILED_ERROR("Invalid utf-8 formatting in calculateDisplayWidth");
            break;
        }

        width += utf8proc_charwidth(codepoint);
        idx += charLen;
    }
    return width;
}

int calculateCodePoints(const std::string &text)
{
    int codePoints = 0;
    const char *str = text.c_str();
    utf8proc_ssize_t len = text.size();
    utf8proc_ssize_t idx = 0;
    utf8proc_int32_t codepoint;
    while (idx < len) {
        utf8proc_ssize_t charLen = utf8proc_iterate((const utf8proc_uint8_t *)(str + idx), len - idx, &codepoint);
        if (charLen <= 0) {
            LOG_DETAILED_ERROR("Invalid utf-8 formatting in calculateDisplayWidth");
            break;
        }
        idx += charLen;
        codePoints += charLen;
    }
    return codePoints;
}

std::string Emitter::createUnderline(const std::string &lineContent, std::size_t byteOffsetInLine,
                                     std::size_t byteLength)
{
    // Extract substrings
    std::string beforeUnderline = lineContent.substr(0, byteOffsetInLine);
    std::string underlineText = lineContent.substr(byteOffsetInLine, byteLength);

    int offsetWidth = calculateDisplayWidth(beforeUnderline);
    int underlineWidth = calculateDisplayWidth(underlineText);

    // Create the underline string
    std::string underline(offsetWidth, ' ');
    if (underlineWidth == 0) {
        LOG_DETAILED_ERROR("underlineWidth is 0");
    }
    std::string carets(underlineWidth > 0 ? underlineWidth : 1, '^');

    if (useColor) {
        underline += fmt::format(fmt::emphasis::bold | fg(fmt::color::red), "{}", carets);
    } else {
        underline += carets;
    }
    return underline;
}

void Emitter::emitJSON(const std::vector<Diagnostic> &diagnostics)
{
    json arr = json::array();
    for (const auto &diag : diagnostics) {
        json item;
        item["message"] = diag.getMessage();
        item["severity"] = static_cast<int>(diag.getLevel());

        for (const auto &[span, labelMsg] : diag.getLabels()) {
            int startLine, startChar, endLine, endChar;
            spanToLineChar(span, startLine, startChar, endLine, endChar);

            item["range"] = {{"start", {{"line", startLine}, {"character", startChar}}},
                             {"end", {{"line", endLine}, {"character", endChar}}}};
        }

        arr.push_back(item);
    }
    std::string result = arr.dump(4);
    out.write(result.data(), result.size());
}

void Emitter::spanToLineChar(const Span &span, int &startLine, int &startChar, int &endLine, int &endChar)
{
    std::filesystem::path filePath;
    std::size_t lineNumberZeroBased, columnNumberZeroBased;

    sourceMap->spanToLocation(span, filePath, lineNumberZeroBased, columnNumberZeroBased);
    startLine = endLine = lineNumberZeroBased;
    startChar = columnNumberZeroBased;

    auto sourceFile = sourceMap->lookupSourceFile(span.lo);
    if (sourceFile) {
        std::string lineContent = sourceFile->getLine(lineNumberZeroBased);
        std::size_t lineStartByte = sourceFile->getLineStart(lineNumberZeroBased);
        std::size_t byteOffsetInLine = span.lo - lineStartByte;
        std::size_t byteLength = span.hi - span.lo;
        std::string underlineText = lineContent.substr(byteOffsetInLine, byteLength);
        endChar = startChar + calculateCodePoints(underlineText) + 1;
    }
}