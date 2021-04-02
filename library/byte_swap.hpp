//
//  byte_swap.hpp
//  library
//
//  Created by Louis D. Langholtz on 4/2/21.
//

#ifndef STIFFER_BYTE_SWAP_HPP
#define STIFFER_BYTE_SWAP_HPP

#include <cstdint> // for std::uint32_t, etc
#include <type_traits>

namespace stiffer {

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

template <typename T>
constexpr std::enable_if_t<std::is_enum_v<T>, T> byte_swap(T value)
{
    using type = decltype(value);
    return static_cast<type>(byte_swap(static_cast<std::underlying_type_t<type>>(value)));
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

} // namespace stiffer

#endif /* STIFFER_BYTE_SWAP_HPP */
