// by Deepseek v4.1 flash
#include <ansi.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

namespace {

struct Face {
    int a{}, b{}, c{}, d{};
    ansi::Color color;
};

const std::array<ansi::Vec3, 8> kCorners = {{
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1,  1}, {1, -1,  1}, {1, 1,  1}, {-1, 1,  1},
}};

const std::array<Face, 6> kFaces = {{
    {0, 3, 2, 1, {230,  80,  80}},
    {4, 5, 6, 7, { 80, 230, 110}},
    {0, 1, 5, 4, { 80, 150, 240}},
    {3, 7, 6, 2, {240, 200,  70}},
    {0, 4, 7, 3, {200,  90, 220}},
    {1, 2, 6, 5, { 70, 220, 220}},
}};

ansi::Color shade(ansi::Color color, double intensity){
    const auto channel = [intensity](int value){
        return std::clamp(static_cast<int>(std::lround(value * intensity)), 0, 255);
    };
    return {channel(color.r), channel(color.g), channel(color.b)};
}

} // namespace

int main(){
    const int width = 160;
    const int height = 80;
    const double distance = 4.0;
    const double focal = std::min(width, height) * 0.9;
    const ansi::Vec3 camera{0.0, 0.0, -distance};
    const ansi::Vec3 light = ansi::normalize(ansi::Vec3{0.5, 0.8, -1.0});

    ansi::Renderer renderer(width, height);
    ansi::CursorGuard cursor_guard;

    const ansi::Vec2 center{width / 2.0, height / 2.0};

    double ax = 0.0;
    double ay = 0.0;
    double az = 0.0;

    while(true){
        renderer.clear(ansi::Color{12, 12, 18});

        std::array<ansi::Vec3, 8> rotated;
        for(std::size_t i = 0; i < kCorners.size(); ++i){
            ansi::Vec3 p = ansi::rotate_x(kCorners[i], ax);
            p = ansi::rotate_y(p, ay);
            rotated[i] = ansi::rotate_z(p, az);
        }

        struct ProjectedFace {
            std::array<ansi::Vec2, 4> screen;
            ansi::Color color;
            double depth{};
        };

        std::vector<ProjectedFace> visible;
        visible.reserve(kFaces.size());

        for(const Face& face : kFaces){
            const ansi::Vec3& p0 = rotated[face.a];
            const ansi::Vec3& p1 = rotated[face.b];
            const ansi::Vec3& p2 = rotated[face.c];

            ansi::Vec3 normal = ansi::normalize(ansi::cross(p1 - p0, p2 - p0));
            const ansi::Vec3 centroid = (p0 + p1 + rotated[face.c] + rotated[face.d]) * 0.25;

            if(ansi::dot(normal, centroid) < 0.0) normal = normal * -1.0;
            if(ansi::dot(normal, camera - centroid) <= 0.0) continue;

            ProjectedFace projected;
            const int indices[4] = {face.a, face.b, face.c, face.d};
            for(int i = 0; i < 4; ++i){
                const ansi::Vec3& p = rotated[indices[i]];
                const double k = focal / (p.z + distance);
                projected.screen[i] = ansi::Vec2{
                    center.x + p.x * k,
                    center.y - p.y * k,
                };
                projected.depth += p.z;
            }
            projected.depth *= 0.25;

            const double intensity = 0.35 + 0.65 * std::max(0.0, ansi::dot(normal, light));
            projected.color = shade(face.color, intensity);
            visible.push_back(projected);
        }

        std::sort(visible.begin(), visible.end(), [](const ProjectedFace& a, const ProjectedFace& b){
            return a.depth > b.depth;
        });

        for(const ProjectedFace& face : visible){
            ansi::fill_triangle(renderer, face.screen[0], face.screen[1], face.screen[2],
                                face.color, face.color, face.color);
            ansi::fill_triangle(renderer, face.screen[0], face.screen[2], face.screen[3],
                                face.color, face.color, face.color);
        }

        renderer.present();

        ax += 0.040;
        ay += 0.055;
        az += 0.020;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
