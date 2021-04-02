//
//  srational.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/19/21.
//

#ifndef STIFFER_SRATIONAL_HPP
#define STIFFER_SRATIONAL_HPP

#include <cstdint> // for std::uint32_t

#include "byte_swap.hpp"

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

constexpr srational byte_swap(const srational& value)
{
    return srational{byte_swap(value.numerator), byte_swap(value.denominator)};
}

} // namespace stiffer

#endif /* STIFFER_SRATIONAL_HPP */
