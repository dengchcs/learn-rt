//
// Created by CD on 2022/10/26.
//

#ifndef RT_HITTABLE_H
#define RT_HITTABLE_H

#include "common.h"
#include "ray.h"
#include <memory>
#include <optional>

class material;

struct hit_record {
    point_t point;
    unit_vec3 normal;      // 交点处的法向, 总是与ray成负角度
    float ray_param{0}; // ray的交点参数
    bool outside{true}; // ray来自曲面外还是曲面内; 当球半径为负时"曲面外"是包含球心的一侧
    std::shared_ptr<material> pmat; // 交点处的材质信息
    // 当球半径为正时外法向背离球心; 当球半径为负时外法向指向球心
    [[nodiscard]] unit_vec3 outward_normal() const {
        return outside ? normal : unit_vec3{-normal};
    }
};

class hittable {
public:
    [[nodiscard]] virtual std::optional<hit_record> hit(const ray &r, float tmin, float tmax) const = 0;
};

using world_t = std::vector<std::shared_ptr<hittable>>;

[[nodiscard]] std::optional<hit_record> hit(const world_t &world, const ray &r, float tmin, float tmax) {
    std::optional<hit_record> res = std::nullopt;
    float closest = tmax;
    for (auto &&object: world) {
        const auto record = object->hit(r, tmin, closest);
        if (record.has_value()) {
            closest = record->ray_param;
            res = record;
        }
    }
    return res;
}


#endif //RT_HITTABLE_H
