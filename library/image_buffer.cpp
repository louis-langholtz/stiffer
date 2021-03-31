//
//  image_buffer.cpp
//  library
//
//  Created by Louis D. Langholtz on 3/24/21.
//

#include "image_buffer.hpp"
#include "stiffer.hpp"

#include <numeric>

namespace stiffer {

image_buffer::image_buffer(std::size_t width, std::size_t height, const std::vector<std::size_t>& bits_per_sample) :
    width_(width), height_(height), bits_per_sample_(bits_per_sample)
{
    buffer_.resize(width * height * get_bytes_per_pixel(bits_per_sample));
}

void image_buffer::resize(std::size_t width, std::size_t height, const std::vector<std::size_t>& bits_per_sample)
{
    width_ = width;
    height_ = height;
    bits_per_sample_ = bits_per_sample;
    buffer_.resize(width * height * get_bytes_per_pixel(bits_per_sample));
}

std::size_t get_bytes_per_pixel(const std::vector<std::size_t>& bits_per_sample)
{
    const auto total_bits = std::accumulate(begin(bits_per_sample), end(bits_per_sample), 0u);
    return (total_bits + 7u) / 8u;
}

} // namespace stiffer
