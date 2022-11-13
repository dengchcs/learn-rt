#ifndef RT_TEXTURE_HPP
#define RT_TEXTURE_HPP

#include <iostream>

#include "common.hpp"
#include "stb_image.h"
#include "utils.hpp"

class texture {
public:
    virtual ~texture() = default;
    [[nodiscard]] virtual color_t color_at(tex_coords_t tex_coords, const point_t &point) const = 0;
};

class solid_color : public texture {
    color_t value_;

public:
    explicit solid_color(const color_t &color) : value_(color) {}

    solid_color(float r, float g, float b) : value_(r, g, b) {}

    [[nodiscard]] color_t color_at(tex_coords_t tex_coords, const point_t &point) const override {
        return value_;
    }
};

class image_texture : public texture {
    unsigned char *data_{nullptr};
    int width_{0}, height_{0};

public:
    explicit image_texture(const std::string &file) {
        int comp_per_pixel = 3;
        data_ = stbi_load(file.c_str(), &width_, &height_, &comp_per_pixel, comp_per_pixel);
        if (data_ == nullptr) {
            std::cerr << "error loading texture\n";
        }
    }
    ~image_texture() override { stbi_image_free(data_); }

    [[nodiscard]] color_t color_at(tex_coords_t tex_coords, const point_t &point) const override {
        auto u = clamp(tex_coords.at(0), 0.0F, 1.0F);
        auto v = 1.0F - clamp(tex_coords.at(1), 0.0F, 1.0F);
        const int i = std::min(width_ - 1, (int)(u * (float)width_));
        const int j = std::min(height_ - 1, (int)(v * (float)height_));
        auto *const pixel = data_ + j * 3 * width_ + i * 3;
        constexpr float scale = 1.0F / 255.0F;
        return scale * color_t(pixel[0], pixel[1], pixel[2]);
    }
};

#endif  // RT_TEXTURE_HPP
