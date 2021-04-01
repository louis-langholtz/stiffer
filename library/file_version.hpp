//
//  file_version.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/30/21.
//

#ifndef STIFFER_FILE_VERSION_HPP
#define STIFFER_FILE_VERSION_HPP

namespace stiffer {

enum class file_version {
    classic,
    bigtiff,
};

constexpr const char* to_string(file_version value)
{
    switch (value) {
    case file_version::classic: return "classic";
    case file_version::bigtiff: return "bigtiff";
    }
    return nullptr;
}

} // namespace stiffer

#endif // STIFFER_FILE_VERSION_HPP
