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

#include "stiffer.hpp"
#include "classic.hpp"
#include "bigtiff.hpp"
#include "details.hpp"

namespace stiffer {

namespace {

constexpr auto classic_version_number = std::uint16_t{42};
constexpr auto bigtiff_version_number = std::uint16_t{43};

} // namespace

file_version to_file_version(std::uint16_t value)
{
    switch (value) {
    case classic_version_number: return file_version::classic;
    case bigtiff_version_number: return file_version::bigtiff;
    }
    throw std::invalid_argument("unrecognized version number");
}

std::uint16_t to_file_version_key(file_version value)
{
    switch (value) {
    case file_version::bigtiff: return bigtiff_version_number;
    case file_version::classic: break;
    }
    return classic_version_number;
}

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

file_context get_file_context(std::istream& stream)
{
    stream.seekg(0);
    if (!stream.good()) {
        throw std::runtime_error("can't seek to position 0");
    }
    const auto byte_order = read<endian_key_t>(stream);
    if (!stream.good()) {
        throw std::runtime_error("can't read byte order");
    }
    const auto endian_found = find_endian(byte_order);
    if (!endian_found) {
        throw std::invalid_argument("unrecognized byte order");
    }
    const auto version_number = from_endian(read<std::uint16_t>(stream), *endian_found);
    if (!stream.good()) {
        throw std::runtime_error("can't read version number");
    }
    const auto fv = to_file_version(version_number);
    switch (fv) {
    case file_version::classic: {
        const auto offset = from_endian(read<classic::file_offset>(stream), *endian_found);
        if (!stream.good()) {
            throw std::runtime_error("can't read initial offset");
        }
        return {offset, *endian_found, fv};
    }
    case file_version::bigtiff: {
        const auto offsets_bytesize = from_endian(read<std::uint16_t>(stream), *endian_found);
        if (!stream.good()) {
            throw std::runtime_error("can't read offsets bytesize");
        }
        if (offsets_bytesize != 8u) {
            throw std::invalid_argument(std::string("unexpected offset bytesize of ")
                                        + std::to_string(offsets_bytesize));
        }
        from_endian(read<std::uint16_t>(stream), *endian_found);
        if (!stream.good()) {
            throw std::runtime_error("can't read header padding");
        }
        const auto offset = from_endian(read<bigtiff::file_offset>(stream), *endian_found);
        if (!stream.good()) {
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

uintmax_t get_unsigned_front(const field_value& result)
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

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order,
                                              file_version version)
{
    return (version == stiffer::file_version::classic)?
        stiffer::classic::get_image_file_directory(in, at, byte_order):
        stiffer::bigtiff::get_image_file_directory(in, at, byte_order);
}

} // namespace stiffer
