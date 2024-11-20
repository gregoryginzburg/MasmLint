#include "emitter.h"
#include "log.h"

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/ostream.h>
#include <map>
#include <algorithm>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <utf8proc.h>

Emitter::Emitter(std::shared_ptr<SourceMap> sourceMap, std::ostream &outStream, bool useColor)
    : sourceMap(sourceMap), out(outStream), useColor(useColor)
{
}

void Emitter::emit(std::shared_ptr<Diagnostic> diag)
{
    if (diag->isCancelled()) {
        return;
    }
    printHeader(diag);
    printDiagnosticBody(diag);
    if (diag->getNoteMessage()) {
        printNote(diag);
    }
    if (diag->getHelpMessage()) {
        printHelp(diag);
    }
}

void Emitter::printHeader(std::shared_ptr<Diagnostic> diag)
{
    auto levelStr = formatLevel(diag->getLevel());
    auto codeStr = formatErrorCode(diag->getLevel(), diag->getCode());
    auto message = format(fmt::emphasis::bold | fg(whiteColor), "{}", diag->getMessage());
    auto colon = format(fmt::emphasis::bold | fg(whiteColor), ":");

    std::string result = fmt::format("{}{}: {}\n", levelStr, codeStr, message);
    out.write(result.data(), result.size());
}

std::string Emitter::formatLevel(Diagnostic::Level level)
{

    switch (level) {
    case Diagnostic::Level::Error:
        return format(fmt::emphasis::bold | fg(redColor), "error");
    case Diagnostic::Level::Warning:
        return format(fmt::emphasis::bold | fg(yellowColor), "warning");
    case Diagnostic::Level::Note:
        return format(fmt::emphasis::bold | fg(cyanColor), "note");
    }

    return "unknown";
}

std::string Emitter::formatErrorCode(Diagnostic::Level level, ErrorCode code)
{
    switch (level) {
    case Diagnostic::Level::Error:
        return format(fmt::emphasis::bold | fg(redColor), "[E{:02d}]", static_cast<int>(code));
    case Diagnostic::Level::Warning:
        return format(fmt::emphasis::bold | fg(yellowColor), "[E{:02d}]", static_cast<int>(code));
    case Diagnostic::Level::Note:
        return format(fmt::emphasis::bold | fg(cyanColor), "[E{:02d}]", static_cast<int>(code));
    }

    return "unknown";
}

