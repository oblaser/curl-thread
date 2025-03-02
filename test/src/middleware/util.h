/*
author          Oliver Blaser
date            01.03.2025
copyright       MIT - Copyright (c) 2025 Oliver Blaser
*/

#ifndef IG_MDW_UTIL_H
#define IG_MDW_UTIL_H

#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>



#define SIZEOF_ARRAY(_array) (sizeof(_array) / sizeof(_array[0]))



#define UTIL_CLAMP(_v, _lo, _hi) ((_v) < (_lo) ? (_lo) : ((_v) > (_hi) ? (_hi) : (_v)))

// rounds a float or double away from zero, casting to an integer type has to be applied
#define UTIL_ROUND(_v) ((_v) < 0 ? ((_v)-0.5f) : ((_v) + 0.5f))



namespace util {

template <class T> const constexpr T& max(const T& a, const T& b) { return (a > b ? a : b); }
template <class T> const constexpr T& min(const T& a, const T& b) { return (a < b ? a : b); }

std::string t_to_iso8601(time_t t);
std::string t_to_iso8601_local(time_t t);
std::string t_to_iso8601_time_local(time_t t);

/**
 * Errors like `EINTR` are not handled.
 *
 * On Windows the value is clamped to a minimum of 1ms, and rounded to a multiple of 1ms in such a way that speed is
 * more important than accurate or binary rounding.
 *
 * @param t_us Time to sleep as microseconds
 * @return Non 0 if sleeping was aported
 */
int sleep(uint32_t t_us);


} // namespace util


#endif // IG_MDW_UTIL_H
