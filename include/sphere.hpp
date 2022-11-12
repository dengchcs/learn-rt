#ifndef RT_SPHERE_HPP
#define RT_SPHERE_HPP

#include <utility>

#include "aabb.hpp"
#include "common.hpp"
#include "hittable.hpp"

class material;

class sphere : public hittable {
    point_t center_;
    float radius_;

public:
    std::shared_ptr<material> pmat_;

public:
    [[nodiscard]] point_t center() const { return center_; }

    [[nodiscard]] float radius() const { return radius_; }

private:
    /**
     * @brief 计算球面一点的参数坐标, 参数范围为[0, 1] * [0, 1]
     */
    void uv_at(const point_t &point, float &u, float &v) const;

public:
    sphere(const point_t &center, float radius, std::shared_ptr<material> m)
        : center_(center), radius_(radius), pmat_(std::move(m)) {}

    [[nodiscard]] std::optional<hit_record> hit(const ray &r, float tmin,
                                                float tmax) const override;

    [[nodiscard]] aabb bounding_box() const override {
        const float absr = std::abs(radius_);
        const vec3_t disp{absr, absr, absr};
        return aabb{center_ - disp, center_ + disp};
    }
};

#endif  // RT_SPHERE_HPP
