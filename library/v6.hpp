//
//  v6.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/30/21.
//

#ifndef STIFFER_V6_HPP
#define STIFFER_V6_HPP

#include "stiffer.hpp"
#include "image.hpp"

namespace stiffer::v6 {

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
constexpr auto transfer_function_tag = field_tag{301u};
constexpr auto software_tag = field_tag{305u};
constexpr auto date_time_tag = field_tag{306u};
constexpr auto artist_tag = field_tag{315u};
constexpr auto host_computer_tag = field_tag{316u};
constexpr auto predictor_tag = field_tag{317u};
constexpr auto white_point_tag = field_tag{318u};
constexpr auto primary_chromatics_tag = field_tag{319};
constexpr auto color_map_tag = field_tag{320u};
constexpr auto halftone_hints_tag = field_tag{321};
constexpr auto tile_width_tag = field_tag{322u};
constexpr auto tile_length_tag = field_tag{323u};
constexpr auto tile_offsets_tag = field_tag{324u};
constexpr auto tile_byte_counts_tag = field_tag{325u};
constexpr auto sub_ifds_tag = field_tag{330u};
constexpr auto ink_set_tag = field_tag{332u};
constexpr auto ink_names_tag = field_tag{333u};
constexpr auto number_of_inks_tag = field_tag{334u};
constexpr auto dot_range_tag = field_tag{336u};
constexpr auto target_printer_tag = field_tag{337u};
constexpr auto extra_samples_tag = field_tag{338u};
constexpr auto sample_format_tag = field_tag{339u};
constexpr auto s_min_sample_value_tag = field_tag{340u};
constexpr auto s_max_sample_value_tag = field_tag{3401};
constexpr auto transfer_range_tag = field_tag{342u};
constexpr auto copyright_tag = field_tag{33432u};

const field_definition_map& get_definitions();

inline std::size_t get_unsigned_front(const field_value_map& fields, field_tag tag)
{
    return get_unsigned_front(get(fields, tag, get_definitions()));
}

/// Gets the compression type used for image data.
/// @note 1 means: "No compression, but pack data into bytes as tightly as possible, leaving
///   no unused bits (except at the end of a row). The component values are stored as
///   an array of type BYTE. Each scan line (row) is padded to the next BYTE boundary."
/// @note 2 means: "CCITT Group 3 1-Dimensional Modified Huffman run length encoding."
/// @note 32773 means: "PackBits compression, a simple byte-oriented run length scheme."
inline std::size_t get_compression(const field_value_map& fields)
{
    return get_unsigned_front(fields, compression_tag);
}

inline std::size_t get_image_length(const field_value_map& fields)
{
    return get_unsigned_front(fields, image_length_tag);
}

inline std::size_t get_image_width(const field_value_map& fields)
{
    return get_unsigned_front(fields, image_width_tag);
}

inline std::size_t get_samples_per_pixel(const field_value_map& fields)
{
    return get_unsigned_front(fields, samples_per_pixel_tag);
}

inline std::size_t get_rows_per_strip(const field_value_map& fields)
{
    return get_unsigned_front(fields, rows_per_strip_tag);
}

inline std::size_t get_orientation(const field_value_map& fields)
{
    return get_unsigned_front(fields, orientation_tag);
}

inline std::size_t get_photometric_interpretation(const field_value_map& fields)
{
    return get_unsigned_front(fields, photometric_interpretation_tag);
}

inline std::size_t get_planar_configuraion(const field_value_map& fields)
{
    return get_unsigned_front(fields, planar_configuration_tag);
}

inline std::size_t get_resolution_unit(const field_value_map& fields)
{
    return get_unsigned_front(fields, resolution_unit_tag);
}

inline std::size_t get_x_resolution(const field_value_map& fields)
{
    return get_unsigned_front(fields, x_resolution_tag);
}

inline std::size_t get_y_resolution(const field_value_map& fields)
{
    return get_unsigned_front(fields, y_resolution_tag);
}

inline std::size_t get_tile_length(const field_value_map& fields)
{
    return get_unsigned_front(fields, tile_length_tag);
}

inline std::size_t get_tile_width(const field_value_map& fields)
{
    return get_unsigned_front(fields, tile_width_tag);
}

inline std::size_t get_strips_per_image(const field_value_map& fields)
{
    const auto rows_per_strip = get_rows_per_strip(fields);
    return (get_image_length(fields) + rows_per_strip - 1u) / rows_per_strip;
}

field_value get_bits_per_sample(const field_value_map& fields);

bool has_striped_image(const field_value_map& fields);
std::size_t get_strip_byte_count(const field_value_map& fields, std::size_t index);
std::size_t get_strip_offset(const field_value_map& fields, std::size_t index);
undefined_array read_strip(std::istream& is, const field_value_map& fields, std::size_t index);

bool has_tiled_image(const field_value_map& fields);
std::size_t get_tile_byte_count(const field_value_map& fields, std::size_t index);
std::size_t get_tile_offset(const field_value_map& fields, std::size_t index);
undefined_array read_tile(std::istream& is, const field_value_map& fields, std::size_t index);

image read_image(std::istream& in, const field_value_map& fields);

} // namespace stiffer::v6

#endif // STIFFER_V6_HPP
