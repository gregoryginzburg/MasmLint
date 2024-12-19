#pragma once

enum class ErrorCode : std::uint8_t {
#define DEFINE_ERROR(code, code_number, message) code = code_number,
#define DEFINE_WARNING(code, code_number, message) code = code_number,
#include "diagnostic_messages.def"
#undef DEFINE_ERROR
#undef DEFINE_WARNING
};
