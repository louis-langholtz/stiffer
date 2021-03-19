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

namespace stiffer {

namespace {

constexpr auto classic_version_number = std::uint16_t{42};
constexpr auto bigtiff_version_number = std::uint16_t{43};

} // namespace

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
    case double_type: return "double";
    default: break;
    }
    return "unrecognized";
}

std::size_t field_type_to_bytesize(field_type value)
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

void add_defaults(field_value_map& map, const field_definition_map& definitions)
{
    for (auto&& def: definitions) {
        if (def.second.default_fn) {
            if (const auto it = map.find(def.first); it == map.end()) {
                map.insert({def.first, def.second.default_fn(map)});
            }
        }
    }
}

file_version to_file_version(std::uint16_t number)
{
    switch (number) {
    case classic_version_number: return file_version::classic;
    case bigtiff_version_number: return file_version::bigtiff;
    }
    throw std::invalid_argument("unrecognized version number");
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
    std::uint16_t version_number{};
    is.read(reinterpret_cast<char*>(&version_number), sizeof(version_number));
    if (!is.good()) {
        throw std::runtime_error("can't read version number");
    }
    const auto fv = to_file_version(from_endian(version_number, *endian_found));
    switch (fv) {
    case file_version::classic: {
        std::uint32_t offset{};
        is.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        if (!is.good()) {
            throw std::runtime_error("can't read initial offset");
        }
        return {offset, *endian_found, fv};
    }
    case file_version::bigtiff:
        break;
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
        if (const auto fn = it->second.default_fn; fn) {
            return {fn(values)};
        }
    }
    return {};
}

