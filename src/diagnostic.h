#pragma once

#include <string>
#include <vector>
#include <memory>

#include "span.h"

class Diagnostic {
public:
    enum class Level { Error, Warning, Note };

    Diagnostic(Level level, const std::string &message);

    void addLabel(const Span &span, const std::string &labelMessage);
    void addNote(const std::string &note);

    // Accessors
    Level getLevel() const;
    const std::string &getMessage() const;
    const std::vector<std::pair<Span, std::string>> &getLabels() const;
    const std::vector<std::string> &getNotes() const;

private:
    Level level;
    std::string message;
    std::vector<std::pair<Span, std::string>> labels;
    std::vector<std::string> notes;
};

