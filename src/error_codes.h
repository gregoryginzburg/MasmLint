#pragma once

enum class ErrorCode {
#define DEFINE_ERROR(code, message) code,
#include "diagnostic_messages.def"
#undef DEFINE_ERROR
};
