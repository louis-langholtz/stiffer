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

} // namespace

std::ostream& operator<< (std::ostream& os, file_version value)
{
    switch (value) {
    case file_version::classic:
        os << "classic";
        return os;
    case file_version::bigtiff:
        os << "bigtiff";
        return os;
    }
    return os;
}

const char* field_type_to_string(field_type value)
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
        const auto offset = read<std::uint32_t>(is, *endian_found, false);
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
        const auto offset = read<std::uint64_t>(is, *endian_found, false);
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
        return to_size_array(*values);
    }
    if (const auto values = std::get_if<long_array>(&value); values) {
        return to_size_array(*values);
    }
    if (const auto values = std::get_if<short_array>(&value); values) {
        return to_size_array(*values);
    }
    if (const auto values = std::get_if<byte_array>(&value); values) {
        return to_size_array(*values);
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
    field_offset value_offset; /// Offset in bytes to the first value.
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

constexpr bool is_value_field(const field_entry& field)
{
    return field_type_to_bytesize(field.type) <= (sizeof(field_entry::value_offset) / field.count);
}

bool seek_less_than(const field_entry& lhs, const field_entry& rhs)
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

} // namespace

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order)
{
    field_value_map field_map;
    in.seekg(at);
    if (!in.good()) {
        throw std::runtime_error("can't seek to given offet");
    }
    const auto num_fields = read<std::uint16_t>(in, byte_order);
    auto fields = read<field_entries>(in, byte_order, num_fields);
    const auto next_ifd_offset = read<std::uint32_t>(in, byte_order);
    std::sort(fields.begin(), fields.end(), seek_less_than);
    for (auto&& field: fields) {
        field_map[field.tag] = get_field_value(in, field, byte_order);
    }
    return image_file_directory{field_map, next_ifd_offset};
}

} // namespace classic

namespace bigtiff {

namespace {

#pragma pack(push, 1)

struct field_entry {
    field_tag tag;
    field_type type;
    field_count count; /// Count of the indicated type.
    field_offset value_offset; /// Offset in bytes to the first value.
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

constexpr bool is_value_field(const field_entry& field)
{
    return field_type_to_bytesize(field.type) <= (sizeof(field_entry::value_offset) / field.count);
}

bool seek_less_than(const field_entry& lhs, const field_entry& rhs)
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

} // namespace

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order)
{
    field_value_map field_map;
    in.seekg(at);
    if (!in.good()) {
        throw std::runtime_error("can't seek to given offet");
    }
    const auto num_fields = read<std::uint64_t>(in, byte_order);
    auto fields = read<field_entries>(in, byte_order, num_fields);
    const auto next_ifd_offset = read<std::uint64_t>(in, byte_order);
    std::sort(fields.begin(), fields.end(), seek_less_than);
    for (auto&& field: fields) {
        field_map[field.tag] = get_field_value(in, field, byte_order);
    }
    return image_file_directory{field_map, next_ifd_offset};
}

} // namespace bigtiff

} // namespace stiffer

