#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <fmt/core.h>

#include "span.h"
#include "error_codes.h"

std::string getErrorMessage(ErrorCode code);

class Diagnostic {
public:
    enum class Level : std::uint8_t { Error, Warning, Note };

    template <typename... Args>
    Diagnostic(Level level, ErrorCode code, Args &&...args)
        : level(level), code(code),
          message(fmt::format(fmt::runtime(getErrorMessage(code)), std::forward<Args>(args)...))
    {
    }

    void addPrimaryLabel(const Span &span, const std::string &labelMessage);
    void addSecondaryLabel(const Span &span, const std::string &labelMessage);

    void addNoteMessage(const std::string &msg);

    Level getLevel() const;
    ErrorCode getCode() const;
    const std::string &getMessage() const;
    const std::pair<Span, std::string> &getPrimaryLabel() const;
    const std::vector<std::pair<Span, std::string>> &getSecondaryLabels() const;
    const std::optional<std::string> &getNoteMessage() const;
    const std::optional<std::string> &getHelpMessage() const;

    void cancel();
    bool isCancelled() const;

private:
    Level level;
    ErrorCode code;
    std::string message;
    std::pair<Span, std::string> primaryLabel;
    std::vector<std::pair<Span, std::string>> secondaryLabels;

    std::optional<std::string> noteMessage;
    std::vector<std::pair<Span, std::string>> noteLabels;

    std::optional<std::string> helpMessage;
    // std::optional<std::string> stringToDelete;
    std::optional<std::string> stringToInsert;
    std::vector<Span> insertColor;
    std::vector<Span> deleteColor;

    bool cancelled = false;
};
