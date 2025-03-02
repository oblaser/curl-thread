/*
author          Oliver Blaser
date            01.03.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>

#include "util.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif


namespace {}


std::string util::t_to_iso8601(time_t t)
{
    std::string r;

    constexpr size_t bufferSize = 100;
    char buffer[bufferSize];

    const struct std::tm* tm = std::localtime(&t);

    if (tm && (std::strftime(buffer, bufferSize, "%FT%T%z", tm) > 0)) { r = std::string(buffer); }
    else { r = '[' + std::to_string(t) + ']'; }

    return r;
}

std::string util::t_to_iso8601_local(time_t t)
{
    std::string r;

    constexpr size_t bufferSize = 100;
    char buffer[bufferSize];

    const struct std::tm* tm = std::localtime(&t);

    if (tm && (std::strftime(buffer, bufferSize, "%FT%T", tm) > 0)) { r = std::string(buffer); }
    else { r = '[' + std::to_string(t) + ']'; }

    return r;
}

std::string util::t_to_iso8601_time_local(time_t t)
{
    std::string r;

    constexpr size_t bufferSize = 100;
    char buffer[bufferSize];

    const struct std::tm* tm = std::localtime(&t);

    if (tm && (std::strftime(buffer, bufferSize, "%T", tm) > 0)) { r = std::string(buffer); }
    else { r = '[' + std::to_string(t) + ']'; }

    return r;
}

int util::sleep(uint32_t t_us)
{
#ifdef _WIN32

    if (t_us < 1000) { t_us = 1000; }

    // round at 3/4 (normal rounding would be done at 1/2), this could potentially lead to a value out of the specified
    // range, but this doesn't matter on Windows
    t_us += 250;

    const DWORD t_ms = (DWORD)(t_us / 1000);
    Sleep(t_ms);
    return 0;

#else

    const uint32_t sec = t_us / 1000000;

    struct timespec dur;
    dur.tv_sec = (time_t)sec;
    dur.tv_nsec = (int)(t_us - (sec * 1000000)) * 1000;
    return nanosleep(&dur, nullptr);

#endif
}
