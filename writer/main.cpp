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
    stream.write(reinterpret_cast<const char*>(&endian_key), sizeof(endian_key));
    if (!stream.good()) {
        throw std::runtime_error("can't write the endian key for the byte ordering of the file");
    }
    const auto version_key = to_endian(to_file_version_key(stiffer::file_version::classic), byte_order);
    stream.write(reinterpret_cast<const char*>(&version_key), sizeof(version_key));
    if (!stream.good()) {
        throw std::runtime_error("can't write the version key to the file");
    }
    const auto file_offset = to_endian(static_cast<stiffer::classic::file_offset>(sizeof(stiffer::classic::file_offset) + 4u), byte_order);
    stream.write(reinterpret_cast<const char*>(&file_offset), sizeof(file_offset));
    if (!stream.good()) {
        throw std::runtime_error("can't write initial file offset");
    }
    const auto directory_count = to_endian(stiffer::classic::directory_count{0}, byte_order);
    stream.write(reinterpret_cast<const char*>(&directory_count), sizeof(directory_count));
    if (!stream.good()) {
        throw std::runtime_error("can't write directory count");
    }
    const auto next_offset = to_endian(static_cast<stiffer::classic::file_offset>(0), byte_order);
    stream.write(reinterpret_cast<const char*>(&next_offset), sizeof(next_offset));
    if (!stream.good()) {
        throw std::runtime_error("can't write next offset");
    }
    std::cout << "done.\n";
    return 0;
}
