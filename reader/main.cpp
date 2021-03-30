//
//  main.cpp
//  reader
//
//  Created by Louis D. Langholtz on 3/12/21.
//

#include <iostream>
#include <filesystem>
#include <fstream>

#include "../library/stiffer.hpp"

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

std::ostream& operator<< (std::ostream& os, const stiffer::rational& value)
{
    os << value.numerator << "/" << value.denominator;
    return os;
}

std::ostream& operator<< (std::ostream& os, const stiffer::srational& value)
{
    os << value.numerator << "/" << value.denominator;
    return os;
}

std::ostream& operator<< (std::ostream& os, stiffer::file_version value)
{
    switch (value) {
    case stiffer::file_version::classic:
        os << "classic";
        return os;
    case stiffer::file_version::bigtiff:
        os << "bigtiff";
        return os;
    }
    return os;
}

template <typename T>
std::enable_if_t<std::is_enum_v<T>, std::ostream&> operator<< (std::ostream& os, const T& value)
{
    os << stiffer::to_underlying(value);
    return os;
}

template <typename T>
std::ostream& operator<< (std::ostream& os, const std::vector<T>& values)
{
    auto first = true;
    for (auto&& v: values) {
        if (first) {
            first = false;
        }
        else {
            os << ",";
        }
        os << v;
    }
    return os;
}

int main(int argc, const char * argv[]) {
    const char* filename = nullptr;
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        std::exit(1);
    }
    filename = argv[1];
    std::fstream in(filename, std::ios_base::binary|std::ios_base::in);
    if (!in.is_open()) {
        std::cerr << "Couldn't open file " << filename;
        std::cerr << " within " << std::filesystem::current_path();
        std::cerr << ".\n";
        return 1;
    }
    const auto file_context = stiffer::get_file_context(in);
    std::cout << "File is version " << file_context.version << "\n";
    std::cout << " file stored in " << file_context.byte_order << " endian order\n";
    std::cout << "native order is " << stiffer::endian::native << " endian order\n";
    std::cout << "first offset is " << file_context.first_ifd_offset << "\n";
    for (auto offset = file_context.first_ifd_offset; offset != 0u;) {
        const auto ifd = (file_context.version == stiffer::file_version::classic)?
            stiffer::classic::get_image_file_directory(in, offset, file_context.byte_order):
            stiffer::bigtiff::get_image_file_directory(in, offset, file_context.byte_order);
        std::cout << "file has " << std::size(ifd.fields) << " fields\n";
        for (const auto& field: ifd.fields) {
            std::cout << "tag=" << stiffer::to_underlying(field.first);
            const auto found = find(stiffer::v6::get_definitions(), field.first);
            if (found) {
                std::cout << "(" << found->name << ")";
            }
            const auto field_type = get_field_type(field.second);
            std::cout << ", type=" << to_underlying(field_type);
            std::cout << "(" << stiffer::to_string(field_type) << ")";
            std::cout << ", count=" << size(field.second);
            std::cout << ", value=";
            std::visit(overloaded{
                [](const stiffer::unrecognized_field_value& arg) {},
                [](const stiffer::ascii_array& arg) { std::cout << std::quoted(arg); },
                [](const auto& arg) { std::cout << arg; }
            }, field.second);
            std::cout << "\n";
        }
        std::cout << "next IFD = " << ifd.next_image << "\n";
        try {
            const auto image = stiffer::v6::read_image(in, ifd.fields);
            std::cout << "image width = " << image.buffer.get_width() << "\n";
            std::cout << "image length = " << image.buffer.get_height() << "\n";
            std::cout << "image orientation = " << image.orientation << "\n";
            std::cout << "image photometric interpretation = " << image.photometric_interpretation << "\n";
            std::cout << "image planar configuration = " << image.planar_configuration << "\n";
        }
        catch (const std::exception& ex) {
            std::cout << "image read problem: " << ex.what() << "\n";
        }
        offset = ifd.next_image;
    }
    return 0;
}
