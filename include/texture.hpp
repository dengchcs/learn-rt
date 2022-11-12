#ifndef RT_TEXTURE_HPP
#define RT_TEXTURE_HPP

#include <iostream>

#include "stb_image.h"
#include "common.hpp"
#include "utils.hpp"


class texture {
public:
    [[nodiscard]] virtual color_t color_at(float u, float v, const point_t &point) const = 0;
};

class solid_color : public texture {
    color_t value_;

public:
    explicit solid_color(const color_t &color) : value_(color) {}

    solid_color(float r, float g, float b) : value_(r, g, b) {}

public:
    [[nodiscard]] color_t color_at(float u, float v, const point_t &point) const override {
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
    ~image_texture() { stbi_image_free(data_); }

public:
    [[nodiscard]] color_t color_at(float u, float v, const point_t &point) const override {
        u = clamp(u, 0.0f, 1.0f);
        v = 1.0f - clamp(v, 0.0f, 1.0f);
        const int i = std::min(width_ - 1, (int)(u * (float)width_));
        const int j = std::min(height_ - 1, (int)(v * (float)height_));
        const auto pixel = data_ + j * 3 * width_ + i * 3;
        const float scale = 1.0f / 255.0f;
        return scale * color_t(pixel[0], pixel[1], pixel[2]);
    }
};

#endif  // RT_TEXTURE_HPP
