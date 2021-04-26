//
//  classic.cpp
//  library
//
//  Created by Louis D. Langholtz on 4/1/21.
//

#include "classic.hpp"
#include "details.hpp"

namespace stiffer::classic {

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order)
{
    return details::get_ifd<directory_count, field_entries, file_offset>(in, at, byte_order);
}

void put_image_file_directory(std::ostream& stream, std::size_t at, endian to_order,
                              const image_file_directory& ifd)
{
    auto total_bytes = std::size_t(0);
    const auto num_fields = std::size(ifd.fields);
    if (num_fields > std::numeric_limits<directory_count>::max()) {
        throw std::invalid_argument("number of fields exceeds classic maximum");
    }
    total_bytes += sizeof(directory_count);
    for (auto&& field: ifd.fields) {
        const auto count = size(field.second);
        if (count > std::numeric_limits<field_count>::max()) {
            throw std::invalid_argument("number of elements exceeds classic maximum");
        }
        total_bytes += sizeof(field_entry);
        if (!is_value_field(field.second)) {
            total_bytes += count * to_bytesize(get_field_type(field.second));
        }
    }
    stream.seekp(at);
    if (!stream.good()) {
        throw std::runtime_error("can't seek to given offet");
    }
    details::write(stream, to_endian(static_cast<directory_count>(num_fields), to_order));
    if (!stream.good()) {
        throw std::runtime_error("can't write number of fields");
    }
    at += sizeof(directory_count) + sizeof(field_entry) * num_fields;
    for (auto&& field: ifd.fields) {
        const auto count = static_cast<field_count>(size(field.second));
        const auto type = get_field_type(field.second);
        auto value_offset = file_offset{};
        if (is_value_field(field.second)) {
        } else {
            at += count * to_bytesize(type);
            value_offset = static_cast<file_offset>(at);
        }
        const auto entry = field_entry{
            field.first, // tag
            type,
            count,
            value_offset
        };
        details::write(stream, to_endian(entry, to_order));
        if (!stream.good()) {
            throw std::runtime_error("can't write field entry");
        }
    }
    for (auto&& field: ifd.fields) {
        if (!is_value_field(field.second)) {
            switch (get_field_type(field.second)) {
            case byte_field_type:
                details::write_field_data<byte_array>(stream, field.second, to_order);
                break;
            case ascii_field_type:
                details::write_field_data<ascii_array>(stream, field.second, to_order);
                break;
            case short_field_type:
                details::write_field_data<short_array>(stream, field.second, to_order);
                break;
            case long_field_type:
                details::write_field_data<long_array>(stream, field.second, to_order);
                break;
            }
        }
    }
    if (at > std::numeric_limits<file_offset>::max() - sizeof(file_offset)) {
        throw std::runtime_error("offset of next image location exceeeds classic capacity");
    }
    details::write(stream, to_endian(static_cast<file_offset>(at), to_order));
}

} // namespace stiffer::classic
