//
//  main.cpp
//  writer
//
//  Created by Louis D. Langholtz on 4/16/21.
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../library/v6.hpp"
#include "../library/classic.hpp"

int main(int argc, const char * argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        std::exit(1);
    }
    const std::string filename = argv[1];
    std::fstream stream(filename, std::ios_base::binary|std::ios_base::out);
    if (!stream.is_open()) {
        std::cerr << "Couldn't open file " << filename;
        std::cerr << " within " << std::filesystem::current_path();
        std::cerr << ".\n";
        return 1;
    }
    constexpr auto byte_order = stiffer::endian::little;
    constexpr auto endian_key = get_endian_key(byte_order);
    stiffer::write(stream, endian_key);
    if (!stream.good()) {
        throw std::runtime_error("can't write the endian key for the byte ordering of the file");
    }
    const auto version_key = to_endian(to_file_version_key(stiffer::file_version::classic), byte_order);
    stiffer::write(stream, version_key);
    if (!stream.good()) {
        throw std::runtime_error("can't write the version key to the file");
    }
    const auto pos = sizeof(stiffer::classic::file_offset) + sizeof(endian_key) + sizeof(version_key);
    const auto offset = to_endian(static_cast<stiffer::classic::file_offset>(pos), byte_order);
    stiffer::write(stream, offset);
    if (!stream.good()) {
        throw std::runtime_error("can't write initial file offset");
    }
    stiffer::field_value_map fields;
    stiffer::classic::put(stream, fields, byte_order);
    const auto next_offset = to_endian(static_cast<stiffer::classic::file_offset>(0), byte_order);
    stiffer::write(stream, next_offset);
    if (!stream.good()) {
        throw std::runtime_error("can't write next offset");
    }
    std::cout << "done.\n";
    return 0;
}
