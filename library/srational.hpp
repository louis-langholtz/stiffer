//
//  srational.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/19/21.
//

#ifndef STIFFER_SRATIONAL_HPP
#define STIFFER_SRATIONAL_HPP

#include <cstdint> // for std::uint32_t

namespace stiffer {

struct srational {
    std::int32_t numerator;
    std::int32_t denominator;
};
static_assert(sizeof(srational) == 8u, "srational size must be 8 bytes");

constexpr bool operator==(const srational& lhs, const srational& rhs)
{
    return lhs.numerator == rhs.numerator && lhs.denominator == rhs.denominator;
}

constexpr bool operator!=(const srational& lhs, const srational& rhs)
{
    return !(lhs == rhs);
}

} // namespace stiffer

#endif /* STIFFER_SRATIONAL_HPP */
