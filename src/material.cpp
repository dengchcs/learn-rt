#include "material.hpp"

float D_GGX(const float coshn, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float coshn2 = coshn * coshn;
    float divisor = coshn2 * (alpha2 - 1) + 1;
    divisor = divisor * divisor * g_pi + 1e-6;
    float out = alpha2 / divisor;
    return out;
}

float G_Smith(const float coswin, const float coswon, float roughness) {
    float k = (roughness + 1) * (roughness + 1) / 8;
    auto schlick_ggx = [=](float cos) {
        return cos / (cos * (1 - k) + k);
    };
    return schlick_ggx(coswin) * schlick_ggx(coswon);
}

color_t fresnelSchlick(const float coswoh, const color_t& F0) {
    float x = std::pow(1 - coswoh, 5);
    return F0 + (color_t{1, 1, 1} - F0) * x;
}

vector3 hemisphere_rotate(const unit_vec3& up1, const unit_vec3& up2, const vec3_t& v) {
    auto axis = up1.cross(up2);
    float cosine = up1.dot(up2);
    auto angle = std::acos(cosine);
    float sine = std::sin(angle);
    std::array<vec3_t, 3> rotate = {
        vec3_t{cosine + axis[0] * axis[0] * (1 - cosine), axis[0] * axis[1] * (1 - cosine) - axis[2] * sine,
            axis[0] * axis[2] * (1 - cosine) + axis[1] * sine},
        {axis[1] * axis[0] * (1 - cosine) + axis[2] * sine, cosine + axis[1] * axis[1] * (1 - cosine),
            axis[1] * axis[2] * (1 - cosine) - axis[0] * sine},
        {axis[2] * axis[0] * (1 - cosine) - axis[1] * sine, axis[2] * axis[1] * (1 - cosine) + axis[0] * sine,
            cosine + axis[2] * axis[2] * (1 - cosine)}
    };
    return vec3_t{rotate[0].dot(v), rotate[1].dot(v), rotate[2].dot(v)};
}

vec3_t tangent2world(const unit_vec3& normal, const unit_vec3& up, const vec3_t& v) {
    if (auto dist = (normal - up).len(); dist < 1e-6) {
        return v;
    } else if (2 - dist < 1e-6) {
        return vec3_t{-v[0], -v[1], -v[2]};
    }
    auto tangent = normal.cross(up).normalized();
    auto bitangent = normal.cross(tangent).normalized();
    std::array<vec3_t, 3> mat = {
        vec3_t{tangent[0], bitangent[0], normal[0]},
        {tangent[1], bitangent[1], normal[1]},
        {tangent[2], bitangent[2], normal[2]}
    };
    return vec3_t{mat[0].dot(v), mat[1].dot(v), mat[2].dot(v)};
}

float cosine_weighted_sample(const unit_vec3& normal, unit_vec3& sampled) {
    float u = random_float();
    float v = random_float();
    float cos_theta = std::sqrt(1 - u);
    float sin_theta = std::sqrt(u);
    float phi = 2 * g_pi * v;
    sampled = vec3_t{sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta}.normalized();
    const unit_vec3 up = vec3_t{0, 0, 1}.normalized();
    sampled = tangent2world(normal, up, sampled).normalized();
    return cos_theta / g_pi;
}

float uniform_sample(const unit_vec3& normal, unit_vec3& sampled) {
    float u = random_float();
    float v = random_float();
    float r = std::sqrt(1 - u * u);
    float phi = 2 * g_pi * v;
    sampled = vec3_t{r * std::cos(phi), r * std::sin(phi), u}.normalized();
    const unit_vec3 up = vec3_t{0, 0, 1}.normalized();
    sampled = tangent2world(normal, up, sampled).normalized();
    return 1 / (2 * g_pi);
}

float importance_sample_ggx(const unit_vec3& wi, const unit_vec3& normal, unit_vec3& wo, float roughness) {
    float u = random_float();
    float v = random_float();
    float alpha = roughness * roughness;
    float cos_theta = std::sqrt((1 - u) / (1 + (alpha * alpha - 1) * u));
    float sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    float phi = 2 * g_pi * v;
    auto half = vec3_t{sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta}.normalized();
    const unit_vec3 up = vec3_t{0, 0, 1}.normalized();
    half = tangent2world(normal, up, half).normalized();
    wo = (wi.dot(half) * 2 * half - wi).normalized();

    float alpha2 = alpha * alpha;
    float cosnh2 = cos_theta * cos_theta;
    float divisor = cosnh2 * (alpha2 - 1) + 1;
    divisor = divisor * divisor * g_pi + 1e-6;
    float D = alpha2 / divisor;
    return D * cos_theta / (4 * wi.dot(half));
}

bool pbr::scatter(const ray &ray_in, const hit_record &record, vec3_t &attenuation, ray &scattered) const {
    const auto albedo = diffuse_->color_at(record.tex_coords, record.point);
    const auto metalic = metallic_->color_at(record.tex_coords, record.point)[0];
    const auto roughness = roughness_->color_at(record.tex_coords, record.point)[0];

    const auto wi = -ray_in.direction();    // "incident light"
    unit_vec3 wo;   // "viewing direction"
    float pdf;
    if (random_float() < metalic) {
        pdf = importance_sample_ggx(wi, record.normal, wo, roughness);
    } else {
        pdf = cosine_weighted_sample(record.normal, wo);
    }

    assert(wo.dot(record.normal) >= 0);
    const auto half = (wi + wo).normalized();
    const float coswin = wi.dot(record.normal);
    const float coswon = wo.dot(record.normal);
    const float cosnh = record.normal.dot(half);
    const float coswoh = wo.dot(half);

    const auto D = D_GGX(cosnh, roughness);
    color_t F0{0.04, 0.04, 0.04};
    F0 = F0 + (albedo - F0) * metalic;
    const auto F = fresnelSchlick(coswoh, F0);
    const auto G = G_Smith(coswin, coswon, roughness);
    const auto out_specular = D * F * G / (4 * coswon * coswin);

    auto ks = F;
    auto kd = color_t{1, 1, 1} - ks;
    kd *= (1 - metalic);
    const auto out_diffuse = kd * albedo / g_pi;
    attenuation = (out_diffuse + out_specular) * coswin;
    attenuation /= (pdf + 1e-6);
    assert(!std::isnan(attenuation[0]) && !std::isnan(attenuation[1]) && !std::isnan(attenuation[2]));
    scattered = {record.point, wo};
    return true;
}