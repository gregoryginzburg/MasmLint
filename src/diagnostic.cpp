#include "diagnostic.h"
#include "fmt/core.h"

// template <typename... Args>
// Diagnostic::Diagnostic(Level level, ErrorCode code, Args&&... args)
//     : level(level), code(code), message(fmt::format(getErrorMessage(code), std::forward<Args>(args)...)) {}

void Diagnostic::addLabel(const Span &span, const std::string &labelMessage)
{
    labels.emplace_back(span, labelMessage);
}

Diagnostic::Level Diagnostic::getLevel() const { return level; }

ErrorCode Diagnostic::getCode() const { return code; }

const std::string &Diagnostic::getMessage() const { return message; }

const std::vector<std::pair<Span, std::string>> &Diagnostic::getLabels() const { return labels; }

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