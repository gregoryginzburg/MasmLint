#include "span.h"
#include "log.h"

void SyntaxContextData::pushMacro(const std::string &macroName) { macroStack.push_back(macroName); }

void SyntaxContextData::popMacro()
{
    if (!macroStack.empty()) {
        macroStack.pop_back();
    }
}

std::string SyntaxContextData::currentMacro() const { return macroStack.empty() ? "" : macroStack.back(); }

bool Span::contains(std::size_t pos) const { return lo <= pos && pos < hi; }

bool Span::overlaps(const Span &other) const { return lo < other.hi && other.lo < hi; }

Span Span::merge(const Span &first, const Span &second)
{
    const std::size_t new_lo = std::min(first.lo, second.lo);
    const std::size_t new_hi = std::max(first.hi, second.hi);

    std::shared_ptr<SyntaxContextData> new_context = first.context;

    if (first.context != second.context) {
        LOG_DETAILED_ERROR("Can't merge spans with different contexts!");
        return {0, 0, nullptr};
    }

    return {new_lo, new_hi, new_context};
}

bool Span::operator==(const Span &other) const { return lo == other.lo && hi == other.hi && context == other.context; }

bool Span::operator!=(const Span &other) const { return !(*this == other); }

bool Span::operator<(const Span &other) const
{
    if (lo != other.lo) {
        return lo < other.lo;
    }
    if (hi != other.hi) {
        return hi < other.hi;
    }
    return true;
}

bool Span::operator>(const Span &other) const { return other < *this; }

bool Span::operator<=(const Span &other) const { return !(other < *this); }

bool Span::operator>=(const Span &other) const { return !(*this < other); }