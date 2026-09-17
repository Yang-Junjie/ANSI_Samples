// by Artichoke 
#pragma once
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "color.hpp"
#include "terminal.hpp"

namespace ansi {

class Renderer {
public:
    Renderer(int width, int pixel_height)
        : width_(width), height_(pixel_height) {
        frame_.assign(static_cast<std::size_t>(width) * pixel_height, Color{255, 255, 255});
        prev_.assign(static_cast<std::size_t>(width) * pixel_height, Color{255, 255, 255});
    }

    void clear(Color clear_color){
        std::fill(frame_.begin(), frame_.end(), clear_color);
    }

    bool in_bounds(int x, int y) const {
        return x >= 0 && x < width_ && y >= 0 && y < height_;
    }

    void set_pixel(int x, int y, Color color){
        if(!in_bounds(x, y)) return;
        frame_[static_cast<std::size_t>(y) * width_ + x] = color;
    }

    Color get_pixel(int x, int y) const {
        if(!in_bounds(x, y)) return Color{0, 0, 0};
        return frame_[static_cast<std::size_t>(y) * width_ + x];
    }

    int width() const { return width_; }
    int height() const { return height_; }

    void present(){
        out_.clear();
        out_.reserve(static_cast<std::size_t>(width_) * height_ * 20);

        for(int y = 0; y < height_; ++y){
            for(int x = 0; x < width_; ++x){
                const std::size_t idx = static_cast<std::size_t>(y) * width_ + x;
                const Color color = frame_[idx];

                if(!dirty_all_ && color == prev_[idx]) continue;

                append_cursor(out_, x * 2, y);
                append_bg(out_, color.r, color.g, color.b);
                out_ += "  ";
            }
        }

        out_ += RESET;
        std::cout.write(out_.data(), out_.size());
        std::cout.flush();

        prev_ = frame_;
        dirty_all_ = false;
    }

    void present_pixel(int x, int y){
        if(!in_bounds(x, y)) return;

        const std::size_t idx = static_cast<std::size_t>(y) * width_ + x;
        const Color color = frame_[idx];

        out_.clear();
        append_cursor(out_, x * 2, y);
        append_bg(out_, color.r, color.g, color.b);
        out_ += "  ";
        out_ += RESET;
        std::cout.write(out_.data(), out_.size());
        std::cout.flush();

        prev_[idx] = color;
    }

private:
    std::vector<Color> frame_;
    std::vector<Color> prev_;
    int width_;
    int height_;
    bool dirty_all_ = true;
    std::string out_;
};

} // namespace ansi
