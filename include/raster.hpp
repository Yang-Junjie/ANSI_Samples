// by Deepseek v4.1 flash
#pragma once
#include <algorithm>
#include <cmath>

#include "color.hpp"
#include "math.hpp"
#include "renderer.hpp"

namespace ansi {

inline int interpolate_channel(int c0, int c1, int c2, double w0, double w1, double w2){
    return static_cast<int>(std::lround(w0 * c0 + w1 * c1 + w2 * c2));
}

inline void fill_triangle(Renderer& renderer,
                          const Vec2& p0, const Vec2& p1, const Vec2& p2,
                          const Color& c0, const Color& c1, const Color& c2){
    const double area = edge(p0, p1, p2);
    if(std::abs(area) < 1e-9) return;

    const int x_begin = std::max(0, static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x}))));
    const int x_end = std::min(renderer.width() - 1, static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x}))));
    const int y_begin = std::max(0, static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y}))));
    const int y_end = std::min(renderer.height() - 1, static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y}))));

    const double inv_area = 1.0 / area;

    for(int y = y_begin; y <= y_end; ++y){
        for(int x = x_begin; x <= x_end; ++x){
            const Vec2 p{x + 0.5, y + 0.5};

            const double w0 = edge(p1, p2, p) * inv_area;
            const double w1 = edge(p2, p0, p) * inv_area;
            const double w2 = edge(p0, p1, p) * inv_area;

            if(w0 < 0.0 || w1 < 0.0 || w2 < 0.0) continue;

            renderer.set_pixel(x, y, Color{
                interpolate_channel(c0.r, c1.r, c2.r, w0, w1, w2),
                interpolate_channel(c0.g, c1.g, c2.g, w0, w1, w2),
                interpolate_channel(c0.b, c1.b, c2.b, w0, w1, w2),
            });
        }
    }
}

} // namespace ansi
