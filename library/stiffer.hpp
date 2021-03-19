//
//  stiffer.hpp
//  stiffer
//
//  Created by Louis D. Langholtz on 3/11/21.
//

#ifndef STIFFER_HPP
#define STIFFER_HPP

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <istream>
#include <ostream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "endian.hpp"
#include "rational.hpp"
#include "srational.hpp"

/* The classes below are exported */
#pragma GCC visibility push(default)

namespace stiffer {

enum class file_version {
    classic,
    bigtiff,
};

file_version to_file_version(std::uint16_t number);

constexpr auto little_endian_key = std::uint16_t{0x4949u};
constexpr auto big_endian_key = std::uint16_t{0x4D4Du};

using field_tag = std::uint16_t;
using field_type = std::uint16_t;

constexpr auto byte_field_type = field_type(1u);
constexpr auto ascii_field_type = field_type(2u);
constexpr auto short_field_type = field_type(3u);
constexpr auto long_field_type = field_type(4u);
constexpr auto rational_field_type = field_type(5u);
constexpr auto sbyte_field_type = field_type(6);
constexpr auto undefined_field_type = field_type(7);
constexpr auto sshort_field_type = field_type(8);
constexpr auto slong_field_type = field_type(9);
constexpr auto srational_field_type = field_type(10);
constexpr auto float_field_type = field_type(11);
constexpr auto double_type = field_type(12);

/// Undefined element.
/// @note This is a helper type to help distinguish an undefined element from other
///   single byte integer value types.
enum class undefined_element: std::uint8_t {};
static_assert(sizeof(undefined_element) == 1u, "undefined_element size must be 1 byte");

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

template <typename T>
constexpr field_type to_field_type();

template <>
constexpr field_type to_field_type<byte_array>()
{
    return byte_field_type;
}

template <>
constexpr field_type to_field_type<ascii_array>()
{
    return ascii_field_type;
}

using field_value = std::variant<
    std::monostate,
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
    double_array
>;

constexpr field_type to_field_type(const field_value& value)
{
    return static_cast<field_type>(value.index());
}

constexpr std::size_t size(const field_value& value)
{
    switch (value.index()) {
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
    }
    return 0u;
}

const char* field_type_to_string(field_type value);

constexpr std::size_t field_type_to_bytesize(field_type value)
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
    case double_type: return 8u;
    default: break;
    }
    return 1u;
}

inline undefined_element byte_swap(undefined_element value)
{
    return value;
}

inline rational byte_swap(const rational& value)
{
    return rational{byte_swap(value.numerator), byte_swap(value.denominator)};
}

inline srational byte_swap(const srational& value)
{
    return srational{byte_swap(value.numerator), byte_swap(value.denominator)};
}

using field_value_map = std::map<field_tag, field_value>;
using field_value_entry = field_value_map::value_type;

using field_default_fn = field_value (*)(const field_value_map& value);

struct field_definition
{
    const char *name = nullptr;
    std::uint32_t types; /// Bit set of types
    field_default_fn default_fn = nullptr;
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

void add_defaults(field_value_map& map, const field_definition_map& definitions);

template <typename M>
const typename M::mapped_type* find(const M& map, const typename M::key_type& key)
{
    if (const auto it = map.find(key); it != map.end()) {
        return &it->second;
    }
    return nullptr;
}

field_value get(const field_value_map& map, field_tag tag,
                const field_value& fallback = {});
field_value get(const field_definition_map& definitions, field_tag tag,
                const field_value_map& values);

inline
field_value get(const field_value_map& map, field_tag tag, const field_definition_map& definitions)
{
    return get(map, tag, get(definitions, tag, map));
}

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

constexpr std::optional<endian> find_endian(std::uint16_t byte_order)
{
    switch (byte_order) {
    case little_endian_key: return {endian::little};
    case big_endian_key: return {endian::big};
    }
    return {};
}

} // namespace stiffer

namespace stiffer::classic {

image_file_directory get_image_file_directory(std::istream& is, std::size_t at, endian byte_order);

} // stiffer::classic

namespace stiffer::bigtiff {

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order);

} // stiffer::bigtiff

namespace stiffer::v6 {

const field_definition_map& get_definitions();

std::size_t get_compression(const field_value_map& map);
std::size_t get_image_length(const field_value_map& map);
std::size_t get_image_width(const field_value_map& map);
std::size_t get_samples_per_pixel(const field_value_map& map);
std::size_t get_rows_per_strip(const field_value_map& map);

inline std::size_t get_strips_per_image(const field_value_map& map)
{
    const auto rows_per_strip = get_rows_per_strip(map);
    return (get_image_length(map) + rows_per_strip - 1u) / rows_per_strip;
}

std::size_t get_strip_byte_count(const field_value_map& map, std::size_t strip);
std::size_t get_strip_offset(const field_value_map& map, std::size_t strip);

field_value get_bits_per_sample(const field_value_map& map);

undefined_array read_strip(std::istream& is, const image_file_directory& ifd, std::size_t strip);

} // namespace stiffer::version_6

#pragma GCC visibility pop
#endif
