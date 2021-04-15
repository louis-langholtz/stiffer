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

#include "stiffer.hpp"

namespace stiffer::details {

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

template <typename T>
constexpr bool is_value_field(const T& field)
{
    return (field.count == 0u) || (to_bytesize(field.type) <= (sizeof(field.value_offset) / field.count));
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

} // namespace stiffer::details

#endif /* STIFFER_DETAILS_HPP */
