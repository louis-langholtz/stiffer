//
//  bigtiff.hpp
//  library
//
//  Created by Louis D. Langholtz on 4/1/21.
//

#ifndef STIFFER_BIGTIFF_HPP
#define STIFFER_BIGTIFF_HPP

#include "stiffer.hpp"

namespace stiffer::bigtiff {

using directory_count = std::uint64_t;
using field_count = std::uint64_t;
using file_offset = std::uint64_t;
image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order);

#pragma pack(push, 1)

struct field_entry {
    field_tag tag;
    field_type type;
    field_count count; /// Count of the indicated type.
    file_offset value_offset; /// Offset in bytes to the first value.
};
static_assert(sizeof(field_entry) == 20u, "field_entry size must be 20 bytes");

#pragma pack(pop)

constexpr bool is_value_field(const field_entry& field)
{
    const auto count = field.count;
    return (count == 0u)
        || (to_bytesize(field.type) <= sizeof(field_entry::value_offset) / count);
}

constexpr bool is_value_field(const field_value& field)
{
    const auto count = size(field);
    return (count == 0u)
        || (to_bytesize(get_field_type(field)) <= sizeof(field_entry::value_offset) / count);
}

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

} // stiffer::bigtiff

#endif // STIFFER_BIGTIFF_HPP
