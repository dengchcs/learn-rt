#include "sphere.hpp"

tex_coords_t sphere::uv_at(const point_t &point) const {
    const auto p_unit = (point - center_) / radius_;
    const float theta = std::acos(-p_unit.y());
    const float phi = std::atan2(-p_unit.z(), p_unit.x()) + g_pi;

    return {phi / (2.0F * g_pi), theta / g_pi};
}

hit_res_t sphere::hit(const ray &r, float tmin, float tmax) const {
    const vec3_t oc = r.origin() - center_;
    const float b_half = oc.dot(r.direction());
    const float c = oc.len_sq() - radius_ * radius_;
    const float discriminant = b_half * b_half - c;
    if (discriminant < 0) {
        return std::nullopt;
    }

    const float sqrtd = std::sqrt(discriminant);
    float root = -b_half - sqrtd;
    if (root < tmin || root > tmax) {
        root = -b_half + sqrtd;
        if (root < tmin || root > tmax) {
            return std::nullopt;
        }
    }

    hit_record record;
    record.ray_param = root;
    record.point = r.point_at(record.ray_param);
    record.normal = unit_vec3(record.point - center_);
    record.outside = (r.direction().dot(record.normal) < 0) == (radius_ > 0);
    if (record.outside == (radius_ < 0)) {
        record.normal = -record.normal;
    }
    record.tex_coords = uv_at(record.point);
    record.pmat = pmat_;
    return record;
}
