//
//  bigtiff.cpp
//  library
//
//  Created by Louis D. Langholtz on 4/1/21.
//

#include "bigtiff.hpp"
#include "details.hpp"

namespace stiffer::bigtiff {

image_file_directory get_image_file_directory(std::istream& in, std::size_t at, endian byte_order)
{
    return details::get_ifd<directory_count, field_entries, file_offset>(in, at, byte_order);
}

} // namespace stiffer::bigtiff
