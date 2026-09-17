// by Deepseek v4.1 flash
#pragma once
#include <iostream>
#include <string>

#include "color.hpp"

namespace ansi {

inline void append_cursor(std::string& out, int x, int y){
    out += "\x1b[";
    append_int(out, y + 1);
    out += ';';
    append_int(out, x + 1);
    out += 'H';
}

inline void set_cursor(int x, int y){
    std::string out;
    append_cursor(out, x, y);
    std::cout.write(out.data(), out.size());
}

inline void reset_cursor(){
    std::cout << "\x1b[H";
}

inline void hide_cursor(){
    std::cout << "\x1b[?25l";
    std::cout.flush();
}

inline void show_cursor(){
    std::cout << "\x1b[?25h";
    std::cout.flush();
}

inline void clear_screen(){
    std::cout << "\x1b[2J";
    std::cout.flush();
}

class CursorGuard {
public:
    CursorGuard(){ hide_cursor(); }
    ~CursorGuard(){ show_cursor(); }

    CursorGuard(const CursorGuard&) = delete;
    CursorGuard& operator=(const CursorGuard&) = delete;
};

} // namespace ansi
