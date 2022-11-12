#include "rectangle.hpp"

#include "aabb.hpp"
#include "ray.hpp"

std::optional<hit_record> rectangle::hit(const ray &r, float tmin, float tmax) const {
    const float divisor = normal_.dot(r.direction());
    if (divisor == 0) {
        return std::nullopt;
    }
    const float t = normal_.dot(corner_ - r.origin()) / divisor;
    if (t < tmin || t > tmax) {
        return std::nullopt;
    }
    const point_t point = r.point_at(t);
    const float u = (point - corner_).dot(edge_u_) / edge_u_.len_sq();
    if (u < 0 || u > 1) {
        return std::nullopt;
    }
    const float v = (point - corner_).dot(edge_v_) / edge_v_.len_sq();
    if (v < 0 || v > 1) {
        return std::nullopt;
    }
    hit_record record;
    record.ray_param = t;
    record.point = point;
    record.outside = divisor < 0;
    record.normal = divisor < 0 ? normal_ : -normal_;
    record.u = u, record.v = v;
    record.pmat = pmat_;
    return record;
}

aabb rectangle::bounding_box() const {
    const auto inf = std::numeric_limits<float>::max();
    float coord_min[3] = {inf, inf, inf};
    float coord_max[3] = {-inf, -inf, -inf};
    point_t vertices[4] = {corner_, corner_ + edge_u_, corner_ + edge_v_,
                           corner_ + edge_u_ + edge_v_};
    for (auto &&vertex : vertices) {
        for (int k = 0; k < 3; k++) {  // x,y,z维度
            coord_min[k] = std::min(coord_min[k], vertex[k]);
            coord_max[k] = std::max(coord_max[k], vertex[k]);
        }
    }
    for (int k = 0; k < 3; k++) {
        coord_min[k] -= 0.0001;
        coord_max[k] += 0.0001;
    }
    return aabb{vec3_t{coord_min}, vec3_t{coord_max}};
}
