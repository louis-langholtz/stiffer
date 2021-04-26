//
//  details.hpp
//  library
//
//  Created by Louis D. Langholtz on 4/1/21.
//

#ifndef STIFFER_DETAILS_HPP
#define STIFFER_DETAILS_HPP

#include <algorithm> // for std::sort
#include <cstring> // for std::memcpy
#include <istream>
#include <ostream>

#include "stiffer.hpp"

namespace stiffer::details {

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

/// Read function for reading a count of elements into a supporting type.
/// @note A supporting type is one that provides a <code>reserve(std::size_t)</code> member
///   function, a type alias of <code>value_type</code>, and a <code>push_back(value_type)</code>
///   member function. For example, a <code>std::vector</code>.
template <typename T>
auto read(std::istream& stream, endian from_order, std::size_t count)
-> decltype(T{}.reserve(0u), T{}.push_back(std::declval<typename T::value_type>()), T{})
{
    using element_type = typename T::value_type;
    T elements;
    elements.reserve(count);
    for (auto i = static_cast<decltype(count)>(0); i < count; ++i) {
        const auto element = from_endian(read<element_type>(stream), from_order);
        if (!stream.good()) {
            throw std::runtime_error(std::string("can't read data for element number ") + std::to_string(i));
        }
        elements.push_back(element);
    }
    return elements;
}

template <typename T>
void write_field_data(std::ostream& stream, const field_value& field, endian to_order)
{
    for (auto&& e: std::get<T>(field)) {
        details::write(stream, to_endian(e, to_order));
    }
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
field_value get_field_value(std::istream& stream, const T& field, endian from_order)
{
    if (!is_value_field(field)) {
        const auto offset = from_endian(field.value_offset, from_order);
        stream.seekg(offset);
        if (!stream.good()) {
            throw std::runtime_error(std::string("can't seek to offet ")
                                     + std::to_string(offset));
        }
    }
    switch (field.type) {
    case byte_field_type:
        return is_value_field(field)?
        get<byte_array>(field.value_offset, from_order, field.count):
        read<byte_array>(stream, from_order, field.count);
    case ascii_field_type:
        return is_value_field(field)?
        get<ascii_array>(field.value_offset, from_order, field.count):
        read<ascii_array>(stream, from_order, field.count);
    case short_field_type:
        return is_value_field(field)?
        get<short_array>(field.value_offset, from_order, field.count):
        read<short_array>(stream, from_order, field.count);
    case long_field_type:
        return is_value_field(field)?
        get<long_array>(field.value_offset, from_order, field.count):
        read<long_array>(stream, from_order, field.count);
    case sbyte_field_type:
        return is_value_field(field)?
        get<sbyte_array>(field.value_offset, from_order, field.count):
        read<sbyte_array>(stream, from_order, field.count);
    case undefined_field_type:
        return is_value_field(field)?
        get<undefined_array>(field.value_offset, from_order, field.count):
        read<undefined_array>(stream, from_order, field.count);
    case sshort_field_type:
        return is_value_field(field)?
        get<sshort_array>(field.value_offset, from_order, field.count):
        read<sshort_array>(stream, from_order, field.count);
    case float_field_type:
        return is_value_field(field)?
        get<float_array>(field.value_offset, from_order, field.count):
        read<float_array>(stream, from_order, field.count);
    case slong_field_type:
        return is_value_field(field)?
        get<slong_array>(field.value_offset, from_order, field.count):
        read<slong_array>(stream, from_order, field.count);
    case long8_field_type:
        return is_value_field(field)?
        get<long8_array>(field.value_offset, from_order, field.count):
        read<long8_array>(stream, from_order, field.count);
    case slong8_field_type:
        return is_value_field(field)?
        get<slong8_array>(field.value_offset, from_order, field.count):
        read<slong8_array>(stream, from_order, field.count);
    case ifd8_field_type:
        return is_value_field(field)?
        get<ifd8_array>(field.value_offset, from_order, field.count):
        read<ifd8_array>(stream, from_order, field.count);
    case rational_field_type:
        return read<rational_array>(stream, from_order, field.count);
    case srational_field_type:
        return read<srational_array>(stream, from_order, field.count);
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
image_file_directory get_ifd(std::istream& stream, std::size_t at, endian from_order)
{
    field_value_map field_map;
    stream.seekg(at);
    if (!stream.good()) {
        throw std::runtime_error("can't seek to given offet");
    }
    const auto num_fields = from_endian(read<directory_count>(stream), from_order);
    if (!stream.good()) {
        throw std::runtime_error("can't read directory count");
    }
    auto fields = read<field_entries>(stream, from_order, num_fields);
    const auto next_ifd_offset = read<file_offset>(stream);
    if (!stream.good()) {
        throw std::runtime_error("can't read next image file directory offset");
    }
    std::sort(fields.begin(), fields.end(), seek_less_than<typename field_entries::value_type>);
    for (auto&& field: fields) {
        field_map[field.tag] = get_field_value(stream, field, from_order);
    }
    return image_file_directory{field_map, from_endian(next_ifd_offset, from_order)};
}

} // namespace stiffer::details

#endif /* STIFFER_DETAILS_HPP */
