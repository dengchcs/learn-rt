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
     * @param attenuation r,g,b三色光分别的反射率
     * @param scattered 反射光线
     */
    virtual bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const = 0;
};

class lambertian : public material {
public:
    color_t albedo;
public:
    explicit lambertian(const color_t &color) : albedo(color) {}

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const override {
        vec3_t direction = record.normal + random_unit_vec();
        if (direction.lengthSquared() < 1e-16) {
            direction = record.normal;
        }
        scattered = {record.point, direction};
        attenuation = albedo;
        return true;
    }
};

class metal : public material {
public:
    color_t albedo;
    float fuzz;
public:
    metal(const color_t &color, float f) : albedo(color) {
        fuzz = clamp(f, 0, 1);
    }

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const override {
        const vec3_t reflected = reflect(ray_in.direction(), record.normal);
        scattered = {record.point, reflected + fuzz * random_in_unit_sphere()};
        attenuation = albedo;
        return QVector3D::dotProduct(scattered.direction(), record.normal) > 0;
    }
};

class dielectric : public material {
    float eta_;
public:
    explicit dielectric(float eta) {
        eta_ = eta;
    }

    bool scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const override {
        attenuation = {1, 1, 1};
        const float eta_ratio = record.outside ? (1.0f / eta_) : eta_;
        const auto refraction = refract(ray_in.direction(), record.normal, eta_ratio);
        const auto out_dire = refraction.value_or(unit_vec3{
                reflect(ray_in.direction(), record.normal)
        });

        scattered = {record.point, out_dire};
        return true;
    }
};

#endif //RT_MATERIAL_H
