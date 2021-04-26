//
//  main.cpp
//  reader
//
//  Created by Louis D. Langholtz on 3/12/21.
//

#include <cstring>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../library/v6.hpp"

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
    if (const auto name = to_string(value); name && *name) {
        os << name;
    } else {
        os << stiffer::to_underlying(value);
    }
    return os;
}

std::ostream& operator<< (std::ostream& os, const stiffer::undefined_element& value)
{
    using type = decltype(stiffer::to_underlying(value));
    const auto old_flags = os.flags();
    os.setf(std::ios_base::hex|std::ios_base::uppercase, ~std::ios_base::fmtflags(0));
    const auto old_fill = os.fill('0');
    // std::showbase won't output 0x for 0, so 0x is output explicitly for consistency...
    os << "0x" << std::setw(sizeof(type) * 2u) << std::uint64_t(stiffer::to_underlying(value));
    os.fill(old_fill);
    os.flags(old_flags);
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

[[noreturn]] void usage(const std::filesystem::path& program_name)
{
    std::cerr << "Usage: " << program_name.filename().string() << " [-v|--] <filename...>\n";
    std::exit(1);
}

int main(int argc, const char * argv[]) {
    auto verbose = false;
    std::vector<std::string> filenames;
    {
        auto parsing_flags = true;
        for (auto i = 1; i < argc; ++i) {
            if (!parsing_flags) {
                filenames.push_back(argv[i]);
            }
            else if (*argv[i] == '-') {
                if (std::strcmp(argv[i], "-h") == 0) {
                    usage(argv[0]);
                }
                else if (std::strcmp(argv[i], "-v") == 0) {
                    verbose = true;
                }
                else if (std::strcmp(argv[i], "--") == 0) {
                    parsing_flags = false;
                }
                else {
                    std::cerr << "Unrecognized argument: " << argv[i] << "\n";
                    usage(argv[0]);
                }
            }
            else {
                filenames.push_back(argv[i]);
            }
        }
    }
    if (empty(filenames)) {
        usage(argv[0]);
    }
    for (const auto& filename: filenames) {
        std::fstream fstream(filename, std::ios_base::binary|std::ios_base::in);
        if (!fstream.is_open()) {
            std::cerr << "Couldn't open file " << filename;
            std::cerr << " within " << std::filesystem::current_path();
            std::cerr << ".\n";
            return 1;
        }
        const auto file_context = stiffer::get_file_context(fstream);
        std::cout << "File is version " << file_context.version << "\n";
        std::cout << " file stored in " << file_context.byte_order << " endian order\n";
        std::cout << "native order is " << stiffer::endian::native << " endian order\n";
        std::cout << "first offset is " << file_context.first_ifd_offset << "\n";
        for (auto offset = file_context.first_ifd_offset; offset != 0u;) {
            const auto ifd = stiffer::get_image_file_directory(fstream, offset, file_context.byte_order,
                                                               file_context.version);
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
            if (verbose) {
                if (stiffer::v6::has_striped_image(ifd.fields)) {
                    const auto max = stiffer::v6::get_strips_per_image(ifd.fields);
                    for (auto i = static_cast<std::size_t>(0); i < max; ++i) {
                        std::cout << "Strip " << i << ": ";
                        const auto strip = stiffer::v6::read_strip(fstream, ifd.fields, i);
                        std::cout << strip;
                        std::cout << "\n";
                    }
                }
            }
            try {
                const auto image = stiffer::v6::read_image(fstream, ifd.fields);
                std::cout << "image width = " << image.buffer.get_width() << "\n";
                std::cout << "image length = " << image.buffer.get_height() << "\n";
                std::cout << "image orientation = " << image.orientation << "\n";
                std::cout << "image photometric interpretation = " << image.photometric_interpretation << "\n";
                std::cout << "image planar configuration = " << image.planar_configuration << "\n";
            }
            catch (const std::exception& ex) {
                std::cout << "image read problem: " << ex.what() << "\n";
            }
            std::cout << "next IFD = " << ifd.next_image << "\n";
            offset = ifd.next_image;
        }
    }
    return 0;
}
