#pragma once
#include <cstdio>

namespace vajra::log {

enum class Level { Trace, Info, Warn, Error };

inline const char* colour(Level l) {
    switch (l) {
        case Level::Trace: return "\033[90m";
        case Level::Info:  return "\033[36m";
        case Level::Warn:  return "\033[33m";
        case Level::Error: return "\033[31m";
    }
    return "\033[0m";
}

inline const char* tag(Level l) {
    switch (l) {
        case Level::Trace: return "TRACE";
        case Level::Info:  return "INFO ";
        case Level::Warn:  return "WARN ";
        case Level::Error: return "ERROR";
    }
    return "?????";
}

template <typename... Args>
void write(Level l, const char* fmt, Args... args) {
    std::fprintf(stderr, "%s[%s]\033[0m ", colour(l), tag(l));
    if constexpr (sizeof...(Args) == 0) {
        std::fputs(fmt, stderr);          // no varargs: avoids format-string risk
    } else {
        std::fprintf(stderr, fmt, args...);
    }
    std::fputc('\n', stderr);
}

}  // namespace vajra::log

#define VJ_TRACE(...) ::vajra::log::write(::vajra::log::Level::Trace, __VA_ARGS__)
#define VJ_INFO(...)  ::vajra::log::write(::vajra::log::Level::Info,  __VA_ARGS__)
#define VJ_WARN(...)  ::vajra::log::write(::vajra::log::Level::Warn,  __VA_ARGS__)
#define VJ_ERROR(...) ::vajra::log::write(::vajra::log::Level::Error, __VA_ARGS__)
