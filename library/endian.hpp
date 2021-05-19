#ifndef STIFFER_ENDIAN_HPP
#define STIFFER_ENDIAN_HPP

#include <cstdint>
#include <ostream>

#include "byte_swap.hpp"

/* The declarations below are exported */
#pragma GCC visibility push(default)

namespace stiffer {

/// Endian.
/// @note Uses a definition similar to what's described for C++20.
/// @see https://en.cppreference.com/w/cpp/types/endian
enum class endian {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
    little = __ORDER_LITTLE_ENDIAN__,
    big = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#elif defined(_MSC_VER)
    little = 0,
    big = 1,
    native = little
#else
#error "platform not supported"
#endif
};

std::ostream& operator<<(std::ostream& os, endian value);

template <typename T>
T to_big_endian(T value)
{
    if (endian::native != endian::big) {
        return byte_swap(value);
    }
    return value;
}

template <typename T>
T to_little_endian(T value)
{
    if (endian::native != endian::little) {
        return byte_swap(value);
    }
    return value;
}

template <typename T>
T to_endian(T value, endian order)
{
    return (order == endian::big) ? to_big_endian(value): to_little_endian(value);
}

template <typename T>
T from_big_endian(const T& value)
{
    if (endian::native != endian::big) {
        return byte_swap(value);
    }
    return value;
}

template <typename T>
T from_little_endian(const T& value)
{
    if (endian::native != endian::little) {
        return byte_swap(value);
    }
    return value;
}

template <typename T>
T from_endian(T value, endian order)
{
    return (order == endian::big)? from_big_endian(value): from_little_endian(value);
}

endian get_native_endian_at_runtime() noexcept;

} // namespace stiffer

#pragma GCC visibility pop

#endif // STIFFER_ENDIAN_HPP
