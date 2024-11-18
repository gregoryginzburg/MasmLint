#include "diagnostic.h"
#include "fmt/core.h"

// template <typename... Args>
// Diagnostic::Diagnostic(Level level, ErrorCode code, Args&&... args)
//     : level(level), code(code), message(fmt::format(getErrorMessage(code), std::forward<Args>(args)...)) {}


void Diagnostic::addPrimaryLabel(const Span &span, const std::string &labelMessage)
{
    primaryLabel = std::pair<Span, std::string>(span, labelMessage);
}

void Diagnostic::addSecondaryLabel(const Span &span, const std::string &labelMessage)
{
    secondaryLabels.emplace_back(span, labelMessage);
}

Diagnostic::Level Diagnostic::getLevel() const { return level; }

ErrorCode Diagnostic::getCode() const { return code; }

const std::string &Diagnostic::getMessage() const { return message; }

const std::pair<Span, std::string> &Diagnostic::getPrimaryLabel() const { return primaryLabel; }

const std::vector<std::pair<Span, std::string>> &Diagnostic::getSecondaryLabels() const { return secondaryLabels; }

const std::optional<std::string> &Diagnostic::getNoteMessage() const { return noteMessage; }

const std::optional<std::string> &Diagnostic::getHelpMessage() const {return helpMessage; }

const std::string getErrorMessage(ErrorCode code)
{
    switch (code) {
#define DEFINE_ERROR(code, message)                                                                                    \
    case ErrorCode::code:                                                                                              \
        return message;
#include "diagnostic_messages.def"
#undef DEFINE_ERROR
    default:
        return "Unknown error.";
    }
}