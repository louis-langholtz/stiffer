//
//  rational.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/19/21.
//

#ifndef STIFFER_RATIONAL_HPP
#define STIFFER_RATIONAL_HPP

#include <cstdint> // for std::uint32_t

namespace stiffer {

struct rational {
    std::uint32_t numerator;
    std::uint32_t denominator;
};
static_assert(sizeof(rational) == 8u, "rational size must be 8 bytes");

constexpr bool operator==(const rational& lhs, const rational& rhs)
{
    return lhs.numerator == rhs.numerator && lhs.denominator == rhs.denominator;
}

constexpr bool operator!=(const rational& lhs, const rational& rhs)
{
    return !(lhs == rhs);
}

} // namespace stiffer

#endif /* STIFFER_RATIONAL_HPP */
