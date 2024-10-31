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
    // unsigned char *data_{nullptr};
    float *dataf_{nullptr};
    int width_{0}, height_{0}, channels_{3};

public:
    explicit image_texture(const std::string &file) {
        int comp_per_pixel = 3;
        auto *data_ = stbi_load(file.c_str(), &width_, &height_, &comp_per_pixel, comp_per_pixel);
        if (data_ == nullptr) {
            std::cerr << "error loading texture\n";
        }
        dataf_ = new float[width_ * height_ * channels_];
        for (int i = 0; i < width_ * height_ * channels_; i++) {
            dataf_[i] = srgb_to_linear(data_[i] / 255.0);
        }
        stbi_image_free(data_);
    }
    ~image_texture() override {
        delete[] dataf_;
    }

    [[nodiscard]] color_t color_at(tex_coords_t tex_coords, const point_t &point) const override {
        const float uwidth = tex_coords[0] * width_;
        const float vheight = (1 - tex_coords[1]) * height_;    // v goes from bottom to top
        const int x = clamp((int)uwidth, 0, width_ - 1);
        const int y = clamp((int)vheight, 0, height_ - 1);
        const int index = (x + y * width_) * channels_;
        const color_t color = {dataf_[index], dataf_[index + 1], dataf_[index + 2]};

        const float u2center = uwidth - (x + 0.5);
        const float v2center = vheight - (y + 0.5);
        const int x1 =  u2center > 0 ? std::min(x + 1, width_ - 1) : std::max(x - 1, 0);
        const int y1 =  v2center > 0 ? std::min(y + 1, height_ - 1) : std::max(y - 1, 0);

        const int index1 = (x1 + y * width_) * channels_;
        const color_t color1 = {dataf_[index1], dataf_[index1 + 1], dataf_[index1 + 2]};
        const auto ycolor = color * abs(u2center) + color1 * abs(1 - abs(u2center));
        const int index2 = (x + y1 * width_) * channels_;
        const color_t color2 = {dataf_[index2], dataf_[index2 + 1], dataf_[index2 + 2]};
        const int index3 = (x1 + y1 * width_) * channels_;
        const color_t color3 = {dataf_[index3], dataf_[index3 + 1], dataf_[index3 + 2]};
        const auto y1color = color2 * abs(u2center) + color3 * abs(1 - abs(u2center));

        auto lerped = ycolor * abs(v2center) + y1color * abs(1 - abs(v2center));
        return lerped;
    }
};

#endif  // RT_TEXTURE_HPP
