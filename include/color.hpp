// by Artichoke
#pragma once
#include <charconv>
#include <string>
#include <string_view>

namespace ansi {

struct Color {
    int r{}, g{}, b{};

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};

inline constexpr std::string_view RESET = "\x1b[0m";

inline void append_int(std::string& out, int value){
    char buf[12];
    auto res = std::to_chars(buf, buf + sizeof(buf), value);
    out.append(buf, res.ptr);
}

inline void append_bg(std::string& out, int r, int g, int b){
    out += "\x1b[48;2;";
    append_int(out, r);
    out += ';';
    append_int(out, g);
    out += ';';
    append_int(out, b);
    out += 'm';
}

} // namespace ansi
