//
//  v6.cpp
//  library
//
//  Created by Louis D. Langholtz on 3/30/21.
//

#include <sstream> // for std::ostringstream
#include <stdexcept> // for std::invalid_argument etc.

#include "v6.hpp"

namespace stiffer::v6 {

namespace {

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

uintmax_t get_strip_byte_count(const field_value_map& fields, std::size_t index)
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

uintmax_t get_strip_offset(const field_value_map& fields, std::size_t index)
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

uintmax_t get_tile_byte_count(const field_value_map& fields, std::size_t index)
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

uintmax_t get_tile_offset(const field_value_map& fields, std::size_t index)
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

std::ptrdiff_t unpack_bits(const undefined_element* src, std::size_t src_siz,
                           std::uint8_t* dst, std::size_t dst_siz)
{
    const auto src_beg = src;
    const auto src_end = src + src_siz;
    const auto dst_beg = dst;
    while (src < src_end) {
        const auto n_index = src - src_beg;
        const auto n = int(std::int8_t(*src));
        ++src;
        --src_siz;
        if (n >= 0) {
            const auto nbytes = std::size_t(n) + 1u;
            if (nbytes > src_siz) {
                std::ostringstream os;
                os << "source byte " << n_index << ", says to copy the next ";
                os << nbytes << " bytes literally except only ";
                os << src_siz << " source bytes remain";
                throw std::invalid_argument(os.str());
            }
            if (nbytes > dst_siz) {
                std::ostringstream os;
                os << "source byte " << n_index << ", says to copy the next ";
                os << nbytes << " bytes literally except only ";
                os << dst_siz << " bytes space left in destination buffer";
                throw std::invalid_argument(os.str());
            }
            std::memcpy(dst, src, nbytes);
            dst += nbytes;
            dst_siz -= nbytes;
            src += nbytes;
            src_siz -= nbytes;
        } else if (n != -128) {
            const auto nbytes = std::size_t(-n) + 1u;
            if (nbytes > dst_siz) {
                std::ostringstream os;
                os << "source byte " << n_index << ", says to copy the next byte ";
                os << nbytes << " times except only ";
                os << dst_siz << " bytes space left in destination buffer";
                throw std::invalid_argument(os.str());
            }
            const auto next_byte = to_underlying(*src);
            ++src;
            --src_siz;
            for (auto i = std::size_t(0); i < nbytes; ++i) {
                *(dst + i) = next_byte;
            }
            dst += nbytes;
            dst_siz -= nbytes;
        }
    }
    return dst - dst_beg;
}

image read_image(std::istream& in, const field_value_map& fields)
{
    if (has_striped_image(fields)) {
        auto result = image{};
        const auto width = get_image_width(fields);
        const auto length = get_image_length(fields);
        result.buffer.resize(width, length, to_vector<std::size_t>(get_bits_per_sample(fields)));
        result.photometric_interpretation = to_underlying(get_photometric_interpretation(fields));
        result.orientation = to_underlying(get_orientation(fields));
        result.planar_configuration = get_planar_configuraion(fields);
        const auto compression = get_compression(fields);
        const auto max = get_strips_per_image(fields);
        auto offset = std::size_t(0);
        for (auto i = static_cast<decltype(get_strips_per_image(fields))>(0); i < max; ++i) {
            const auto strip = read_strip(in, fields, i);
            switch (compression) {
            case no_compression:
                std::memcpy(result.buffer.data() + offset, data(strip), size(strip));
                offset += strip.size();
                break;
            case packbits_compression:
                offset += unpack_bits(data(strip), size(strip), result.buffer.data() + offset, result.buffer.size() - offset);
                break;
            case ccitt_huffman_compression:
            default:
                throw std::invalid_argument(std::string("unable to decode compression " + std::to_string(to_underlying(compression))));
            }
        }
        return result;
    }
    return image{};
}

} // namespace stiffer::v6
