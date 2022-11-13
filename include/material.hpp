#ifndef RT_MATERIAL_HPP
#define RT_MATERIAL_HPP

#include <utility>

#include "common.hpp"
#include "hittable.hpp"
#include "ray.hpp"
#include "texture.hpp"
#include "utils.hpp"

class material {
public:
    virtual ~material() = default;
    /**
     * @param attenuation r,g,b三色光分别的反/折射率
     * @param scattered 反/折射光线
     */
    virtual bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation,
                         ray &scattered) const = 0;

    [[nodiscard]] virtual color_t emit(tex_coords_t tex_coords, const point_t &point) const {
        return {0, 0, 0};
    }
};

/**
 * 理想漫反射, 反射光均匀向各个方向传播
 */
class lambertian : public material {
    std::shared_ptr<texture> albedo_;

public:
    explicit lambertian(const color_t &albedo) : albedo_(std::make_shared<solid_color>(albedo)) {}

    explicit lambertian(std::shared_ptr<texture> tex) : albedo_(std::move(tex)) {}

    bool scatter(const ray &ray_in, const hit_record &record, color_t &attenuation,
                 ray &scattered) const override {
        // 漫反射方向: 在切点处相切球面上随机选取一点,
        // 切点到此点的方向为漫反射方向 vec3_t direction = record.normal +
        // random_unit_vec();

        vec3_t direction = random_in_hemisphere(record.normal);
        if (direction.len_sq() < 1e-16) {
            direction = record.normal;
        }
        scattered = {record.point, direction};
        attenuation = albedo_->color_at(record.tex_coords, record.point);
        return true;
    }
};

/**
 * 镜面反射 & 模糊效果
 */
class metal : public material {
    std::shared_ptr<texture> albedo_;
    float fuzz_;  // 模糊程度
public:
    explicit metal(const vec3_t &albedo, float f = 0)
        : albedo_(std::make_shared<solid_color>(albedo)), fuzz_(clamp(f, 0, 1)) {}

    explicit metal(std::shared_ptr<texture> tex, float f = 0)
        : albedo_(std::move(tex)), fuzz_(clamp(f, 0, 1)) {}

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation,
                 ray &scattered) const override {
        const vec3_t reflected = reflect(ray_in.direction(), record.normal);
        scattered = {record.point, reflected + fuzz_ * random_in_unit_sphere()};
        attenuation = albedo_->color_at(record.tex_coords, record.point);
        return scattered.direction().dot(record.normal) > 0;
    }
};

/**
 * 透明材质, 随机选择反射或折射
 */
class dielectric : public material {
    float eta_;  // 折射率
public:
    explicit dielectric(float eta) : eta_(eta) {}

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation,
                 ray &scattered) const override {
        attenuation = {1, 1, 1};
        const float eta_ratio = record.outside ? (1.0F / eta_) : eta_;
        const auto refraction = refract(ray_in.direction(), record.normal, eta_ratio);

        // 使用 Schlick's Approximation 计算反射率
        auto reflectance = [&]() {
            const float cos_theta = std::min(-ray_in.direction().dot(record.normal), 1.0F);
            auto r0 = (1 - eta_ratio) / (1 + eta_ratio);
            r0 = r0 * r0;
            return r0 + (1 - r0) * std::pow(1 - cos_theta, 5);
        };

        // 当无法折射或者反射率比较高时使用反射光, 否则使用折射光
        vec3_t out;
        if (!refraction.has_value() || reflectance() > random_float()) {
            out = reflect(ray_in.direction(), record.normal);
        } else {
            out = refraction.value();
        }

        scattered = {record.point, out};
        return true;
    }
};

class light : public material {
    std::shared_ptr<texture> albedo_;

public:
    explicit light(const color_t &color) : albedo_(std::make_shared<solid_color>(color)) {}

    explicit light(std::shared_ptr<texture> tex) : albedo_(std::move(tex)) {}

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation,
                 ray &scattered) const override {
        return false;
    }

    [[nodiscard]] color_t emit(tex_coords_t tex_coords, const point_t &point) const override {
        return albedo_->color_at(tex_coords, point);
    }
};

#endif  // RT_MATERIAL_HPP
