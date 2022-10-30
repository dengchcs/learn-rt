#include "common.h"
#include "bvh_node.h"
#include "camera.h"
#include "material.h"
#include "ray.h"
#include "sphere.h"
#include "utils.h"

#include <QDebug>
#include <fstream>

color_t ray_color(const ray &r, const world_t &world, int depth) {
    if (depth <= 0) {
        return {0, 0, 0};
    }
    auto record = hit(world, r, 0.001, g_infinity); // 忽略距离太近的光线
    if (record.has_value()) {
        ray scattered;
        color_t attenuation;
        if (record->pmat->scatter(r, record.value(), attenuation, scattered)) {
            return attenuation * ray_color(scattered, world, depth - 1);
        }
        return color_t{0, 0, 0};
    }
    float t = (r.direction().y() + 1.0f) / 2.0f;
    return (1.0f - t) * color_t{1.0, 1.0, 1.0} + t * color_t{0.5, 0.7, 1.0};
}

color_t ray_color(const ray &r, const bvh_node &bvh, int depth) {
    if (depth <= 0) {
        return {0, 0, 0};
    }
    auto record = bvh.hit(r, 0.001, g_infinity);    // 忽略距离太近的光线
    if (record.has_value()) {
        ray scattered;
        color_t attenuation;
        if (record->pmat->scatter(r, record.value(), attenuation, scattered)) {
            return attenuation * ray_color(scattered, bvh, depth - 1);
        }
        return color_t{0, 0, 0};
    }
    float t = (r.direction().y() + 1.0f) / 2.0f;
    return (1.0f - t) * color_t{1.0, 1.0, 1.0} + t * color_t{0.5, 0.7, 1.0};
}

world_t random_world() {
    auto material_ground = std::make_shared<lambertian>(color_t{0.8, 0.8, 0.0});

    world_t world;
    world.push_back(std::make_shared<sphere>(point_t{0, -1000, 0}, 1000, material_ground));
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_float();
            point_t center(a + 0.9f * random_float(), 0.2, b + 0.9f * random_float());

            if ((center - point_t(4, 0.2, 0)).length() > 0.9) {
                std::shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    vec3_t albedo = {random_float(), random_float(), random_float()};
                    sphere_material = std::make_shared<lambertian>(albedo);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    vec3_t albedo = {random_float(0.5, 1), random_float(0.5, 1), random_float(0.5, 1)};
                    auto fuzz = random_float(0, 0.5);
                    sphere_material = std::make_shared<metal>(albedo, fuzz);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = std::make_shared<dielectric>(1.5);
                    world.push_back(std::make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = std::make_shared<dielectric>(1.5);
    world.push_back(std::make_shared<sphere>(point_t(0, 1, 0), 1.0, material1));

    auto material2 = std::make_shared<lambertian>(vec3_t{0.4, 0.2, 0.1});
    world.push_back(std::make_shared<sphere>(point_t(-4, 1, 0), 1.0, material2));

    auto material3 = std::make_shared<metal>(vec3_t(0.7, 0.6, 0.5), 0.0);
    world.push_back(std::make_shared<sphere>(point_t(4, 1, 0), 1.0, material3));

    return world;
}

int main(int argc, char *argv[]) {
    constexpr float aspect_ratio = 3.0 / 2.0;
    constexpr int width = 300;
    constexpr int height = width / aspect_ratio;

    /*
    const auto world = random_world();
    point_t lookfrom(13, 2, 3);
    point_t lookat(0, 0, 0);
    vec3_t vup(0, 1, 0);
    float dist_to_focus = 10.0;
    float aperture = 0.1;
    camera cam(lookfrom, lookat, vup, g_pi / 9.0, aspect_ratio, aperture, dist_to_focus);
    */


    world_t world;
    auto material_ground = std::make_shared<lambertian>(color_t{0.2, 0.8, 0.2});
    world.push_back(std::make_shared<sphere>(point_t{0, -100.5, 0}, 100, material_ground));
    auto mat1 = std::make_shared<dielectric>(1.5);
    auto mat2 = std::make_shared<lambertian>(vec3_t{1.0, 0.4, 0.4});
    auto mat3 = std::make_shared<metal>(vec3_t{0.2, 0.5, 0.8}, 0.1);
    world.push_back(std::make_shared<sphere>(point_t(-1, 0, -1), 0.5, mat1));
    world.push_back(std::make_shared<sphere>(point_t(-1, 0, -1), -0.45, mat1));
    world.push_back(std::make_shared<sphere>(point_t(0, 0, -1), 0.5, mat2));
    world.push_back(std::make_shared<sphere>(point_t(1, 0, -1), 0.5, mat3));
    constexpr point_t eye{3, 3, 2};
    constexpr point_t center{0, 0, -1};
    constexpr vec3_t up{0, 1, 0};
    const float dist_to_focus = (eye - center).length();
    constexpr float aperture = 2.0f;
    camera cam(eye, center, up, g_pi / 9.0, aspect_ratio, aperture, dist_to_focus);

    const bvh_node bvh{world, 0, world.size()};

    std::string path = "../images/" + current_time() + ".ppm";
    std::ofstream file(path);
    file << "P3\n" << width << " " << height << "\n255\n";

    auto start = std::chrono::steady_clock::now();
    constexpr int samples_per_pixel = 500;
    constexpr int max_depth = 100;
    for (int j = height - 1; j >= 0; j--) {
        qDebug() << "\r scan lines remaining: " << j << ' ';
        for (int i = 0; i < width; i++) {
            // 抗锯齿: 对每个像素作多次采样取平均值
            color_t color;
            for (int s = 0; s < samples_per_pixel; s++) {
                const float u = ((float) i + random_float()) / (width - 1);
                const float v = ((float) j + random_float()) / (height - 1);
                ray r = cam.get_ray_no_blur(u, v);
                // color += ray_color(r, world, max_depth);
                color += ray_color(r, bvh, max_depth);
            }
            write_color(file, color, samples_per_pixel);
        }
    }
    auto end = std::chrono::steady_clock::now();

    qDebug() << "\n done in " << std::chrono::duration<double>(end - start).count() << "s.";
}
