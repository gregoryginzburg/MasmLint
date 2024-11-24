#pragma once

// Regular logging macros
// #define LOG_TRACE(...) Log::get_core_logger()->trace(__VA_ARGS__)
// #define LOG_INFO(...) Log::get_core_logger()->info(__VA_ARGS__)
// #define LOG_WARN(...) Log::get_core_logger()->warn(__VA_ARGS__)
// #define LOG_ERROR(...) Log::get_core_logger()->error(__VA_ARGS__)

// // Detailed logging macros
// #if defined(__clang__)
// #    define PRETTY_FUNCTION __PRETTY_FUNCTION__
// #elif defined(__GNUC__) || defined(__GNUG__)
// #    define PRETTY_FUNCTION __PRETTY_FUNCTION__
// #elif defined(_MSC_VER)
// #    define PRETTY_FUNCTION __FUNCSIG__
// #else
// #    define PRETTY_FUNCTION __func__
// #endif

// #define LOG_FORMAT_DETAILED(message, file, line, function) "[{}:{} ({})] {}", file, line, function, message

// #define LOG_DETAILED_TRACE(...)                                                                                        \
//     Log::get_core_logger()->trace(LOG_FORMAT_DETAILED(fmt::format(__VA_ARGS__), __FILE__, __LINE__, PRETTY_FUNCTION))
// #define LOG_DETAILED_INFO(...)                                                                                         \
//     Log::get_core_logger()->info(LOG_FORMAT_DETAILED(fmt::format(__VA_ARGS__), __FILE__, __LINE__, PRETTY_FUNCTION))
// #define LOG_DETAILED_WARN(...)                                                                                         \
//     Log::get_core_logger()->warn(LOG_FORMAT_DETAILED(fmt::format(__VA_ARGS__), __FILE__, __LINE__, PRETTY_FUNCTION))
// #define LOG_DETAILED_ERROR(...)                                                                                        \
//     Log::get_core_logger()->error(LOG_FORMAT_DETAILED(fmt::format(__VA_ARGS__), __FILE__, __LINE__, PRETTY_FUNCTION))
#include <fmt/core.h>
#include <fmt/color.h>

#if defined(__GNUC__) || defined(__clang__)
#    define PRETTY_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#    define PRETTY_FUNCTION __FUNCSIG__
#else
#    define PRETTY_FUNCTION __func__
#endif

#define LOG_DETAILED_ERROR(...)                                                                                        \
    fmt::print(stderr, "{} {} ({}:{}): {}\n", fmt::format(fg(fmt::color::red), "[ERROR]"), __FILE__, __LINE__,         \
               PRETTY_FUNCTION, fmt::format(__VA_ARGS__));
