#include "rectangle.hpp"

#include <array>

#include "aabb.hpp"
#include "ray.hpp"

hit_res_t rectangle::hit(const ray &r, float tmin, float tmax) const {
    const float divisor = normal_.dot(r.direction());
    if (divisor == 0) {
        return std::nullopt;
    }
    const float t = normal_.dot(corner_ - r.origin()) / divisor;
    if (t < tmin || t > tmax) {
        return std::nullopt;
    }
    const point_t point = r.point_at(t);
    const float tex_u = (point - corner_).dot(edge_u_) / edge_u_.len_sq();
    if (tex_u < 0 || tex_u > 1) {
        return std::nullopt;
    }
    const float tex_v = (point - corner_).dot(edge_v_) / edge_v_.len_sq();
    if (tex_v < 0 || tex_v > 1) {
        return std::nullopt;
    }
    hit_record record;
    record.ray_param = t;
    record.point = point;
    record.outside = divisor < 0;
    record.normal = divisor < 0 ? normal_ : -normal_;
    record.tex_coords = {tex_u, tex_v};
    record.pmat = pmat_;
    return record;
}

aabb rectangle::bounding_box() const {
    std::array<float, 3> coord_min = {g_max, g_max, g_max};
    std::array<float, 3> coord_max = {-g_max, -g_max, -g_max};
    std::array<point_t, 4> vertices = {corner_, corner_ + edge_u_, corner_ + edge_v_,
                                       corner_ + edge_u_ + edge_v_};
    for (auto &&vertex : vertices) {
        for (int k = 0; k < 3; k++) {  // x,y,z维度
            coord_min.at(k) = std::min(coord_min.at(k), vertex[k]);
            coord_max.at(k) = std::max(coord_max.at(k), vertex[k]);
        }
    }
    for (int k = 0; k < 3; k++) {
        coord_min.at(k) -= aabb::dim_padding;
        coord_max.at(k) += aabb::dim_padding;
    }
    return aabb{vec3_t{coord_min.data()}, vec3_t{coord_max.data()}};
}
