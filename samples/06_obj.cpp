// by Deepseek v4.1 flash
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <ansi.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

namespace {

constexpr double kPI = 3.14159265358979323846;

struct Material {
    ansi::Vec3 ambient;
    ansi::Vec3 diffuse;
    ansi::Vec3 specular;
    double shininess{};
};

struct Triangle {
    int a{};
    int b{};
    int c{};
};

struct Shading {
    ansi::Vec3 light_dir;
    ansi::Vec3 eye;
    ansi::Vec3 ambient_light;
    ansi::Vec3 light_color;
};

const Material kMaterials[] = {
    {{0.20, 0.19, 0.18}, {0.88, 0.86, 0.82}, {0.05, 0.05, 0.05}, 12.0},
    {{0.14, 0.08, 0.05}, {0.90, 0.45, 0.30}, {0.85, 0.62, 0.50}, 45.0},
};

constexpr int kMaterialCount = sizeof(kMaterials) / sizeof(kMaterials[0]);

ansi::Color blinn_phong(const ansi::Vec3& position, const ansi::Vec3& normal,
                        const Material& material, const Shading& shading){
    const ansi::Vec3 view = ansi::normalize(shading.eye - position);
    const ansi::Vec3 halfway = ansi::normalize(shading.light_dir + view);
    const double n_dot_l = std::max(0.0, ansi::dot(normal, shading.light_dir));
    const double n_dot_h = std::max(0.0, ansi::dot(normal, halfway));
    const double spec = n_dot_l > 0.0 ? std::pow(n_dot_h, material.shininess) : 0.0;

    const auto channel = [&](double ambient, double diffuse, double specular,
                             double ambient_light, double light_color){
        const double value = ambient * ambient_light
                           + diffuse * n_dot_l * light_color
                           + specular * spec * light_color;
        return std::clamp(static_cast<int>(std::lround(value * 255.0)), 0, 255);
    };

    return {
        channel(material.ambient.x, material.diffuse.x, material.specular.x,
                shading.ambient_light.x, shading.light_color.x),
        channel(material.ambient.y, material.diffuse.y, material.specular.y,
                shading.ambient_light.y, shading.light_color.y),
        channel(material.ambient.z, material.diffuse.z, material.specular.z,
                shading.ambient_light.z, shading.light_color.z),
    };
}

void rasterize_triangle(ansi::Renderer& renderer, std::vector<double>& depth_buffer,
                        const ansi::Vec3& p0, const ansi::Vec3& p1, const ansi::Vec3& p2,
                        const ansi::Vec3& n0, const ansi::Vec3& n1, const ansi::Vec3& n2,
                        const Material& material, const Shading& shading,
                        double focal, const ansi::Vec2& center, double distance){
    const double d0 = p0.z + distance;
    const double d1 = p1.z + distance;
    const double d2 = p2.z + distance;
    if(d0 <= 1e-3 || d1 <= 1e-3 || d2 <= 1e-3) return;

    const ansi::Vec2 s0{center.x + p0.x * focal / d0, center.y - p0.y * focal / d0};
    const ansi::Vec2 s1{center.x + p1.x * focal / d1, center.y - p1.y * focal / d1};
    const ansi::Vec2 s2{center.x + p2.x * focal / d2, center.y - p2.y * focal / d2};

    const double area = ansi::edge(s0, s1, s2);
    if(std::abs(area) < 1e-9) return;
    const double inv_area = 1.0 / area;

    const int width = renderer.width();
    const int height = renderer.height();

    const int x_begin = std::max(0, static_cast<int>(std::floor(std::min({s0.x, s1.x, s2.x}))));
    const int x_end = std::min(width - 1, static_cast<int>(std::ceil(std::max({s0.x, s1.x, s2.x}))));
    const int y_begin = std::max(0, static_cast<int>(std::floor(std::min({s0.y, s1.y, s2.y}))));
    const int y_end = std::min(height - 1, static_cast<int>(std::ceil(std::max({s0.y, s1.y, s2.y}))));

    const double inv0 = 1.0 / d0;
    const double inv1 = 1.0 / d1;
    const double inv2 = 1.0 / d2;

    for(int y = y_begin; y <= y_end; ++y){
        for(int x = x_begin; x <= x_end; ++x){
            const ansi::Vec2 p{x + 0.5, y + 0.5};

            const double w0 = ansi::edge(s1, s2, p) * inv_area;
            const double w1 = ansi::edge(s2, s0, p) * inv_area;
            const double w2 = ansi::edge(s0, s1, p) * inv_area;
            if(w0 < 0.0 || w1 < 0.0 || w2 < 0.0) continue;

            const double inv_z = w0 * inv0 + w1 * inv1 + w2 * inv2;
            if(inv_z <= 0.0) continue;

            const double z = 1.0 / inv_z;
            const std::size_t idx = static_cast<std::size_t>(y) * width + x;
            if(z >= depth_buffer[idx]) continue;
            depth_buffer[idx] = z;

            const double b0 = w0 * inv0 / inv_z;
            const double b1 = w1 * inv1 / inv_z;
            const double b2 = w2 * inv2 / inv_z;

            const ansi::Vec3 position{
                p0.x * b0 + p1.x * b1 + p2.x * b2,
                p0.y * b0 + p1.y * b1 + p2.y * b2,
                p0.z * b0 + p1.z * b1 + p2.z * b2,
            };
            const ansi::Vec3 normal = ansi::normalize(ansi::Vec3{
                n0.x * b0 + n1.x * b1 + n2.x * b2,
                n0.y * b0 + n1.y * b1 + n2.y * b2,
                n0.z * b0 + n1.z * b1 + n2.z * b2,
            });

            renderer.set_pixel(x, y, blinn_phong(position, normal, material, shading));
        }
    }
}

bool load_model(const std::string& path,
                std::vector<ansi::Vec3>& out_positions,
                std::vector<ansi::Vec3>& out_normals,
                std::vector<Triangle>& out_triangles){
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())){
        std::cerr << "Failed to load '" << path << "': " << warn << err << '\n';
        return false;
    }

    std::vector<ansi::Vec3> positions;
    positions.reserve(attrib.vertices.size() / 3);
    for(std::size_t i = 0; i + 2 < attrib.vertices.size(); i += 3){
        positions.push_back(ansi::Vec3{
            attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2],
        });
    }

    std::vector<ansi::Vec3> normals(positions.size(), ansi::Vec3{0.0, 0.0, 0.0});
    std::vector<Triangle> triangles;

    for(const tinyobj::shape_t& shape : shapes){
        const std::vector<tinyobj::index_t>& indices = shape.mesh.indices;
        for(std::size_t i = 0; i + 2 < indices.size(); i += 3){
            const int a = indices[i].vertex_index;
            const int b = indices[i + 1].vertex_index;
            const int c = indices[i + 2].vertex_index;
            triangles.push_back(Triangle{a, b, c});

            const ansi::Vec3 face = ansi::cross(positions[b] - positions[a],
                                                positions[c] - positions[a]);
            normals[a] += face;
            normals[b] += face;
            normals[c] += face;
        }
    }

    if(triangles.empty()){
        std::cerr << "Model '" << path << "' contains no triangles\n";
        return false;
    }

    for(ansi::Vec3& normal : normals){
        normal = ansi::normalize(normal);
    }

    ansi::Vec3 min{std::numeric_limits<double>::max(),
                   std::numeric_limits<double>::max(),
                   std::numeric_limits<double>::max()};
    ansi::Vec3 max{std::numeric_limits<double>::lowest(),
                   std::numeric_limits<double>::lowest(),
                   std::numeric_limits<double>::lowest()};
    for(const ansi::Vec3& p : positions){
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }

    const ansi::Vec3 center = (min + max) * 0.5;
    const ansi::Vec3 extent = max - min;
    const double max_extent = std::max({extent.x, extent.y, extent.z});
    const double scale = max_extent > 0.0 ? 2.0 / max_extent : 1.0;

    out_positions.resize(positions.size());
    for(std::size_t i = 0; i < positions.size(); ++i){
        out_positions[i] = (positions[i] - center) * scale;
    }
    out_normals = std::move(normals);
    out_triangles = std::move(triangles);
    return true;
}

} // namespace

