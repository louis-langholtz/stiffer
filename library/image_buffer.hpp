//
//  image_buffer.hpp
//  library
//
//  Created by Louis D. Langholtz on 3/24/21.
//

#ifndef STIFFER_IMAGE_BUFFER_HPP
#define STIFFER_IMAGE_BUFFER_HPP

#include <cstddef> // for std::size_t
#include <vector>

namespace stiffer {

/// Image buffer.
/// @invariant The size in bytes of the buffer is tied to the width, height, and bits-per-sample
///   this instance is constructed with or resized with.
class image_buffer {
    std::size_t width_{0u};
    std::size_t height_{0u};
    std::vector<std::size_t> bits_per_sample_; // vector size is samples-per-pixel.
    std::vector<unsigned char> buffer_;

public:
    image_buffer() = default;

    image_buffer(std::size_t width, std::size_t height, const std::vector<std::size_t>& bits_per_sample);

    std::size_t get_width() const noexcept {
        return width_;
    }

    std::size_t get_height() const noexcept {
        return height_;
    }

    const std::vector<std::size_t>& get_bits_per_sample() const {
        return bits_per_sample_;
    }

    const unsigned char *data() const noexcept {
        return buffer_.data();
    }

    unsigned char *data() noexcept {
        return buffer_.data();
    }

    std::size_t size() const noexcept {
        return buffer_.size();
    }

    void resize(std::size_t width, std::size_t height, const std::vector<std::size_t>& bits_per_sample);
};

std::size_t get_bytes_per_pixel(const std::vector<std::size_t>& bits_per_sample);

struct image {
    image_buffer buffer;
    std::size_t photometric_interpretation;
    std::size_t orientation;
    std::size_t planar_configuration;
};

} // namespace stiffer

#endif /* STIFFER_IMAGE_BUFFER_HPP */
