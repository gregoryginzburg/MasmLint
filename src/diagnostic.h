#pragma once

#include <string>
#include <vector>
#include <memory>
#include <fmt/core.h>

#include "span.h"
#include "error_codes.h"

const std::string getErrorMessage(ErrorCode code);

class Diagnostic {
public:
    enum class Level { Error, Warning, Note };

    template <typename... Args>
    Diagnostic(Level level, ErrorCode code, Args&&... args)
        : level(level), code(code), message(fmt::format(fmt::runtime(getErrorMessage(code)), std::forward<Args>(args)...)) {}

    void addLabel(const Span &span, const std::string &labelMessage);

    Level getLevel() const;
    ErrorCode getCode() const;
    const std::string &getMessage() const;
    const std::vector<std::pair<Span, std::string>> &getLabels() const;

private:
    Level level;
    ErrorCode code;
    std::string message;
    std::vector<std::pair<Span, std::string>> labels;
};