template <typename T>
std::enable_if_t<std::is_trivially_copyable_v<T>, T> read(std::istream& in, endian from_order)
{
    auto element = T{};
    in.read(reinterpret_cast<char*>(&element), sizeof(element));
    if (!in.good()) {
        throw std::runtime_error("can't read element");
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
        auto element = element_type{};
        try {
            element = read<element_type>(in, from_order);
        }
        catch (const std::runtime_error& ex) {
            throw std::runtime_error(std::string(ex.what()) +
                                     std::string(" number ") + std::to_string(i));
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
    constexpr auto num_bits = sizeof(element_type) * 8u;
    constexpr auto mask = (static_cast<std::uint64_t>(1) << num_bits) - 1u;
    constexpr auto avail = sizeof(in) / sizeof(element_type);
    const auto max_count = std::min(count, avail);
    for (auto i = static_cast<decltype(count)>(0); i < max_count; ++i) {
        const auto element = static_cast<element_type>(in & static_cast<decltype(in)>(mask));
        elements.push_back(element);
        if constexpr (sizeof(element_type) < sizeof(in)) {
            in >>= num_bits;
        }
    }
    return elements;
}

namespace classic {

constexpr auto get_file_type_magic(endian value)
{
    // 42 or 0x2a in the specified byte order...
    switch (value) {
    case endian::little: return to_little_endian(classic_version_number);
    case endian::big: break;
    }
    return to_big_endian(classic_version_number);
}

#pragma pack(push, 1)

struct file_header {
    std::uint16_t byte_order;
    std::uint16_t version_number;
    std::uint32_t first_image;
};
static_assert(sizeof(file_header) == 8u, "file_header size must be 8 bytes");

struct field_entry {
    std::uint16_t tag;
    std::uint16_t type;
    std::uint32_t count; /// Count of the indicated type.
    std::uint32_t value_offset; /// Offset in bytes to the first value.
};
static_assert(sizeof(field_entry) == 12u, "field_entry size must be 12 bytes");

#pragma pack(pop)

using field_entries = std::vector<field_entry>;

inline field_entry byte_swap(const field_entry& value)
{
    return field_entry{
        ::stiffer::byte_swap(value.tag),
        ::stiffer::byte_swap(value.type),
        ::stiffer::byte_swap(value.count),
        ::stiffer::byte_swap(value.value_offset)
    };
}

bool is_value_field(const field_entry& field)
{
    return field_type_to_bytesize(field.type) <= sizeof(field_entry::value_offset) && field.count <= 1u;
}

file_header get_file_header(std::istream& is)
{
    auto header = file_header{};
    is.seekg(0);
    if (!is.good()) {
        throw std::runtime_error("can't seek to position 0");
    }
    is.read(static_cast<char*>(static_cast<void*>(&header)), sizeof(header));
    if (!is.good()) {
        throw std::runtime_error("can't read header");
    }
    return header;
}

file_context get_file_context(const file_header& header)
{
    const auto endian_found = find_endian(header.byte_order);
    if (!endian_found) {
        throw std::invalid_argument("unrecognized byte order");
    }
    if (from_endian(header.version_number, *endian_found) != classic_version_number) {
        throw std::invalid_argument("invalid magic");
    }
    return file_context{from_endian(header.first_image, *endian_found), *endian_found};
}

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order)
{
    field_value_map field_map;
    in.seekg(at);
    if (!in.good()) {
        throw std::runtime_error("can't seek to given offet");
    }
    std::uint16_t num_fields;
    in.read(static_cast<char*>(static_cast<void*>(&num_fields)), sizeof(num_fields));
    if (!in.good()) {
        throw std::runtime_error("can't read number of fields of first image");
    }
    num_fields = from_endian(num_fields, byte_order);
    auto fields = read<field_entries>(in, byte_order, num_fields);
    auto next_ifd_offset = std::uint32_t{};
    in.read(reinterpret_cast<char*>(&next_ifd_offset), sizeof(next_ifd_offset));
    if (!in.good()) {
        throw std::runtime_error("can't read next IFD offset");
    }
    next_ifd_offset = from_endian(next_ifd_offset, byte_order);
    std::sort(fields.begin(), fields.end(), [](const field_entry& a, const field_entry& b){
        if (is_value_field(a) && is_value_field(b)) {
            return &a < &b;
        }
        if (is_value_field(a)) {
            return true;
        }
        if (is_value_field(b)) {
            return false;
        }
        return a.value_offset < b.value_offset;
    });
    for (auto&& field: fields) {
        field_value value;
        if (!is_value_field(field)) {
            in.seekg(field.value_offset);
        }
        switch (field.type) {
        case byte_field_type:
            value = is_value_field(field)?
            get<byte_array>(field.value_offset, field.count):
            read<byte_array>(in, byte_order, field.count);
            break;
        case ascii_field_type:
            value = is_value_field(field)?
            get<ascii_array>(field.value_offset, field.count):
            read<ascii_array>(in, byte_order, field.count);
            break;
        case short_field_type:
            value = is_value_field(field)?
            get<short_array>(field.value_offset, field.count):
            read<short_array>(in, byte_order, field.count);
            break;
        case long_field_type:
            value = is_value_field(field)?
            get<long_array>(field.value_offset, field.count):
            read<long_array>(in, byte_order, field.count);
            break;
        case sbyte_field_type:
            value = is_value_field(field)?
            get<sbyte_array>(field.value_offset, field.count):
            read<sbyte_array>(in, byte_order, field.count);
            break;
        case undefined_field_type:
            value = is_value_field(field)?
            get<undefined_array>(field.value_offset, field.count):
            read<undefined_array>(in, byte_order, field.count);
            break;
        case rational_field_type:
            value = read<rational_array>(in, byte_order, field.count);
            break;
        case srational_field_type:
            value = read<srational_array>(in, byte_order, field.count);
            break;
        }
        field_map[field.tag] = value;
    }
    return image_file_directory{field_map, next_ifd_offset};
}

} // namespace classic
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

std::size_t get_unsigned_front(const field_value_map& map, field_tag tag)
{
    const auto result = get(map, tag, get_definitions());
    if (result == field_value{}) {
        throw std::invalid_argument(std::to_string(tag) + " tag entry not retrievable");
    }
    if (const auto values = std::get_if<long_array>(&result); values) return values->at(0);
    if (const auto values = std::get_if<short_array>(&result); values) return values->at(0);
    if (const auto values = std::get_if<byte_array>(&result); values) return values->at(0);
    throw std::invalid_argument(std::to_string(tag) + " tag entry not any unsigned integral type");
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
