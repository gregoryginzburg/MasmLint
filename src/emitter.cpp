#include "emitter.h"
#include "log.h"

#include <spdlog/fmt/bundled/core.h>
#include <spdlog/fmt/bundled/color.h>
#include <spdlog/fmt/bundled/ostream.h>

#include <utf8proc.h>

Emitter::Emitter(std::shared_ptr<SourceMap> sourceMap, std::ostream &outStream, bool useColor)
    : sourceMap(std::move(sourceMap)), out(outStream), useColor(useColor)
{
}

void Emitter::emit(const Diagnostic &diag)
{
    printHeader(diag);
    printLabels(diag);
    printNotes(diag);
}

void Emitter::printHeader(const Diagnostic &diag)
{
    auto levelStr = formatLevel(diag.getLevel());
    auto message = diag.getMessage();

    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), "{}: {}\n", levelStr, message);
    out.write(buffer.data(), buffer.size());
}

void Emitter::printLabels(const Diagnostic &diag)
{
    for (const auto &labelPair : diag.getLabels()) {
        const Span &span = labelPair.first;
        const std::string &labelMsg = labelPair.second;

        std::filesystem::path filePath;
        std::size_t lineNumberZeroBased, columnNumberZeroBased;
        sourceMap->spanToLocation(span, filePath, lineNumberZeroBased, columnNumberZeroBased);

        auto sourceFile = sourceMap->lookupSourceFile(span.lo);
        if (sourceFile) {
            // Get line content using zero-based line number
            std::string lineContent = sourceFile->getLine(lineNumberZeroBased);

            // Get line start byte position
            std::size_t lineStartByte = sourceFile->getLineStart(lineNumberZeroBased);

            // Compute byte offset of span.lo relative to line start
            std::size_t byteOffsetInLine = span.lo - lineStartByte;

            // Compute byte length of the span
            std::size_t byteLength = span.hi - span.lo;

            // Print location (convert to one-based for user display)
            fmt::memory_buffer buffer;
            fmt::format_to(std::back_inserter(buffer), "  --> {}:{}:{}\n", filePath.string(), lineNumberZeroBased + 1,
                           columnNumberZeroBased + 1);
            fmt::format_to(std::back_inserter(buffer), "   {} | {}\n", lineNumberZeroBased + 1, lineContent);

            // Print underline
            std::string underline = createUnderline(lineContent, byteOffsetInLine, byteLength);
            fmt::format_to(std::back_inserter(buffer), "     | {}\n", underline);

            // Print label message
            if (!labelMsg.empty()) {
                fmt::format_to(std::back_inserter(buffer), "     = {}\n", labelMsg);
            }

            out.write(buffer.data(), buffer.size());
        }
    }
}

void Emitter::printNotes(const Diagnostic &diag)
{
    for (const auto &note : diag.getNotes()) {
        fmt::memory_buffer buffer;
        if (useColor) {
            fmt::format_to(std::back_inserter(buffer), "{}: {}\n",
                           fmt::format(fmt::emphasis::bold | fg(fmt::color::cyan), "note"), note);
        } else {
            fmt::format_to(std::back_inserter(buffer), "note: {}\n", note);
        }
        out.write(buffer.data(), buffer.size());
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
    std::string carets(underlineWidth > 0 ? underlineWidth : 1, '^'); // Ensure at least one caret

    if (useColor) {
        underline += fmt::format(fmt::emphasis::bold | fg(fmt::color::red), "{}", carets);
    } else {
        underline += carets;
    }
    return underline;
}