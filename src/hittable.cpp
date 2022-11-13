#include "hittable.hpp"

#include "aabb.hpp"

hit_res_t hit(const world_t &world, const ray &r, float tmin, float tmax) {
    hit_res_t res = std::nullopt;
    float closest = tmax;
    for (auto &&object : world) {
        const auto record = object->hit(r, tmin, closest);
        if (record.has_value()) {
            closest = record->ray_param;
            res = record;
        }
    }
    return res;
}

aabb bounding_box(const world_t &world, int start, int end) {
    assert(end - start > 0);
    aabb bound = world[start]->bounding_box();
    for (int i = start + 1; i < end; i++) {
        bound = bound.union_with(world[i]->bounding_box());
    }
    return bound;
}
