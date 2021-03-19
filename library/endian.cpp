#include "endian.hpp"

namespace stiffer {

endian get_native_endian_at_runtime() noexcept
{
    constexpr auto byte_value = std::uint8_t{1};
    constexpr auto word = std::uint16_t{byte_value};
    return (*static_cast<const std::uint8_t*>(static_cast<const void*>(&word)) == byte_value)?
        endian::little: endian::big;
}

std::ostream& operator<<(std::ostream& os, endian value)
{
    switch (value) {
    case endian::little: os << "little"; return os;
    case endian::big: os << "big"; return os;
    }
    os << "mixed";
    return os;
}

} // namespace stiffer
