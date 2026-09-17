// by Artichoke
#include <iostream>
#include <string>
#include <vector>

constexpr std::string_view RESET = "\x1b[0m";

struct Color {
    int r{}, g{}, b{};
};
void set_color(std::string& out,const Color& c) {
    out += "\x1b[48;2;" + std::to_string(c.r) + ";" 
                        + std::to_string(c.g) + ";" 
                        + std::to_string(c.b) + "m";  
}
void render(std::vector<Color> &frame_buffer, int width, int height) {
    std::string out;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Color& c = frame_buffer[y * width + x];
            set_color(out, c);
            out += "  ";
            out += RESET;
        }
        out += "\n";  
    }
    std::cout << out;
}
int main(){
    const int width = 100;
    const int height = 100;
    std::vector<Color> frame_buffer(width * height, Color{0,0,0}); 
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            frame_buffer[y * width + x] = Color{(x * 255) / (width - 1), (y * 255) / (height - 1), 0};
        }
    }
    render(frame_buffer, width, height);
    return 0;
}