namespace stiffer::v6 {

namespace {

constexpr auto new_subfile_type_tag = field_tag{254u};
constexpr auto subfile_type_tag = field_tag{255u};
constexpr auto image_width_tag = field_tag{256u};
constexpr auto image_length_tag = field_tag{257u};
constexpr auto bits_per_sample_tag = field_tag{258u};
constexpr auto compression_tag = field_tag{259u};
constexpr auto photometric_interpretation_tag = field_tag{262u};
constexpr auto threshholding_tag = field_tag{263u};
constexpr auto cell_width_tag = field_tag{264u};
constexpr auto cell_length_tag = field_tag{265u};
constexpr auto fill_order_tag = field_tag{266u};
constexpr auto document_name_tag = field_tag{269u};
constexpr auto image_description_tag = field_tag{270u};
constexpr auto make_tag = field_tag{271u};
constexpr auto model_tag = field_tag{272u};
constexpr auto strip_offsets_tag = field_tag{273u};
constexpr auto orientation_tag = field_tag{274u};
constexpr auto samples_per_pixel_tag = field_tag{277u};
constexpr auto rows_per_strip_tag = field_tag{278u};
constexpr auto strip_byte_counts_tag = field_tag{279u};
constexpr auto min_sample_value_tag = field_tag{280u};
constexpr auto max_sample_value_tag = field_tag{281u};
constexpr auto x_resolution_tag = field_tag{282u};
constexpr auto y_resolution_tag = field_tag{283u};
constexpr auto planar_configuration_tag = field_tag{284u};
constexpr auto page_name_tag = field_tag{285u};
constexpr auto x_position_tag = field_tag{286u};
constexpr auto y_position_tag = field_tag{287u};
constexpr auto free_offsets_tag = field_tag{288u};
constexpr auto free_byte_counts_tag = field_tag{289u};
constexpr auto gray_response_unit_tag = field_tag{290u};
constexpr auto gray_response_curve_tag = field_tag{291u};
constexpr auto t4_options_tag = field_tag{292u};
constexpr auto t6_options_tag = field_tag{293u};
constexpr auto resolution_unit_tag = field_tag{296u};
constexpr auto page_number_tag = field_tag{297u};
constexpr auto software_tag = field_tag{305u};
constexpr auto date_time_tag = field_tag{306u};
constexpr auto artist_tag = field_tag{315u};
constexpr auto host_computer_tag = field_tag{316u};
constexpr auto color_map_tag = field_tag{320u};
constexpr auto tile_width_tag = field_tag{322u};
constexpr auto tile_length_tag = field_tag{323u};
constexpr auto tile_offsets_tag = field_tag{324u};
constexpr auto tile_byte_counts_tag = field_tag{325u};
constexpr auto sub_ifds_tag = field_tag{330u};
constexpr auto extra_samples_tag = field_tag{338u};
constexpr auto copyright_tag = field_tag{33432u};

field_value bits_per_sample_value_default(const field_value_map& fields)
{
    return short_array(get_samples_per_pixel(fields), 1u);
}

field_value max_sample_value_default(const field_value_map& fields)
{
    /*, Default is 2**(BitsPerSample) - 1 */
    const auto& definitions = get_definitions();
    const auto bits_per_sample = get(fields, bits_per_sample_tag, definitions);
    if (const auto entries = std::get_if<short_array>(&bits_per_sample); entries) {
        auto result = short_array{};
        for (auto&& entry: *entries) {
            result.push_back((0x2u << entry) - 1u);
        }
        return {result};
    }
    return {};
}

constexpr auto ascii_field_bit = (static_cast<std::uint32_t>(0x1u) << to_underlying(ascii_field_type));
constexpr auto short_field_bit = (static_cast<std::uint32_t>(0x1u) << to_underlying(short_field_type));
constexpr auto long_field_bit = (static_cast<std::uint32_t>(0x1u) << to_underlying(long_field_type));
constexpr auto rational_field_bit = (static_cast<std::uint32_t>(0x1u) << to_underlying(rational_field_type));
constexpr auto ifd_field_bit = (static_cast<std::uint32_t>(0x1u) << to_underlying(ifd_field_type));

} // namespace

const field_definition_map& get_definitions()
{
    static const auto definitions = field_definition_map{
        {artist_tag, {"Artist", ascii_field_bit}},
        {bits_per_sample_tag, {"BitsPerSample", short_field_bit, bits_per_sample_value_default}},
        {cell_length_tag, {"CellLength", short_field_bit}},
        {cell_width_tag, {"CellWidth", short_field_bit}},
        {color_map_tag, {"ColorMap", short_field_bit}},
        {compression_tag, {"Compression", short_field_bit, get_short_array_1}},
        {copyright_tag, {"Copyright", ascii_field_bit}},
        {date_time_tag, {"DateTime", ascii_field_bit}},
        {document_name_tag, {"DocumentName", ascii_field_bit}},
        {extra_samples_tag, {"ExtraSamples", short_field_bit}},
        {fill_order_tag, {"FillOrder", short_field_bit, get_short_array_1}},
        {free_byte_counts_tag, {"FreeByteCounts", long_field_bit}},
        {free_offsets_tag, {"FreeOffsets", long_field_bit}},
        {gray_response_curve_tag, {"GrayResponseCurve", short_field_bit}},
        {gray_response_unit_tag, {"GrayResponseUnit", short_field_bit, get_short_array_2}},
        {host_computer_tag, {"HostComputer", ascii_field_bit}},
        {image_description_tag, {"ImageDescription", ascii_field_bit}},
        {image_length_tag, {"ImageLength", short_field_bit|long_field_bit}},
        {image_width_tag, {"ImageWidth", short_field_bit|long_field_bit}},
        {make_tag, {"Make", ascii_field_bit}},
        {max_sample_value_tag, {"MaxSampleValue", short_field_bit, max_sample_value_default}},
        {min_sample_value_tag, {"MinSampleValue", short_field_bit, get_short_array_0}},
        {model_tag, {"Model", ascii_field_bit}},
        {new_subfile_type_tag, {"NewSubfileType", long_field_bit, get_long_array_0}},
        {orientation_tag, {"Orientation", short_field_bit, get_short_array_1}},
        {page_name_tag, {"PageName", ascii_field_bit}},
        {page_number_tag, {"PageNumber", short_field_bit}},
        {photometric_interpretation_tag, {"PhotometricInterpretation", short_field_bit, get_short_array_1}},
        {planar_configuration_tag, {"PlanarConfiguration", short_field_bit, get_short_array_1}},
        {resolution_unit_tag, {"ResolutionUnit", short_field_bit, get_short_array_2}},
        {rows_per_strip_tag, {"RowsPerStrip", short_field_bit|long_field_bit, get_long_array_max}},
        {samples_per_pixel_tag, {"SamplesPerPixel", short_field_bit, get_short_array_1}},
        {software_tag, {"Software", ascii_field_bit}},
        {strip_byte_counts_tag, {"StripByteCounts", short_field_bit|long_field_bit}},
        {strip_offsets_tag, {"StripOffsets", short_field_bit|long_field_bit}},
        {subfile_type_tag, {"SubfileType", short_field_bit}},
        {sub_ifds_tag, {"SubIFDs", long_field_bit|ifd_field_bit}},
        {t4_options_tag, {"T4Options", long_field_bit, get_long_array_0}},
        {t6_options_tag, {"T6Options", long_field_bit, get_long_array_0}},
        {threshholding_tag, {"Threshholding", short_field_bit, get_short_array_1}},
        {tile_byte_counts_tag, {"TileByteCounts", short_field_bit|long_field_bit}},
        {tile_length_tag, {"TileLength", short_field_bit|long_field_bit}},
        {tile_offsets_tag, {"TileOffsets", long_field_bit}},
        {tile_width_tag, {"TileWidth", short_field_bit|long_field_bit}},
        {x_position_tag, {"XPosition", rational_field_bit}},
        {x_resolution_tag, {"XResolution", rational_field_bit}},
        {y_position_tag, {"YPosition", rational_field_bit}},
        {y_resolution_tag, {"YResolution", rational_field_bit}},
    };
    return definitions;
}

std::size_t get_compression(const field_value_map& fields)
{
    return get_unsigned_front(fields, compression_tag);
}

std::size_t get_image_length(const field_value_map& fields)
{
    return get_unsigned_front(fields, image_length_tag);
}

std::size_t get_image_width(const field_value_map& fields)
{
    return get_unsigned_front(fields, image_width_tag);
}

std::size_t get_samples_per_pixel(const field_value_map& fields)
{
    return get_unsigned_front(fields, samples_per_pixel_tag);
}

std::size_t get_rows_per_strip(const field_value_map& fields)
{
    return get_unsigned_front(fields, rows_per_strip_tag);
}

std::size_t get_orientation(const field_value_map& fields)
{
    return get_unsigned_front(fields, orientation_tag);
}

std::size_t get_photometric_interpretation(const field_value_map& fields)
{
    return get_unsigned_front(fields, photometric_interpretation_tag);
}

std::size_t get_planar_configuraion(const field_value_map& fields)
{
    return get_unsigned_front(fields, planar_configuration_tag);
}

std::size_t get_resolution_unit(const field_value_map& fields)
{
    return get_unsigned_front(fields, resolution_unit_tag);
}

std::size_t get_x_resolution(const field_value_map& fields)
{
    return get_unsigned_front(fields, x_resolution_tag);
}

std::size_t get_y_resolution(const field_value_map& fields)
{
    return get_unsigned_front(fields, y_resolution_tag);
}

std::size_t get_tile_length(const field_value_map& fields)
{
    return get_unsigned_front(fields, tile_length_tag);
}

std::size_t get_tile_width(const field_value_map& fields)
{
    return get_unsigned_front(fields, tile_width_tag);
}

std::size_t get_strip_byte_count(const field_value_map& fields, std::size_t index)
{
    const auto found = find(fields, strip_byte_counts_tag);
    if (!found) {
        throw std::invalid_argument("strip byte counts entry missing from ifd");
    }
    const auto longs = std::get_if<long_array>(found);
    if (longs) return longs->at(index);
    const auto shorts = std::get_if<short_array>(found);
    if (shorts) return shorts->at(index);
    throw std::invalid_argument("strip byte counts entry type not long nor short");
}

std::size_t get_strip_offset(const field_value_map& fields, std::size_t index)
{
    const auto found = find(fields, strip_offsets_tag);
    if (!found) {
        throw std::invalid_argument("strip offsets entry missing from ifd");
    }
    const auto longs = std::get_if<long_array>(found);
    if (longs) return longs->at(index);
    const auto shorts = std::get_if<short_array>(found);
    if (shorts) return shorts->at(index);
    throw std::invalid_argument("strip offsets entry type not long nor short");
}

std::size_t get_tile_byte_count(const field_value_map& fields, std::size_t index)
{
    const auto found = find(fields, tile_byte_counts_tag);
    if (!found) {
        throw std::invalid_argument("tile byte counts entry missing from ifd");
    }
    const auto longs = std::get_if<long_array>(found);
    if (longs) return longs->at(index);
    const auto shorts = std::get_if<short_array>(found);
    if (shorts) return shorts->at(index);
    throw std::invalid_argument("tile byte counts entry type not long nor short");
}

std::size_t get_tile_offset(const field_value_map& fields, std::size_t index)
{
    const auto found = find(fields, tile_offsets_tag);
    if (!found) {
        throw std::invalid_argument("tile offsets entry missing from ifd");
    }
    const auto longs = std::get_if<long_array>(found);
    if (longs) return longs->at(index);
    const auto shorts = std::get_if<short_array>(found);
    if (shorts) return shorts->at(index);
    throw std::invalid_argument("tile offsets entry type not long nor short");
}

field_value get_bits_per_sample(const field_value_map& fields)
{
    return get(fields, bits_per_sample_tag, get_definitions());
}

undefined_array read_strip(std::istream& is, const field_value_map& fields, std::size_t index)
{
    auto bytes = undefined_array{};
    const auto byte_count = get_strip_byte_count(fields, index);
    const auto offset = get_strip_offset(fields, index);
    bytes.resize(byte_count);
    is.seekg(offset);
    if (!is.good()) {
        throw std::runtime_error("can't seek to offset");
    }
    is.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!is.good()) {
        throw std::runtime_error("can't read data");
    }
    return bytes;
}

