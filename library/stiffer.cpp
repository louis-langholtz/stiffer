//
//  stiffer.cpp
//  stiffer
//
//  Created by Louis D. Langholtz on 3/11/21.
//

#ifdef _MSC_VER
#include <winnt.h>
#endif

#include <algorithm>
#include <exception>
#include <ios>
#include <optional>

#include "stiffer.hpp"

namespace stiffer {

namespace {

constexpr auto little_endian_key = std::uint16_t{0x4949u};
constexpr auto big_endian_key = std::uint16_t{0x4D4Du};
constexpr auto classic_version_number = std::uint16_t{42};
constexpr auto bigtiff_version_number = std::uint16_t{43};

constexpr std::optional<endian> find_endian(std::uint16_t byte_order)
{
    switch (byte_order) {
    case little_endian_key: return {endian::little};
    case big_endian_key: return {endian::big};
    }
    return {};
}

file_version to_file_version(std::uint16_t number)
{
    switch (number) {
    case classic_version_number: return file_version::classic;
    case bigtiff_version_number: return file_version::bigtiff;
    }
    throw std::invalid_argument("unrecognized version number");
}

template <typename T>
std::enable_if_t<std::is_trivially_copyable_v<T>, T> read(std::istream& in, endian from_order,
                                                          bool throw_on_fail = true)
{
    auto element = T{};
    in.read(reinterpret_cast<char*>(&element), sizeof(element));
    if (throw_on_fail && !in.good()) {
        throw std::runtime_error("can't read data");
    }
    return from_endian(element, from_order);
}

template <typename T>
T read(std::istream& in, endian from_order, std::size_t count)
{
    using element_type = typename T::value_type;
    T elements;
    elements.reserve(count);
    for (auto i = static_cast<decltype(count)>(0); i < count; ++i) {
        const auto element = read<element_type>(in, from_order, false);
        if (!in.good()) {
            throw std::runtime_error(std::string("can't read data for element number ") + std::to_string(i));
        }
        elements.push_back(element);
    }
    return elements;
}

template <typename T, typename U>
std::enable_if_t<std::is_unsigned_v<U>, T> get(U in, endian from_order, std::size_t count)
{
    T elements;
    elements.reserve(count);
    using element_type = typename T::value_type;
    if constexpr (sizeof(element_type) < sizeof(in)) {
        constexpr auto avail = sizeof(in) / sizeof(element_type);
        const auto max_count = std::min(count, avail);
        for (auto i = static_cast<decltype(count)>(0); i < max_count; ++i) {
            elements.push_back(from_endian(*(reinterpret_cast<const element_type*>(&in) + i), from_order));
        }
        return elements;
    }
    else {
        elements.push_back(from_endian(static_cast<element_type>(in), from_order));
        return elements;
    }
}

template <typename T>
constexpr bool is_value_field(const T& field)
{
    return (field.count == 0u) || (to_bytesize(field.type) <= (sizeof(field.value_offset) / field.count));
}

template <typename T>
field_value get_field_value(std::istream& in, const T& field, endian byte_order)
{
    if (!is_value_field(field)) {
        const auto offset = from_endian(field.value_offset, byte_order);
        in.seekg(offset);
        if (!in.good()) {
            throw std::runtime_error(std::string("can't seek to offet ")
                                     + std::to_string(offset));
        }
    }
    switch (field.type) {
    case byte_field_type:
        return is_value_field(field)?
        get<byte_array>(field.value_offset, byte_order, field.count):
        read<byte_array>(in, byte_order, field.count);
    case ascii_field_type:
        return is_value_field(field)?
        get<ascii_array>(field.value_offset, byte_order, field.count):
        read<ascii_array>(in, byte_order, field.count);
    case short_field_type:
        return is_value_field(field)?
        get<short_array>(field.value_offset, byte_order, field.count):
        read<short_array>(in, byte_order, field.count);
    case long_field_type:
        return is_value_field(field)?
        get<long_array>(field.value_offset, byte_order, field.count):
        read<long_array>(in, byte_order, field.count);
    case sbyte_field_type:
        return is_value_field(field)?
        get<sbyte_array>(field.value_offset, byte_order, field.count):
        read<sbyte_array>(in, byte_order, field.count);
    case undefined_field_type:
        return is_value_field(field)?
        get<undefined_array>(field.value_offset, byte_order, field.count):
        read<undefined_array>(in, byte_order, field.count);
    case sshort_field_type:
        return is_value_field(field)?
        get<sshort_array>(field.value_offset, byte_order, field.count):
        read<sshort_array>(in, byte_order, field.count);
    case float_field_type:
        return is_value_field(field)?
        get<float_array>(field.value_offset, byte_order, field.count):
        read<float_array>(in, byte_order, field.count);
    case slong_field_type:
        return is_value_field(field)?
        get<slong_array>(field.value_offset, byte_order, field.count):
        read<slong_array>(in, byte_order, field.count);
    case long8_field_type:
        return is_value_field(field)?
        get<long8_array>(field.value_offset, byte_order, field.count):
        read<long8_array>(in, byte_order, field.count);
    case slong8_field_type:
        return is_value_field(field)?
        get<slong8_array>(field.value_offset, byte_order, field.count):
        read<slong8_array>(in, byte_order, field.count);
    case ifd8_field_type:
        return is_value_field(field)?
        get<ifd8_array>(field.value_offset, byte_order, field.count):
        read<ifd8_array>(in, byte_order, field.count);
    case rational_field_type:
        return read<rational_array>(in, byte_order, field.count);
    case srational_field_type:
        return read<srational_array>(in, byte_order, field.count);
    }
    auto data = undefined_array{};
    data.resize(sizeof(field.value_offset));
    std::memcpy(data.data(), &field.value_offset, sizeof(field.value_offset));
    return unrecognized_field_value{field.type, field.count, data};
}

template <typename T>
bool seek_less_than(const T& lhs, const T& rhs)
{
    if (is_value_field(lhs) && is_value_field(rhs)) {
        return &lhs < &rhs;
    }
    if (is_value_field(lhs)) {
        return true;
    }
    if (is_value_field(rhs)) {
        return false;
    }
    return lhs.value_offset < rhs.value_offset;
}

template <typename directory_count, typename field_entries, typename file_offset>
image_file_directory get_ifd(std::istream& in, std::size_t at, endian byte_order)
{
    field_value_map field_map;
    in.seekg(at);
    if (!in.good()) {
        throw std::runtime_error("can't seek to given offet");
    }
    const auto num_fields = read<directory_count>(in, byte_order);
    auto fields = read<field_entries>(in, byte_order, num_fields);
    const auto next_ifd_offset = read<file_offset>(in, byte_order);
    std::sort(fields.begin(), fields.end(), seek_less_than<typename field_entries::value_type>);
    for (auto&& field: fields) {
        field_map[field.tag] = get_field_value(in, field, byte_order);
    }
    return image_file_directory{field_map, next_ifd_offset};
}

} // namespace

const char* to_string(field_type value)
{
    switch (value) {
    case byte_field_type: return "byte";
    case ascii_field_type: return "ascii";
    case short_field_type: return "short";
    case long_field_type: return "long";
    case rational_field_type: return "rational";
    case sbyte_field_type: return "sbyte";
    case undefined_field_type: return "undefined";
    case sshort_field_type: return "sshort";
    case slong_field_type: return "slong";
    case srational_field_type: return "srational";
    case float_field_type: return "float";
    case double_field_type: return "double";
    case ifd_field_type: return "ifd";
    case long8_field_type: return "long8";
    case slong8_field_type: return "slong8";
    case ifd8_field_type: return "ifd8";
    default: break;
    }
    return "unrecognized";
}

void add_defaults(field_value_map& fields, const field_definition_map& definitions)
{
    for (auto&& def: definitions) {
        if (def.second.defaulter) {
            if (const auto it = fields.find(def.first); it == fields.end()) {
                fields.insert({def.first, def.second.defaulter(fields)});
            }
        }
    }
}

file_context get_file_context(std::istream& is)
{
    is.seekg(0);
    if (!is.good()) {
        throw std::runtime_error("can't seek to position 0");
    }
    std::uint16_t byte_order{};
    is.read(reinterpret_cast<char*>(&byte_order), sizeof(byte_order));
    if (!is.good()) {
        throw std::runtime_error("can't read byte order");
    }
    const auto endian_found = find_endian(byte_order);
    if (!endian_found) {
        throw std::invalid_argument("unrecognized byte order");
    }
    const auto version_number = read<std::uint16_t>(is, *endian_found, false);
    if (!is.good()) {
        throw std::runtime_error("can't read version number");
    }
    const auto fv = to_file_version(version_number);
    switch (fv) {
    case file_version::classic: {
        const auto offset = read<classic::file_offset>(is, *endian_found, false);
        if (!is.good()) {
            throw std::runtime_error("can't read initial offset");
        }
        return {offset, *endian_found, fv};
    }
    case file_version::bigtiff: {
        const auto offsets_bytesize = read<std::uint16_t>(is, *endian_found, false);
        if (!is.good()) {
            throw std::runtime_error("can't read offsets bytesize");
        }
        if (offsets_bytesize != 8u) {
            throw std::invalid_argument(std::string("unexpected offset bytesize of ")
                                        + std::to_string(offsets_bytesize));
        }
        read<std::uint16_t>(is, *endian_found, false);
        if (!is.good()) {
            throw std::runtime_error("can't read header padding");
        }
        const auto offset = read<bigtiff::file_offset>(is, *endian_found, false);
        if (!is.good()) {
            throw std::runtime_error("can't read initial offset");
        }
        return {offset, *endian_found, fv};
    }
    }
    throw std::invalid_argument("unhandled file version");
}

field_value get(const field_value_map& fields, field_tag tag, const field_value& fallback)
{
    if (const auto it = find(fields, tag); it) {
        return *it;
    }
    return fallback;
}

field_value get(const field_definition_map& definitions, field_tag tag,
                const field_value_map& values)
{
    if (const auto it = definitions.find(tag); it != definitions.end()) {
        if (const auto fn = it->second.defaulter; fn) {
            return {fn(values)};
        }
    }
    return {};
}

std::size_t get_unsigned_front(const field_value& result)
{
    if (result == field_value{}) {
        throw std::invalid_argument("no field value");
    }
    if (const auto values = std::get_if<long_array>(&result); values) return values->at(0);
    if (const auto values = std::get_if<short_array>(&result); values) return values->at(0);
    if (const auto values = std::get_if<byte_array>(&result); values) return values->at(0);
    throw std::invalid_argument("field value not an unsigned integral array type");
}

void decompress_packed_bits()
{
    /*
     * Loop until you get the number of unpacked bytes you are expecting:
     * Read the next source byte into n.
     * If n is between 0 and 127 inclusive, copy the next n+1 bytes literally. Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
     * Else if n is -128, noop.
     * Endloop
     */
    for (;;) {

    }
}

std::vector<std::size_t> as_size_array(const field_value& value)
{
    if (const auto values = std::get_if<long8_array>(&value); values) {
        return to_vector<std::size_t>(*values);
    }
    if (const auto values = std::get_if<long_array>(&value); values) {
        return to_vector<std::size_t>(*values);
    }
    if (const auto values = std::get_if<short_array>(&value); values) {
        return to_vector<std::size_t>(*values);
    }
    if (const auto values = std::get_if<byte_array>(&value); values) {
        return to_vector<std::size_t>(*values);
    }
    throw std::invalid_argument("not an unsigned integral array type");
}

namespace classic {

namespace {

#pragma pack(push, 1)

struct field_entry {
    field_tag tag;
    field_type type;
    field_count count; /// Count of the indicated type.
    file_offset value_offset; /// Offset in bytes to the first value.
};
static_assert(sizeof(field_entry) == 12u, "field_entry size must be 12 bytes");

#pragma pack(pop)

using field_entries = std::vector<field_entry>;

constexpr field_entry byte_swap(const field_entry& value)
{
    return field_entry{
        ::stiffer::byte_swap(value.tag),
        ::stiffer::byte_swap(value.type),
        ::stiffer::byte_swap(value.count),
        value.value_offset // don't byte swap this field yet!
    };
}

} // namespace

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order)
{
    return get_ifd<directory_count, field_entries, file_offset>(in, at, byte_order);
}

} // namespace classic

namespace bigtiff {

namespace {

#pragma pack(push, 1)

struct field_entry {
    field_tag tag;
    field_type type;
    field_count count; /// Count of the indicated type.
    file_offset value_offset; /// Offset in bytes to the first value.
};
static_assert(sizeof(field_entry) == 20u, "field_entry size must be 20 bytes");

#pragma pack(pop)

using field_entries = std::vector<field_entry>;

constexpr field_entry byte_swap(const field_entry& value)
{
    return field_entry{
        ::stiffer::byte_swap(value.tag),
        ::stiffer::byte_swap(value.type),
        ::stiffer::byte_swap(value.count),
        value.value_offset // don't byte swap this field yet!
    };
}

} // namespace

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order)
{
    return get_ifd<directory_count, field_entries, file_offset>(in, at, byte_order);
}

} // namespace bigtiff

} // namespace stiffer
