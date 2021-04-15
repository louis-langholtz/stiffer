//
//  image.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/31/21.
//

#ifndef STIFFER_IMAGE_HPP
#define STIFFER_IMAGE_HPP

#include "image_buffer.hpp"
#include "stiffer.hpp" // for stiffer::uintmax_t

namespace stiffer {

struct image {
    image_buffer buffer;
    uintmax_t photometric_interpretation;
    uintmax_t orientation;
    uintmax_t planar_configuration;
};

} // namespace stiffer

#endif // STIFFER_IMAGE_HPP
