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

std::ostream& operator<< (std::ostream& os, const stiffer::undefined_element& value)
{
    os << static_cast<std::underlying_type_t<stiffer::undefined_element>>(value);
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
            std::cout << "tag=" << field.first;
            const auto found = find(stiffer::v6::get_definitions(), field.first);
            if (found) {
                std::cout << "(" << found->name << ")";
            }
            const auto field_type = to_field_type(field.second);
            std::cout << ", type=" << field_type;
            std::cout << "(" << stiffer::field_type_to_string(field_type) << ")";
            std::cout << ", count=" << size(field.second);
            std::cout << ", value=";
            std::visit(overloaded{
                [](const std::monostate& arg) {},
                [](const stiffer::byte_array& arg) { std::cout << arg; },
                [](const stiffer::ascii_array& arg) { std::cout << std::quoted(arg); },
                [](const stiffer::short_array& arg) { std::cout << arg; },
                [](const stiffer::long_array& arg) { std::cout << arg; },
                [](const stiffer::rational_array& arg) { std::cout << arg; },
                [](const stiffer::sbyte_array& arg) { std::cout << arg; },
                [](const stiffer::undefined_array& arg) { std::cout << arg; },
                [](const stiffer::sshort_array& arg) { std::cout << arg; },
                [](const stiffer::slong_array& arg) { std::cout << arg; },
                [](const stiffer::srational_array& arg) {},
                [](const stiffer::float_array& arg) { std::cout << arg; },
                [](const stiffer::double_array& arg) { std::cout << arg; },
                [](const stiffer::long8_array& arg) { std::cout << arg; },
                [](const stiffer::slong8_array& arg) { std::cout << arg; },
                [](const stiffer::ifd8_array& arg) {}
            }, field.second);
            std::cout << "\n";
        }
        std::cout << "next IFD=" << ifd.next_image << "\n";
        offset = ifd.next_image;
    }
    return 0;
}
