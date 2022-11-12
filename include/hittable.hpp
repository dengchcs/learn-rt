#ifndef RT_HITTABLE_HPP
#define RT_HITTABLE_HPP

#include <memory>
#include <optional>
#include <vector>

#include "common.hpp"

class aabb;
class material;
class ray;

struct hit_record {
    point_t point;
    unit_vec3 normal;                // 交点处的法向, 总是与ray成负角度
    float ray_param{0};              // ray的交点参数
    float u{0}, v{0};                // 交点的曲面参数
    bool outside{true};              // ray来自曲面外还是曲面内;
                                     // 当球半径为负时"曲面外"是包含球心的一侧
    std::shared_ptr<material> pmat;  // 交点处的材质信息
};

class hittable {
public:
    [[nodiscard]] virtual std::optional<hit_record> hit(const ray &r, float tmin,
                                                        float tmax) const = 0;

    [[nodiscard]] virtual aabb bounding_box() const = 0;
};

using world_t = std::vector<std::shared_ptr<hittable>>;

[[nodiscard]] std::optional<hit_record> hit(const world_t &world, const ray &r, float tmin,
                                            float tmax);

[[nodiscard]] aabb bounding_box(const world_t &world, int start, int end);

#endif  // RT_HITTABLE_HPP
