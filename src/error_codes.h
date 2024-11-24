#pragma once

enum class ErrorCode : std::uint8_t {
#define DEFINE_ERROR(code, message) code,
#define DEFINE_WARNING(code, message) code,
#include "diagnostic_messages.def"
#undef DEFINE_ERROR
#undef DEFINE_WARNING
};