int main(int argc, char** argv){
    const std::string path = argc > 1 ? argv[1] : std::string(ASSET_DIR) + "/teapot.obj";

    std::vector<ansi::Vec3> positions;
    std::vector<ansi::Vec3> normals;
    std::vector<Triangle> triangles;
    if(!load_model(path, positions, normals, triangles)) return 1;

    std::cout << "Loaded " << path << ": " << positions.size() << " vertices, "
              << triangles.size() << " triangles\n";

    const int width = 160;
    const int height = 80;
    const double distance = 3.5;
    const double focal = std::min(width, height) * 1.1;
    const ansi::Vec2 center{width / 2.0, height / 2.0};

    const Shading shading{
        ansi::normalize(ansi::Vec3{0.6, 0.9, -0.8}),
        ansi::Vec3{0.0, 0.0, -distance},
        ansi::Vec3{0.14, 0.15, 0.20},
        ansi::Vec3{1.0, 0.97, 0.92},
    };

    ansi::Renderer renderer(width, height);
    ansi::CursorGuard cursor_guard;

    constexpr int kTeapotCount = 2;
    constexpr double kModelScale = 1.0;
    constexpr double kSpacing = 2.5;
    const double phase_step = 2.0 * kPI / kTeapotCount;

    std::vector<double> depth_buffer(static_cast<std::size_t>(width) * height);
    std::vector<ansi::Vec3> scene_positions(positions.size() * kTeapotCount);
    std::vector<ansi::Vec3> scene_normals(normals.size() * kTeapotCount);

    double ax = 0.0;
    double ay = 0.0;

    while(true){
        renderer.clear(ansi::Color{10, 10, 14});
        std::fill(depth_buffer.begin(), depth_buffer.end(),
                  std::numeric_limits<double>::infinity());

        const double cx = std::cos(ax);
        const double sx = std::sin(ax);

        for(int i = 0; i < kTeapotCount; ++i){
            const double cy = std::cos(ay + i * phase_step);
            const double sy = std::sin(ay + i * phase_step);
            const double offset_x = (i - (kTeapotCount - 1) * 0.5) * kSpacing;
            const std::size_t base = static_cast<std::size_t>(i) * positions.size();

            for(std::size_t v = 0; v < positions.size(); ++v){
                const ansi::Vec3& p = positions[v];
                const ansi::Vec3 rx{p.x, p.y * cx - p.z * sx, p.y * sx + p.z * cx};
                const ansi::Vec3 ry{rx.x * cy + rx.z * sy, rx.y, -rx.x * sy + rx.z * cy};
                scene_positions[base + v] = ansi::Vec3{
                    ry.x * kModelScale + offset_x,
                    ry.y * kModelScale,
                    ry.z * kModelScale,
                };
            }

            for(std::size_t v = 0; v < normals.size(); ++v){
                const ansi::Vec3& n = normals[v];
                const ansi::Vec3 rx{n.x, n.y * cx - n.z * sx, n.y * sx + n.z * cx};
                scene_normals[base + v] = ansi::Vec3{
                    rx.x * cy + rx.z * sy, rx.y, -rx.x * sy + rx.z * cy,
                };
            }
        }

        for(int i = 0; i < kTeapotCount; ++i){
            const std::size_t base = static_cast<std::size_t>(i) * positions.size();
            const Material& material = kMaterials[i % kMaterialCount];
            for(std::size_t t = 0; t < triangles.size(); ++t){
                const Triangle& tri = triangles[t];
                rasterize_triangle(renderer, depth_buffer,
                    scene_positions[base + tri.a], scene_positions[base + tri.b],
                    scene_positions[base + tri.c],
                    scene_normals[base + tri.a], scene_normals[base + tri.b],
                    scene_normals[base + tri.c],
                    material, shading, focal, center, distance);
            }
        }

        renderer.present();

        ay += 0.06;
        ax += 0.021;
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    return 0;
}
