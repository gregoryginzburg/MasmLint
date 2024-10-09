#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include "log.h"

std::shared_ptr<spdlog::logger> Log::s_core_logger;
const char *Log::format_string = "%^%v%$";
spdlog::level::level_enum Log::log_level = spdlog::level::trace;

void Log::init()
{
    // https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
    spdlog::set_pattern(format_string);
    s_core_logger = spdlog::stdout_color_mt("MasmLint");
    s_core_logger->set_level(log_level);
}

void Log::add_sink(const spdlog::sink_ptr &new_sink)
{
    new_sink->set_pattern(format_string);
    new_sink->set_level(log_level);
    s_core_logger->sinks().push_back(new_sink);
}