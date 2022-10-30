//
// Created by CD on 2022/10/26.
//

#ifndef RT_MATERIAL_H
#define RT_MATERIAL_H

#include "common.h"
#include "hittable.h"
#include "ray.h"
#include "utils.h"

class material {
public:
    /**
     * @param attenuation r,g,b三色光分别的反/折射率
     * @param scattered 反/折射光线
     */
    virtual bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const = 0;
};

/**
 * 理想漫反射, 反射光均匀向各个方向传播
 */
class lambertian : public material {
public:
    vec3_t albedo_;
public:
    explicit lambertian(const vec3_t &albedo) : albedo_(albedo) {}

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const override {
        // 漫反射方向: 在切点处相切球面上随机选取一点, 切点到此点的方向为漫反射方向
        // vec3_t direction = record.normal + random_unit_vec();

        vec3_t direction = random_in_hemisphere(record.normal);
        if (direction.lengthSquared() < 1e-16) {
            direction = record.normal;
        }
        scattered = {record.point, direction};
        attenuation = albedo_;
        return true;
    }
};

/**
 * 镜面反射 & 模糊效果
 */
class metal : public material {
public:
    vec3_t albedo_;
    float fuzz_;    // 模糊程度
public:
    explicit metal(const vec3_t &albedo, float f = 0) : albedo_(albedo) {
        fuzz_ = clamp(f, 0, 1);
    }

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const override {
        const vec3_t reflected = reflect(ray_in.direction(), record.normal);
        scattered = {record.point, reflected + fuzz_ * random_in_unit_sphere()};
        attenuation = albedo_;
        return QVector3D::dotProduct(scattered.direction(), record.normal) > 0;
    }
};

/**
 * 透明材质, 随机选择反射或折射
 */
class dielectric : public material {
    float eta_; // 折射率
public:
    explicit dielectric(float eta) {
        eta_ = eta;
    }

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const override {
        attenuation = {1, 1, 1};
        const float eta_ratio = record.outside ? (1.0f / eta_) : eta_;
        const auto refraction = refract(ray_in.direction(), record.normal, eta_ratio);

        // 使用 Schlick's Approximation 计算反射率
        auto reflectance = [&]() {
            const float cos_theta = std::min(QVector3D::dotProduct(-ray_in.direction(), record.normal), 1.0f);
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

#endif //RT_MATERIAL_H
