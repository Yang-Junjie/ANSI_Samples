// by Deepseek v4.1 flash
#include <ansi.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace {

constexpr double PI = 3.14159265358979323846;

double radius_of(const ansi::Renderer& renderer){
    return std::min(renderer.width(), renderer.height()) * 0.4;
}

} // namespace

int main(){
    const int width = 80;
    const int height = 40;

    ansi::Renderer renderer(width, height);
    ansi::CursorGuard cursor_guard;

    const ansi::Vec2 center{width / 2.0, height / 2.0};
    const double radius = radius_of(renderer);

    const ansi::Color colors[3] = {
        {255, 64, 64},
        {64, 255, 96},
        {64, 128, 255},
    };

    double angle = 0.0;
    while(true){
        renderer.clear(ansi::Color{12, 12, 18});

        ansi::Vec2 verts[3];
        for(int i = 0; i < 3; ++i){
            const double theta = angle + i * (2.0 * PI / 3.0);
            verts[i] = ansi::Vec2{
                center.x + radius * std::cos(theta),
                center.y + radius * std::sin(theta),
            };
        }

        ansi::fill_triangle(renderer, verts[0], verts[1], verts[2],
                            colors[0], colors[1], colors[2]);
        renderer.present();

        angle += 0.05;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
