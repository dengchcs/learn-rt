#include "common.h"
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
    auto record = hit(world, r, 0.001, g_infinity);
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
                    vec3_t albedo = { random_float(0.5, 1), random_float(0.5, 1), random_float(0.5, 1) };
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
    constexpr int width = 600;
    constexpr int height = width / aspect_ratio;

    const auto world = random_world();

    constexpr point_t eye{13, 2, 3};
    constexpr point_t center{0, 0, 0};
    constexpr vec3_t up{0, 1, 0};
    const float dist_to_focus = 10;
    constexpr float aperture = 0.1f;
    camera cam(eye, center, up, g_pi / 9.0, aspect_ratio, aperture, dist_to_focus);

    std::string path = "../images/" + current_time() + ".ppm";
    std::ofstream file(path);
    file << "P3\n" << width << " " << height << "\n255\n";

    auto start = std::chrono::steady_clock::now();
    constexpr int samples_per_pixel = 50;
    constexpr int max_depth = 10;
    for (int j = height - 1; j >= 0; j--) {
        qDebug() << "\r scan lines remaining: " << j << ' ';
        for (int i = 0; i < width; i++) {
            color_t color;
            for (int s = 0; s < samples_per_pixel; s++) {
                const float u = ((float)i + random_float()) / (width - 1);
                const float v = ((float)j + random_float()) / (height - 1);
                ray r = cam.get_ray(u, v);
                color += ray_color(r, world, max_depth);
            }
            write_color(file, color, samples_per_pixel);
        }
    }
    auto end = std::chrono::steady_clock::now();

    qDebug() << "\n done in " << std::chrono::duration<double>(end - start).count() << "s.";
}
