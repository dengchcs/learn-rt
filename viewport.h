//
// Created by CD on 2022/10/26.
//

#ifndef RT_VIEWPORT_H
#define RT_VIEWPORT_H

#include "common.h"

// 平行于XY平面、中心在Z轴负半轴上的视窗
class viewport {
    float width_, height_;
    float focal_length_;
public:
    viewport(float width, float height, float focal_length) {
        width_ = width;
        height_ = height;
        focal_length_ = focal_length;
    }
    [[nodiscard]] point_t lower_left() const {
        return {-width_ / 2.0f, -height_ / 2.0f, -focal_length_};
    }
    [[nodiscard]] point_t point_at(float u, float v) const {
        return lower_left() + vec3_t{width_ * u, height_ * v, 0};
    }
};

#endif //RT_VIEWPORT_H
