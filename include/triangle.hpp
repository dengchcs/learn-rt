#ifndef RT_TRIANGLE_HPP
#define RT_TRIANGLE_HPP

#include <iostream>

#include "hittable.hpp"

class triangle : public hittable {
    std::array<point_t, 3> vertices_;
    unit_vec3 normal_;      // 顶点按右手方向旋转得到的法向
    float dist_to_origin_;  // A*x+B*y+C*z+D=0中的D
    std::shared_ptr<material> pmat_;
    using tex_coords_t = std::pair<float, float>;
    std::array<tex_coords_t, 3> tex_coords_;
    std::array<vec3_t, 3> edges_;
    float area2_;

public:
    triangle(const point_t &p0, const point_t &p1, const point_t &p2, float tex[6],
             std::shared_ptr<material> pmat)
        : vertices_{p0, p1, p2},
          pmat_(std::move(pmat)),
          tex_coords_{tex_coords_t{tex[0], tex[1]}, {tex[2], tex[3]}, {tex[4], tex[5]}} {
        normal_ = (p1 - p0).cross(p2 - p1).normalized();
        if (normal_.len_sq() == 0) {
            std::cerr << "bad triangle: collinear edges\n";
        }
        dist_to_origin_ = -normal_.dot(p0);
        edges_ = {p2 - p1, p0 - p2, p1 - p0};
        area2_ = edges_[0].cross(edges_[1]).len();
    }

private:
    [[nodiscard]] point_t vertex(int i) const { return vertices_[i % 3]; }

public:
    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin,
                                                float tmax) const override;

    [[nodiscard]] aabb bounding_box() const override;
};

#endif  // RT_TRIANGLE_HPP
