#ifndef STIFFER_ENDIAN_HPP
#define STIFFER_ENDIAN_HPP

#include <cstdint>
#include <ostream>

/* The classes below are exported */
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
constexpr std::enable_if_t<std::is_unsigned_v<T> && (sizeof(T) > 1u), T> byte_swap(T value)
{
    auto result = T{};
    for (auto i = 0u; i < sizeof(value); ++i) {
        result <<= 8;
        result |= (value & 0xFFu);
        value >>= 8;
    }
    return result;
}

template <typename T>
constexpr std::enable_if_t<std::is_unsigned_v<T> && (sizeof(T) == 1u), T> byte_swap(T value)
{
    return value;
}

template <typename T>
constexpr std::enable_if_t<!std::is_unsigned_v<T> && std::is_integral_v<T>, T> byte_swap(T value)
{
    return static_cast<T>(byte_swap(static_cast<std::make_unsigned_t<T>>(value)));
}

inline float byte_swap(const float& value)
{
    const auto integral_value = *reinterpret_cast<const std::uint32_t *>(&value);
    const auto swapped = byte_swap(integral_value);
    return *static_cast<const float*>(static_cast<const void*>(&swapped));
}

inline double byte_swap(const double& value)
{
    const auto integral_value = *reinterpret_cast<const std::uint64_t *>(&value);
    const auto swapped = byte_swap(integral_value);
    return *static_cast<const double*>(static_cast<const void*>(&swapped));
}

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

} // namespace stiffer

#pragma GCC visibility pop

#endif // STIFFER_ENDIAN_HPP
