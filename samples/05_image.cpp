// by Deepseek v4.1 flash
#include <ansi.hpp>

#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

int main(int argc, char** argv){
    const std::string path = argc > 1 ? argv[1] : std::string(ASSET_DIR) + "/image.png";

    int src_w = 0;
    int src_h = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &src_w, &src_h, &channels, 3);
    if(pixels == nullptr){
        std::cerr << "Failed to load '" << path << "': " << stbi_failure_reason() << '\n';
        return 1;
    }

    const int max_width = 200;
    const int max_height = 200;
    const double scale = std::min(max_width / static_cast<double>(src_w),
                                  max_height / static_cast<double>(src_h));
    const int width = std::max(1, static_cast<int>(std::lround(src_w * scale)));
    const int height = std::max(1, static_cast<int>(std::lround(src_h * scale)));

    ansi::Renderer renderer(width, height);

    for(int y = 0; y < height; ++y){
        const int sy_begin = y * src_h / height;
        const int sy_end = std::max(sy_begin + 1, (y + 1) * src_h / height);

        for(int x = 0; x < width; ++x){
            const int sx_begin = x * src_w / width;
            const int sx_end = std::max(sx_begin + 1, (x + 1) * src_w / width);

            long r = 0;
            long g = 0;
            long b = 0;
            long count = 0;
            for(int sy = sy_begin; sy < sy_end; ++sy){
                for(int sx = sx_begin; sx < sx_end; ++sx){
                    const stbi_uc* texel = pixels + (static_cast<std::size_t>(sy) * src_w + sx) * 3;
                    r += texel[0];
                    g += texel[1];
                    b += texel[2];
                    ++count;
                }
            }

            renderer.set_pixel(x, y, ansi::Color{
                static_cast<int>(r / count),
                static_cast<int>(g / count),
                static_cast<int>(b / count),
            });
        }
    }

    ansi::clear_screen();
    renderer.present();
    ansi::set_cursor(0, height);
    std::cout << ansi::RESET << "Loaded " << path
              << "  (" << src_w << 'x' << src_h << " -> " << width << 'x' << height << ")\n";
    std::cout.flush();

    stbi_image_free(pixels);
    return 0;
}
