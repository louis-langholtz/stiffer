//
//  stiffer.hpp
//  stiffer
//
//  Created by Louis D. Langholtz on 3/11/21.
//

#ifndef STIFFER_HPP
#define STIFFER_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <istream>
#include <ostream>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "endian.hpp"
#include "rational.hpp"
#include "srational.hpp"
#include "file_version.hpp"

/* The classes below are exported */
#pragma GCC visibility push(default)

namespace stiffer {

/// Converts the given value to the value as the underlying type.
/// @note This is like <code>std::to_underlying</code> slated for C++23.
template <typename T>
constexpr std::underlying_type_t<T> to_underlying(T value) noexcept
{
    return static_cast<std::underlying_type_t<T>>(value);
}

template <typename ReturnType, typename GivenType>
auto to_vector(const std::vector<GivenType>& values) -> decltype(ReturnType{GivenType{}}, std::vector<ReturnType>{})
{
    std::vector<ReturnType> result;
    result.reserve(size(values));
    result.assign(begin(values), end(values));
    return result;
}

template <typename T>
std::enable_if_t<std::is_trivially_copyable_v<T>, T> read(std::istream& stream)
{
    auto value = T{};
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

template <typename T>
std::enable_if_t<std::is_trivially_copyable_v<T>, void> write(std::ostream& stream, const T& value)
{
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

enum class field_tag: std::uint16_t {};

template <int N>
constexpr std::enable_if_t<N >= 0 && N < std::numeric_limits<std::underlying_type_t<field_tag>>::max(), field_tag>
to_tag()
{
    return static_cast<field_tag>(N);
}

enum class endian_key_t: std::uint16_t;

constexpr auto little_endian_key = endian_key_t{0x4949u};
constexpr auto big_endian_key = endian_key_t{0x4D4Du};

constexpr std::optional<endian> find_endian(endian_key_t byte_order)
{
    switch (byte_order) {
    case little_endian_key: return {endian::little};
    case big_endian_key: return {endian::big};
    }
    return {};
}

constexpr endian_key_t get_endian_key(endian value)
{
    switch (value) {
    case endian::little: return little_endian_key;
    case endian::big: break;
    }
    return big_endian_key;
}

enum class field_type: std::uint16_t {};

constexpr auto byte_field_type = field_type(1u);
constexpr auto ascii_field_type = field_type(2u);
constexpr auto short_field_type = field_type(3u);
constexpr auto long_field_type = field_type(4u);
constexpr auto rational_field_type = field_type(5u);
constexpr auto sbyte_field_type = field_type(6u);
constexpr auto undefined_field_type = field_type(7u);
constexpr auto sshort_field_type = field_type(8u);
constexpr auto slong_field_type = field_type(9u);
constexpr auto srational_field_type = field_type(10u);
constexpr auto float_field_type = field_type(11u);
constexpr auto double_field_type = field_type(12u);
constexpr auto ifd_field_type = field_type(13u);
constexpr auto long8_field_type = field_type{16u}; /// BigTIFF type
constexpr auto slong8_field_type = field_type{17u}; /// BigTIFF type
constexpr auto ifd8_field_type = field_type{18u}; /// BigTIFF type

/// Undefined element.
/// @note This is a helper type to help distinguish an undefined element from other
///   single byte integer value types.
enum class undefined_element: std::uint8_t {};
static_assert(sizeof(undefined_element) == 1u, "undefined_element size must be 1 byte");

/// IFD element.
/// @note This is a helper type to help distinguish an IFD element from other
///   4-byte integer value types.
enum class ifd_element: std::uint32_t {};
static_assert(sizeof(ifd_element) == 4u, "ifd_element size must be 4 bytes");

/// IFD8 element.
/// @note This is a helper type to help distinguish an IFD8 element from other
///   8-byte integer value types.
enum class ifd8_element: std::uint64_t {};
static_assert(sizeof(ifd8_element) == 8u, "ifd8_element size must be 8 bytes");

using byte_array = std::vector<std::uint8_t>;
using ascii_array = std::string;
using short_array = std::vector<std::uint16_t>;
using long_array = std::vector<std::uint32_t>;
using rational_array = std::vector<rational>;
using sbyte_array = std::vector<std::int8_t>;
using undefined_array = std::vector<undefined_element>;
using sshort_array = std::vector<std::int16_t>;
using slong_array = std::vector<std::int32_t>;
using srational_array = std::vector<srational>;
using float_array = std::vector<float>;
using double_array = std::vector<double>;
using ifd_array = std::vector<ifd_element>;
using long8_array = std::vector<std::uint64_t>;
using slong8_array = std::vector<std::int64_t>;
using ifd8_array = std::vector<ifd8_element>;

using unrecognized_field_value = std::tuple<field_type,std::size_t,undefined_array>;

template <typename T> struct to_field_type;
template <> struct to_field_type<byte_array> { static const auto value = byte_field_type; };
template <> struct to_field_type<ascii_array> { static const auto value = ascii_field_type; };
template <> struct to_field_type<short_array> { static const auto value = short_field_type; };
template <> struct to_field_type<long_array> { static const auto value = long_field_type; };
template <> struct to_field_type<rational_array> { static const auto value = rational_field_type; };
template <> struct to_field_type<sbyte_array> { static const auto value = sbyte_field_type; };
template <> struct to_field_type<undefined_array> { static const auto value = undefined_field_type; };
template <> struct to_field_type<sshort_array> { static const auto value = sshort_field_type; };
template <> struct to_field_type<slong_array> { static const auto value = slong_field_type; };
template <> struct to_field_type<srational_array> { static const auto value = srational_field_type; };
template <> struct to_field_type<float_array> { static const auto value = float_field_type; };
template <> struct to_field_type<double_array> { static const auto value = double_field_type; };
template <> struct to_field_type<ifd_array> { static const auto value = ifd_field_type; };
template <> struct to_field_type<long8_array> { static const auto value = long8_field_type; };
template <> struct to_field_type<slong8_array> { static const auto value = slong8_field_type; };
template <> struct to_field_type<ifd8_array> { static const auto value = ifd8_field_type; };

using field_value = std::variant<
    unrecognized_field_value,
    byte_array,
    ascii_array,
    short_array,
    long_array,
    rational_array,
    sbyte_array,
    undefined_array,
    sshort_array,
    slong_array,
    srational_array,
    float_array,
    double_array,
    ifd_array,
    long8_array,
    slong8_array,
    ifd8_array
>;

template <typename T>
std::enable_if_t<std::is_unsigned_v<T>, std::vector<T>> to_vector(const field_value& value)
{
    if (const auto values = std::get_if<long8_array>(&value); values) {
        return to_vector<T>(*values);
    }
    if (const auto values = std::get_if<long_array>(&value); values) {
        return to_vector<T>(*values);
    }
    if (const auto values = std::get_if<short_array>(&value); values) {
        return to_vector<T>(*values);
    }
    if (const auto values = std::get_if<byte_array>(&value); values) {
        return to_vector<T>(*values);
    }
    throw std::invalid_argument("not an integral array type");
}

constexpr field_type get_field_type(const field_value& value)
{
    static_assert(std::variant_size_v<field_value> == 17u, "switch setup for 17 types");
    switch (value.index()) {
    case 0: return std::get<field_type>(std::get<0>(value));
    case 1: return to_field_type<std::decay_t<decltype(std::get<1>(value))>>::value;
    case 2: return to_field_type<std::decay_t<decltype(std::get<2>(value))>>::value;
    case 3: return to_field_type<std::decay_t<decltype(std::get<3>(value))>>::value;
    case 4: return to_field_type<std::decay_t<decltype(std::get<4>(value))>>::value;
    case 5: return to_field_type<std::decay_t<decltype(std::get<5>(value))>>::value;
    case 6: return to_field_type<std::decay_t<decltype(std::get<6>(value))>>::value;
    case 7: return to_field_type<std::decay_t<decltype(std::get<7>(value))>>::value;
    case 8: return to_field_type<std::decay_t<decltype(std::get<8>(value))>>::value;
    case 9: return to_field_type<std::decay_t<decltype(std::get<9>(value))>>::value;
    case 10: return to_field_type<std::decay_t<decltype(std::get<10>(value))>>::value;
    case 11: return to_field_type<std::decay_t<decltype(std::get<11>(value))>>::value;
    case 12: return to_field_type<std::decay_t<decltype(std::get<12>(value))>>::value;
    case 13: return to_field_type<std::decay_t<decltype(std::get<13>(value))>>::value;
    case 14: return to_field_type<std::decay_t<decltype(std::get<14>(value))>>::value;
    case 15: return to_field_type<std::decay_t<decltype(std::get<15>(value))>>::value;
    case 16: return to_field_type<std::decay_t<decltype(std::get<16>(value))>>::value;
    }
    return field_type(0);
}

constexpr std::size_t size(const field_value& value)
{
    static_assert(std::variant_size_v<field_value> == 17u, "switch setup for 17 types");
    switch (value.index()) {
    case 0: return std::get<std::size_t>(std::get<0>(value));
    case 1: return std::size(std::get<1>(value));
    case 2: return std::size(std::get<2>(value));
    case 3: return std::size(std::get<3>(value));
    case 4: return std::size(std::get<4>(value));
    case 5: return std::size(std::get<5>(value));
    case 6: return std::size(std::get<6>(value));
    case 7: return std::size(std::get<7>(value));
    case 8: return std::size(std::get<8>(value));
    case 9: return std::size(std::get<9>(value));
    case 10: return std::size(std::get<10>(value));
    case 11: return std::size(std::get<11>(value));
    case 12: return std::size(std::get<12>(value));
    case 13: return std::size(std::get<13>(value));
    case 14: return std::size(std::get<14>(value));
    case 15: return std::size(std::get<15>(value));
    case 16: return std::size(std::get<16>(value));
    }
    return 0u;
}

const char* to_string(field_type value);

constexpr std::size_t to_bytesize(field_type value)
{
    switch (value) {
    case byte_field_type: return 1u;
    case ascii_field_type: return 1u;
    case short_field_type: return 2u;
    case long_field_type: return 4u;
    case rational_field_type: return 8u;
    case sbyte_field_type: return 1u;
    case undefined_field_type: return 1u;
    case sshort_field_type: return 2u;
    case slong_field_type: return 4u;
    case srational_field_type: return 8u;
    case float_field_type: return 4u;
    case double_field_type: return 8u;
    case long8_field_type: return 8u;
    case slong8_field_type: return 8u;
    case ifd8_field_type: return 8u;
    }
    return 0u;
}

using field_value_map = std::map<field_tag, field_value>;
using field_value_entry = field_value_map::value_type;

struct field_definition
{
    using default_fn = field_value (*)(const field_value_map& value);

    const char *name = nullptr;
    std::uint32_t types = 0u; /// Bit set of types
    default_fn defaulter = nullptr;
};

using field_definition_map = std::map<field_tag, field_definition>;

using field_definition_entry = field_definition_map::value_type;

inline field_value get_short_array_0(const field_value_map&)
{
    return short_array{0u};
}

inline field_value get_short_array_1(const field_value_map&)
{
    return short_array{1u};
}

inline field_value get_short_array_2(const field_value_map&)
{
    return short_array{2u};
}

inline field_value get_long_array_0(const field_value_map&)
{
    return long_array{0u};
}

inline field_value get_long_array_max(const field_value_map&)
{
    return long_array{static_cast<std::uint32_t>(-1)};
}

void add_defaults(field_value_map& fields, const field_definition_map& definitions);

template <typename M>
const typename M::mapped_type* find(const M& fields, const typename M::key_type& key)
{
    if (const auto it = fields.find(key); it != fields.end()) {
        return &it->second;
    }
    return nullptr;
}

field_value get(const field_value_map& fields, field_tag tag,
                const field_value& fallback = {});
field_value get(const field_definition_map& definitions, field_tag tag,
                const field_value_map& values);

inline
field_value get(const field_value_map& fields, field_tag tag, const field_definition_map& definitions)
{
    return get(fields, tag, get(definitions, tag, fields));
}

using intmax_t = std::int64_t;
using uintmax_t = std::uint64_t;

uintmax_t get_unsigned_front(const field_value& result);

file_version to_file_version(std::uint16_t value);
std::uint16_t to_file_version_key(file_version value);

struct file_context
{
    std::size_t first_ifd_offset;
    endian byte_order;
    file_version version;
};

file_context get_file_context(std::istream& is);

struct image_file_directory
{
    field_value_map fields;
    std::size_t next_image;
};

void decompress_packed_bits();

image_file_directory get_image_file_directory(std::istream& is, std::size_t at, endian byte_order,
                                              file_version version);

} // namespace stiffer

#pragma GCC visibility pop
#endif
