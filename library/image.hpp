//
//  image.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/31/21.
//

#ifndef STIFFER_IMAGE_HPP
#define STIFFER_IMAGE_HPP

#include "image_buffer.hpp"

namespace stiffer {

struct image {
    image_buffer buffer;
    std::size_t photometric_interpretation;
    std::size_t orientation;
    std::size_t planar_configuration;
};

} // namespace stiffer

#endif // STIFFER_IMAGE_HPP
