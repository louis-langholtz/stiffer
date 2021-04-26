//
//  classic.hpp
//  library
//
//  Created by Louis D. Langholtz on 4/1/21.
//

#ifndef STIFFER_CLASSIC_HPP
#define STIFFER_CLASSIC_HPP

#include "stiffer.hpp"

namespace stiffer::classic {

using directory_count = std::uint16_t;
using field_count = std::uint32_t;
using file_offset = std::uint32_t;
image_file_directory get_image_file_directory(std::istream& is, std::size_t at, endian byte_order);
void put_image_file_directory(std::ostream& stream, std::size_t at, endian byte_order,
                              const image_file_directory& ifd);

#pragma pack(push, 1)

struct field_entry {
    field_tag tag;
    field_type type;
    field_count count; /// Count of the indicated type.
    file_offset value_offset; /// Offset in bytes to the first value.
};
static_assert(sizeof(field_entry) == 12u, "field_entry size must be 12 bytes");

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

} // stiffer::classic

#endif /* STIFFER_CLASSIC_HPP */