undefined_array read_tile(std::istream& is, const field_value_map& fields, std::size_t index)
{
    auto bytes = undefined_array{};
    const auto byte_count = get_tile_byte_count(fields, index);
    const auto offset = get_tile_offset(fields, index);
    bytes.resize(byte_count);
    is.seekg(offset);
    if (!is.good()) {
        throw std::runtime_error("can't seek to offset");
    }
    is.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!is.good()) {
        throw std::runtime_error("can't read data");
    }
    return bytes;
}

bool has_striped_image(const field_value_map& fields)
{
    const auto bytes_found = find(fields, strip_byte_counts_tag);
    const auto offsets_found = find(fields, strip_byte_counts_tag);
    return bytes_found && offsets_found;
}

bool has_tiled_image(const field_value_map& fields)
{
    const auto bytes_found = find(fields, tile_byte_counts_tag);
    const auto offsets_found = find(fields, tile_offsets_tag);
    return bytes_found && offsets_found;
}

image read_image(std::istream& in, const field_value_map& fields)
{
    if (has_striped_image(fields)) {
        auto result = image{};
        result.buffer.resize(get_image_width(fields),
                             get_image_length(fields),
                             as_size_array(get_bits_per_sample(fields)));
        result.photometric_interpretation = get_photometric_interpretation(fields);
        result.orientation = get_orientation(fields);
        result.planar_configuration = get_planar_configuraion(fields);
        const auto compression = get_compression(fields);
        const auto max = get_strips_per_image(fields);
        std::size_t offset = 0;
        for (auto i = static_cast<std::size_t>(0); i < max; ++i) {
            const auto strip = read_strip(in, fields, i);
            switch (compression) {
            case 1u:
                std::memcpy(result.buffer.data() + offset, strip.data(), strip.size());
                offset += strip.size();
                break;
            default:
                throw std::invalid_argument(std::string("unable to decode compression " + std::to_string(compression)));
            }
        }
        return result;
    }
    return image{};
}

} // namespace stiffer::version_6