void Emitter::printDiagnosticBody(std::shared_ptr<Diagnostic> diag)
{
    // Collect all labels
    std::map<std::filesystem::path, std::map<size_t, std::vector<LabelType>>> labelsMapping;

    size_t maxLineNumber = 0;
    // primary label
    const auto &[primarySpan, primaryLabelMsg] = diag->getPrimaryLabel();
    std::filesystem::path primaryFilePath;
    std::size_t primaryLineNumberZeroBased, primaryColumnNumberZeroBased;
    sourceMap->spanToLocation(primarySpan, primaryFilePath, primaryLineNumberZeroBased, primaryColumnNumberZeroBased);
    labelsMapping[primaryFilePath][primaryLineNumberZeroBased] =
        std::vector<LabelType>(1, LabelType(primarySpan, primaryLabelMsg));
    maxLineNumber = std::max(maxLineNumber, primaryLineNumberZeroBased + 1);

    // secondary labels
    for (const auto &[span, labelMsg] : diag->getSecondaryLabels()) {
        std::filesystem::path filePath;
        std::size_t lineNumberZeroBased, columnNumberZeroBased;
        sourceMap->spanToLocation(span, filePath, lineNumberZeroBased, columnNumberZeroBased);
        if (labelsMapping.contains(filePath) && labelsMapping[filePath].contains(lineNumberZeroBased)) {
            labelsMapping[filePath][lineNumberZeroBased].emplace_back(span, labelMsg);
        } else {
            labelsMapping[filePath][lineNumberZeroBased] = std::vector<LabelType>(1, LabelType(span, labelMsg));
        }
        maxLineNumber = std::max(maxLineNumber, lineNumberZeroBased + 1);
    }

    int spaceCount = calculateDisplayWidth(std::to_string(maxLineNumber)) + 1;
    fmt::memory_buffer buffer;
    int primaryLineNumberWidth = calculateDisplayWidth(std::to_string(primaryLineNumberZeroBased + 1));
    // Print the location header
    fmt::format_to(std::back_inserter(buffer), "{}{} {}:{}:{}\n", std::string(spaceCount, ' '),
                   format(fg(cyanColor), "-->"), primaryFilePath.string(), primaryLineNumberZeroBased + 1,
                   primaryColumnNumberZeroBased + 1);

    // print empty line
    fmt::format_to(std::back_inserter(buffer), "{} {}\n", std::string(spaceCount, ' '), format(fg(cyanColor), "|"));

    // print primary string
    auto primarySourceFile = sourceMap->getSourceFile(primaryFilePath);
    std::string primaryLineContent = primarySourceFile->getLine(primaryLineNumberZeroBased);
    fmt::format_to(std::back_inserter(buffer), "{}{} {} {}\n", std::string(spaceCount - primaryLineNumberWidth, ' '),
                   format(fg(cyanColor), "{}", primaryLineNumberZeroBased + 1), format(fg(cyanColor), "|"),
                   primaryLineContent);

    // print labels in primary string
    std::vector<LabelType> primarySecondaryLabels;
    printLabelsForLine(buffer, primaryLineContent, primaryLineNumberZeroBased, spaceCount,
                       LabelType(primarySpan, primaryLabelMsg),
                       labelsMapping[primaryFilePath][primaryLineNumberZeroBased], diag->getLevel());

    // print all other lines in primary file
    for (const auto &[lineNumberZeroBased, labels] : labelsMapping[primaryFilePath]) {
        if (lineNumberZeroBased == primaryLineNumberZeroBased) {
            continue;
        }
        // print "..."
        fmt::format_to(std::back_inserter(buffer), "{}{}\n", std::string(spaceCount, ' '),
                       format(fg(cyanColor), "..."));
        // print empty line
        fmt::format_to(std::back_inserter(buffer), "{} {}\n", std::string(spaceCount, ' '), format(fg(cyanColor), "|"));

        // print string
        std::string lineContent = primarySourceFile->getLine(lineNumberZeroBased);
        int lineNumberWidth = calculateDisplayWidth(std::to_string(lineNumberZeroBased + 1));
        fmt::format_to(std::back_inserter(buffer), "{}{} {} {}\n", std::string(spaceCount - lineNumberWidth, ' '),
                       format(fg(cyanColor), "{}", lineNumberZeroBased + 1), format(fg(cyanColor), "|"), lineContent);
        printLabelsForLine(buffer, lineContent, lineNumberZeroBased, spaceCount, std::nullopt,
                           labelsMapping[primaryFilePath][lineNumberZeroBased], diag->getLevel());
    }

    // print all other files and labels
    for (const auto &[filePath, linesToLabelsMap] : labelsMapping) {
        if (filePath == primaryFilePath) {
            continue;
        }
        // print location
        // print all lines
        // TODO: finish this
        fmt::format_to(std::back_inserter(buffer), "{}\n",
                       format(fg(redColor), "Labels in different files not implemented!"));
    }

    out.write(buffer.data(), buffer.size());
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

int Emitter::calculateCodePoints(const std::string &text)
{
    int codePoints = 0;
    const char *str = text.c_str();
    utf8proc_ssize_t len = text.size();
    utf8proc_ssize_t idx = 0;
    utf8proc_int32_t codepoint;
    while (idx < len) {
        utf8proc_ssize_t charLen = utf8proc_iterate((const utf8proc_uint8_t *)(str + idx), len - idx, &codepoint);
        if (charLen <= 0) {
            LOG_DETAILED_ERROR("Invalid utf-8 formatting in calculateCodePoints");
            break;
        }
        idx += charLen;
        codePoints += static_cast<int>(charLen);
    }
    return codePoints;
}

void Emitter::printLabelsForLine(fmt::memory_buffer &buffer, std::string lineContent, size_t lineNumberZeroBased,
                                 int spacesCount, const std::optional<LabelType> &primaryLabel,
                                 std::vector<LabelType> &labels, Diagnostic::Level level)
{
    fmt::rgb primaryColor;
    switch (level) {
    case Diagnostic::Level::Error:
        primaryColor = redColor;
        break;
    case Diagnostic::Level::Warning:
        primaryColor = yellowColor;
        break;
    }

    // Initialize a marker line
    // + 1 needed if we are underlining the '\n' (it's not included in the lineContent)
    std::string markerLine(calculateDisplayWidth(lineContent) + 1, ' ');

    // Vector to hold messages that need to be printed under the marker line
    std::vector<std::tuple<size_t, std::string, bool>> labelMessagesToPrint;
    // Label Message to print on the same line
    std::string labelMessageToAdd = "";

    // Sort in the descneding order
    std::sort(labels.begin(), labels.end(), [](auto a, auto b) { return a > b; });

    // Apply labels to the marker line
    for (const auto &[span, labelMsg] : labels) {
        std::filesystem::path filePath;
        std::size_t startLine, startColumn, endLine, endColumn;
        sourceMap->spanToStartLocation(span, filePath, startLine, startColumn);
        sourceMap->spanToEndLocation(span, filePath, endLine, endColumn);

        if (startLine != lineNumberZeroBased || endLine != lineNumberZeroBased) {
            LOG_DETAILED_ERROR("In printLabelsForLine label span isn't the same as specified");
            return;
        }

        // Adjust for UTF-8 characters
        size_t startPos = calculateDisplayWidth(lineContent.substr(0, startColumn));
        size_t endPos = calculateDisplayWidth(lineContent.substr(0, endColumn));

        char markerChar = '-';
        bool isPrimaryLabel = primaryLabel && primaryLabel.value().first == span;
        if (isPrimaryLabel) {
            markerChar = '^';
        }

        // Fill in the markers
        for (size_t i = startPos; i < endPos && i < markerLine.size(); ++i) {
            markerLine[i] = markerChar;
        }

        // '\n' has empty width, need to handle separately
        if (startPos == endPos && startPos < markerLine.size()) {
            markerLine[startPos] = markerChar;
        }

        // Handle label messages
        if (!labelMsg.empty()) {
            // Append the message after the markers if it's the first from the right and it's the primary
            if (labels[0].first == span && isPrimaryLabel) {
                labelMessageToAdd = " " + labelMsg;
            } else {
                labelMessagesToPrint.emplace_back(startPos, labelMsg, isPrimaryLabel);
            }
        }
    }

    // Print the marker line with appropriate color
    std::string coloredMarkerLine;
    for (size_t i = 0; i < markerLine.size(); ++i) {
        char c = markerLine[i];
        if (c == '^') {
            // Color primary markers red
            coloredMarkerLine += format(fmt::emphasis::bold | fg(primaryColor), "{}", c);
        } else if (c == '-') {
            // Color secondary markers cyan
            coloredMarkerLine += format(fmt::emphasis::bold | fg(cyanColor), "{}", c);
        } else if (c == ' ') {
            // Keep spaces
            coloredMarkerLine += c;
        }
    }

    if (!labelMessageToAdd.empty()) {
        coloredMarkerLine += format(fg(primaryColor), "{}", labelMessageToAdd);
    }

    fmt::format_to(std::back_inserter(buffer), "{} {} {}\n", std::string(spacesCount, ' '), format(fg(cyanColor), "|"),
                   coloredMarkerLine);

    if (labelMessagesToPrint.empty()) {
        return;
    }

    // Now print any label messages that didn't fit on the marker line
    // first print initial line of | | | ...
    {
        const auto &[currentStartPos, currentLabelMsg, currentIsPrimaryLabel] = labelMessagesToPrint[0];
        size_t lineLength = currentStartPos + 1;
        std::string messageLine(lineLength, ' ');

        std::optional<size_t> primaryLabelIndex;
        for (const auto &[startPos, _, isPrimaryLabel] : labelMessagesToPrint) {
            if (startPos > currentStartPos) {
                continue;
            }
            messageLine[startPos] = '|';
            if (isPrimaryLabel) {
                primaryLabelIndex = startPos;
            }
        }

        std::string coloredLine;
        for (size_t i = 0; i < messageLine.size(); ++i) {
            char c = messageLine[i];
            if (c == '|') {
                if (primaryLabelIndex && primaryLabelIndex == i) {
                    coloredLine += format(fg(primaryColor), "{}", "│");
                } else {
                    coloredLine += format(fg(cyanColor), "{}", "│");
                }
            } else {
                coloredLine += c;
            }
        }
        fmt::format_to(std::back_inserter(buffer), "{} {} {}\n", std::string(spacesCount, ' '),
                       format(fg(cyanColor), "|"), coloredLine);
    }

    // then print all the messages
    for (const auto &[currentStartPos, currentLabelMsg, currentIsPrimaryLabel] : labelMessagesToPrint) {
        // + 1 isn't needed, because last can't be a |
        size_t lineLength = currentStartPos;
        std::string messageLine(lineLength, ' ');

        std::optional<size_t> primaryLabelIndex;
        for (const auto &[startPos, _, isPrimaryLabel] : labelMessagesToPrint) {
            if (startPos >= currentStartPos) {
                continue;
            }
            messageLine[startPos] = '|';
            if (isPrimaryLabel) {
                primaryLabelIndex = startPos;
            }
        }
        std::string coloredLine;
        for (size_t i = 0; i < messageLine.size(); ++i) {
            char c = messageLine[i];
            if (c == '|') {
                if (primaryLabelIndex && primaryLabelIndex == i) {
                    coloredLine += format(fg(primaryColor), "{}", "│");
                } else {
                    coloredLine += format(fg(cyanColor), "{}", "│");
                }
            } else {
                coloredLine += c;
            }
        }
        std::string labelMessage = currentLabelMsg;
        if (currentIsPrimaryLabel) {
            coloredLine += format(fg(primaryColor), "{}", labelMessage);
        } else {
            coloredLine += format(fg(cyanColor), "{}", labelMessage);
        }

        fmt::format_to(std::back_inserter(buffer), "{} {} {}\n", std::string(spacesCount, ' '),
                       format(fg(cyanColor), "|"), coloredLine);
    }
}

void Emitter::printNote(std::shared_ptr<Diagnostic> /*diag*/) {}

void Emitter::printHelp(std::shared_ptr<Diagnostic> /*diag*/) {}

void Emitter::emitJSON(const std::vector<std::shared_ptr<Diagnostic>> &diagnostics)
{
    json arr = json::array();
    for (const auto &diag : diagnostics) {
        json item;
        item["message"] = diag->getMessage();
        item["severity"] = static_cast<int>(diag->getLevel());

        for (const auto &[span, labelMsg] : diag->getSecondaryLabels()) {
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
    startLine = endLine = static_cast<int>(lineNumberZeroBased);
    startChar = static_cast<int>(columnNumberZeroBased);

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