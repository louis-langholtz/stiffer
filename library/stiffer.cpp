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
std::enable_if_t<std::is_unsigned_v<U>, T> get(U in, std::size_t count)
{
    T elements;
    elements.reserve(count);
    using element_type = typename T::value_type;
    if constexpr (sizeof(element_type) < sizeof(in)) {
        constexpr auto num_bits = sizeof(element_type) * 8u;
        constexpr auto mask = (static_cast<std::uint64_t>(1) << num_bits) - 1u;
        constexpr auto avail = sizeof(in) / sizeof(element_type);
        const auto max_count = std::min(count, avail);
        for (auto i = static_cast<decltype(count)>(0); i < max_count; ++i) {
            const auto element = static_cast<element_type>(in & static_cast<decltype(in)>(mask));
            elements.push_back(element);
            in >>= num_bits;
        }
        return elements;
    }
    else {
        constexpr auto avail = sizeof(in) / sizeof(element_type);
        const auto max_count = std::min(count, avail);
        for (auto i = static_cast<decltype(count)>(0); i < max_count; ++i) {
            elements.push_back(static_cast<element_type>(in));
        }
        return elements;
    }
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
    default: break;
    }
    return "unrecognized";
}

void add_defaults(field_value_map& map, const field_definition_map& definitions)
{
    for (auto&& def: definitions) {
        if (def.second.defaulter) {
            if (const auto it = map.find(def.first); it == map.end()) {
                map.insert({def.first, def.second.defaulter(map)});
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

field_value get(const field_value_map& map, field_tag tag, const field_value& fallback)
{
    if (const auto it = find(map, tag); it) {
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
        ::stiffer::byte_swap(value.value_offset)
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

field_value get_field_value(std::istream& in, const field_entry& field, endian byte_order)
{
    if (!is_value_field(field)) {
        in.seekg(field.value_offset);
        if (!in.good()) {
            throw std::runtime_error(std::string("can't seek to offet ")
                                     + std::to_string(field.value_offset));
        }
    }
    switch (field.type) {
    case byte_field_type:
        return is_value_field(field)?
        get<byte_array>(field.value_offset, field.count):
        read<byte_array>(in, byte_order, field.count);
    case ascii_field_type:
        return is_value_field(field)?
        get<ascii_array>(field.value_offset, field.count):
        read<ascii_array>(in, byte_order, field.count);
    case short_field_type:
        return is_value_field(field)?
        get<short_array>(field.value_offset, field.count):
        read<short_array>(in, byte_order, field.count);
    case long_field_type:
        return is_value_field(field)?
        get<long_array>(field.value_offset, field.count):
        read<long_array>(in, byte_order, field.count);
    case sbyte_field_type:
        return is_value_field(field)?
        get<sbyte_array>(field.value_offset, field.count):
        read<sbyte_array>(in, byte_order, field.count);
    case undefined_field_type:
        return is_value_field(field)?
        get<undefined_array>(field.value_offset, field.count):
        read<undefined_array>(in, byte_order, field.count);
    case rational_field_type:
        return read<rational_array>(in, byte_order, field.count);
    case srational_field_type:
        return read<srational_array>(in, byte_order, field.count);
    }
    return {};
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
static_assert(sizeof(field_entry) == 20u, "field_entry size must be 12 bytes");

#pragma pack(pop)

using field_entries = std::vector<field_entry>;

constexpr field_entry byte_swap(const field_entry& value)
{
    return field_entry{
        ::stiffer::byte_swap(value.tag),
        ::stiffer::byte_swap(value.type),
        ::stiffer::byte_swap(value.count),
        ::stiffer::byte_swap(value.value_offset)
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

field_value get_field_value(std::istream& in, const field_entry& field, endian byte_order)
{
    if (!is_value_field(field)) {
        in.seekg(field.value_offset);
        if (!in.good()) {
            throw std::runtime_error(std::string("can't seek to offet ")
                                     + std::to_string(field.value_offset));
        }
    }
    switch (field.type) {
    case byte_field_type:
        return is_value_field(field)?
        get<byte_array>(field.value_offset, field.count):
        read<byte_array>(in, byte_order, field.count);
    case ascii_field_type:
        return is_value_field(field)?
        get<ascii_array>(field.value_offset, field.count):
        read<ascii_array>(in, byte_order, field.count);
    case short_field_type:
        return is_value_field(field)?
        get<short_array>(field.value_offset, field.count):
        read<short_array>(in, byte_order, field.count);
    case long_field_type:
        return is_value_field(field)?
        get<long_array>(field.value_offset, field.count):
        read<long_array>(in, byte_order, field.count);
    case sbyte_field_type:
        return is_value_field(field)?
        get<sbyte_array>(field.value_offset, field.count):
        read<sbyte_array>(in, byte_order, field.count);
    case undefined_field_type:
        return is_value_field(field)?
        get<undefined_array>(field.value_offset, field.count):
        read<undefined_array>(in, byte_order, field.count);
    case long8_field_type:
        return is_value_field(field)?
        get<long8_array>(field.value_offset, field.count):
        read<long8_array>(in, byte_order, field.count);
    case slong8_field_type:
        return is_value_field(field)?
        get<slong8_array>(field.value_offset, field.count):
        read<slong8_array>(in, byte_order, field.count);
    case ifd8_field_type:
        return is_value_field(field)?
        get<ifd8_array>(field.value_offset, field.count):
        read<ifd8_array>(in, byte_order, field.count);
    case rational_field_type:
        return read<rational_array>(in, byte_order, field.count);
    case srational_field_type:
        return read<srational_array>(in, byte_order, field.count);
    }
    return {};
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

constexpr auto image_width_tag = field_tag{256u};
constexpr auto image_length_tag = field_tag{257u};
constexpr auto bits_per_sample_tag = field_tag{258u};
constexpr auto samples_per_pixel_tag = field_tag{277u};
constexpr auto compression_tag = field_tag{259u};
constexpr auto strip_offsets_tag = field_tag{273u};
constexpr auto rows_per_strip_tag = field_tag{278u};
constexpr auto strip_byte_counts_tag = field_tag{279u};

field_value bits_per_sample_value_default(const field_value_map& map)
{
    const auto samples_per_pixel = get_samples_per_pixel(map);
    return short_array(samples_per_pixel, 1u);
}

field_value max_sample_value_default(const field_value_map& map)
{
    /*, Default is 2**(BitsPerSample) - 1 */
    const auto& definitions = get_definitions();
    const auto bits_per_sample = get(map, bits_per_sample_tag, definitions);
    if (const auto entries = std::get_if<short_array>(&bits_per_sample); entries) {
        auto result = short_array{};
        for (auto&& entry: *entries) {
            result.push_back((0x2u << entry) - 1u);
        }
        return {result};
    }
    return {};
}

} // namespace

const field_definition_map& get_definitions()
{
    static const auto map = field_definition_map{
        {315u, {"Artist", (0x1u << ascii_field_type)}},
        {bits_per_sample_tag, {"BitsPerSample", (0x1u << short_field_type), bits_per_sample_value_default}},
        {265u, {"CellLength", (0x1u << short_field_type)}},
        {264u, {"CellWidth", (0x1u << short_field_type)}},
        {320u, {"ColorMap", (0x1u << short_field_type)}},
        {compression_tag, {"Compression", (0x1u << short_field_type), get_short_array_1}},
        {33432u, {"Copyright", (0x1u << ascii_field_type)}},
        {306u, {"DateTime", (0x1u << ascii_field_type)}},
        {338u, {"ExtraSamples", (0x1u << short_field_type)}},
        {266u, {"FillOrder", (0x1u << short_field_type), get_short_array_1}},
        {289u, {"FreeByteCounts", (0x1u << long_field_type)}},
        {288u, {"FreeOffsets", (0x1u << long_field_type)}},
        {291u, {"GrayResponseCurve", (0x1u << short_field_type)}},
        {290u, {"GrayResponseUnit", (0x1u << short_field_type), get_short_array_2}},
        {316u, {"HostComputer", (0x1u << ascii_field_type)}},
        {270u, {"ImageDescription", (0x1u << ascii_field_type)}},
        {image_length_tag, {"ImageLength", (0x1u << short_field_type)|(0x1u << long_field_type)}},
        {image_width_tag, {"ImageWidth", (0x1u << short_field_type)|(0x1u << long_field_type)}},
        {271u, {"Make", (0x1u << ascii_field_type)}},
        {281u, {"MaxSampleValue", (0x1u << short_field_type), max_sample_value_default}},
        {280u, {"MinSampleValue", (0x1u << short_field_type), get_short_array_0}},
        {272u, {"Model", (0x1u << ascii_field_type)}},
        {254u, {"NewSubfileType", (0x1u << long_field_type), get_long_array_0}},
        {274u, {"Orientation", (0x1u << short_field_type), get_short_array_1}},
        {262u, {"PhotometricInterpretation", (0x1u << short_field_type), get_short_array_1}},
        {284u, {"PlanarConfiguration", (0x1u << short_field_type), get_short_array_1}},
        {296u, {"ResolutionUnit", (0x1u << short_field_type), get_short_array_2}},
        {rows_per_strip_tag, {"RowsPerStrip", (0x1u << short_field_type)|(0x1u << long_field_type), get_long_array_max}},
        {samples_per_pixel_tag, {"SamplesPerPixel", (0x1u << short_field_type), get_short_array_1}},
        {305u, {"Software", (0x1u << ascii_field_type)}},
        {strip_byte_counts_tag, {"StripByteCounts", (0x1u << short_field_type)|(0x1u << long_field_type)}},
        {strip_offsets_tag, {"StripOffsets", (0x1u << short_field_type)|(0x1u << long_field_type)}},
        {255u, {"SubfileType", (0x1u << short_field_type)}},
        {263u, {"Threshholding", (0x1u << short_field_type), get_short_array_1}},
        {282u, {"XResolution", (0x1u << rational_field_type)}},
        {283u, {"YResolution", (0x1u << rational_field_type)}},
    };
    return map;
}

std::size_t get_compression(const field_value_map& map)
{
    return get_unsigned_front(map, compression_tag);
}

std::size_t get_image_length(const field_value_map& map)
{
    return get_unsigned_front(map, image_length_tag);
}

std::size_t get_image_width(const field_value_map& map)
{
    return get_unsigned_front(map, image_width_tag);
}

std::size_t get_samples_per_pixel(const field_value_map& map)
{
    return get_unsigned_front(map, samples_per_pixel_tag);
}

std::size_t get_rows_per_strip(const field_value_map& map)
{
    return get_unsigned_front(map, rows_per_strip_tag);
}

std::size_t get_strip_byte_count(const field_value_map& map, std::size_t strip)
{
    const auto found = find(map, strip_byte_counts_tag);
    if (!found) {
        throw std::invalid_argument("strip byte counts entry missing from ifd");
    }
    const auto longs = std::get_if<long_array>(found);
    if (longs) return longs->at(strip);
    const auto shorts = std::get_if<short_array>(found);
    if (shorts) return shorts->at(strip);
    throw std::invalid_argument("strip byte counts entry type not long nor short");
}

std::size_t get_strip_offset(const field_value_map& map, std::size_t strip)
{
    const auto found = find(map, strip_offsets_tag);
    if (!found) {
        throw std::invalid_argument("strip offsets entry missing from ifd");
    }
    const auto longs = std::get_if<long_array>(found);
    if (longs) return longs->at(strip);
    const auto shorts = std::get_if<short_array>(found);
    if (shorts) return shorts->at(strip);
    throw std::invalid_argument("strip offsets entry type not long nor short");
}

field_value get_bits_per_sample(const field_value_map& map)
{
    return get(map, bits_per_sample_tag, get_definitions());
}

undefined_array read_strip(std::istream& is, const image_file_directory& ifd, std::size_t strip)
{
    auto bytes = undefined_array{};
    const auto strip_byte_count = get_strip_byte_count(ifd.fields, strip);
    const auto strip_offset = get_strip_offset(ifd.fields, strip);
    bytes.resize(strip_byte_count);
    is.seekg(strip_offset);
    if (!is.good()) {
        throw std::runtime_error("can't seek to strip offset");
    }
    is.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!is.good()) {
        throw std::runtime_error("can't read strip");
    }
    return bytes;
}

} // namespace stiffer::version_6
