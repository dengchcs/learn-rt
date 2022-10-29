//
// Created by CD on 2022/10/28.
//

#ifndef RT_CAMERA_H
#define RT_CAMERA_H

#include "common.h"
#include "ray.h"
#include "utils.h"

class camera {
    point_t eye_;
    point_t lower_left_;
    vec3_t horizontal_;
    vec3_t vertical_;
    float len_radius_;
public:
    camera(
            point_t eye,        // 相机位置
            point_t center,     // 视点
            vec3_t up,          // 相机上方向
            float vfov,         // 竖直方向视角
            float aspect_ratio, // 水平/竖直视域比例
            float aperture,     // 孔径
            float focus_dist    // 焦距
    ) {
        const float vp_height = 2.0f * std::tan(vfov / 2.0f);
        const float vp_width = vp_height * aspect_ratio;
        const auto w = unit_vec3{eye - center};
        const auto u = unit_vec3{QVector3D::crossProduct(up, w)};
        const auto v = unit_vec3{QVector3D::crossProduct(w, u)};
        eye_ = eye;
        horizontal_ = focus_dist * vp_width * u;
        vertical_ = focus_dist * vp_height * v;
        lower_left_ = eye_ - horizontal_ / 2.0 - vertical_ / 2.0 - focus_dist * w;
        len_radius_ = aperture / 2.0f;
    }

    [[nodiscard]] ray get_ray(float u, float v) const {
        const vec3_t rd = len_radius_ * random_in_unit_disk();
        const vec3_t offset = rd.x() * horizontal_.normalized() + rd.y() * vertical_.normalized();
        return {
                eye_ + offset,
                lower_left_ + horizontal_ * u + vertical_ * v - eye_ - offset
        };
    }

    [[nodiscard]] ray get_ray_no_blur(float u, float v) const {
        return {eye_, lower_left_ + horizontal_ * u + vertical_ * v - eye_};
    }
};

#endif //RT_CAMERA_H
