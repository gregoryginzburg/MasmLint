#include "diagnostic.h"

Diagnostic::Diagnostic(Level level, const std::string &message)
    : level(level), message(message)
{
}

void Diagnostic::addLabel(const Span &span, const std::string &labelMessage)
{
    labels.emplace_back(span, labelMessage);
}

void Diagnostic::addNote(const std::string &note)
{
    notes.push_back(note);
}

Diagnostic::Level Diagnostic::getLevel() const
{
    return level;
}

const std::string &Diagnostic::getMessage() const
{
    return message;
}

const std::vector<std::pair<Span, std::string>> &Diagnostic::getLabels() const
{
    return labels;
}

const std::vector<std::string> &Diagnostic::getNotes() const
{
    return notes;
}
