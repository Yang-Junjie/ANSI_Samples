// by Deepseek v4.1 flash
#pragma once
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace ansi {

enum class Key {
    None,
    Up,
    Down,
    Left,
    Right,
    Escape,
    Quit,
    Restart,
    Space,
    Enter,
};

inline void sleep_ms(int milliseconds){
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

class Input {
public:
    Input(){
#ifdef _WIN32
        active_ = true;
#else
        if(tcgetattr(STDIN_FILENO, &original_) == 0){
            struct termios raw = original_;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            if(tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) active_ = true;
        }
#endif
    }

    ~Input(){
        if(!active_) return;
#ifndef _WIN32
        tcsetattr(STDIN_FILENO, TCSANOW, &original_);
#endif
    }

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    Key poll(){
        if(!active_) return Key::None;

#ifdef _WIN32
        if(!_kbhit()) return Key::None;

        const int ch = _getch();
        if(ch < 0) return Key::None;
        if(ch == 0 || ch == 0xE0){
            const int scan = _getch();
            if(scan < 0) return Key::None;
            switch(scan){
                case 72: return Key::Up;
                case 80: return Key::Down;
                case 75: return Key::Left;
                case 77: return Key::Right;
                default: return Key::None;
            }
        }
        return translate(ch);
#else
        unsigned char ch = 0;
        if(read(STDIN_FILENO, &ch, 1) != 1) return Key::None;

        if(ch == 27){
            unsigned char seq[2] = {};
            if(read(STDIN_FILENO, &seq[0], 1) != 1) return Key::Escape;
            if(read(STDIN_FILENO, &seq[1], 1) != 1) return Key::Escape;
            if(seq[0] == '['){
                switch(seq[1]){
                    case 'A': return Key::Up;
                    case 'B': return Key::Down;
                    case 'C': return Key::Right;
                    case 'D': return Key::Left;
                    default: return Key::None;
                }
            }
            return Key::Escape;
        }
        return translate(static_cast<int>(ch));
#endif
    }

private:
    static Key translate(int ch){
        switch(ch){
            case 'w': case 'W': return Key::Up;
            case 's': case 'S': return Key::Down;
            case 'a': case 'A': return Key::Left;
            case 'd': case 'D': return Key::Right;
            case 'q': case 'Q': return Key::Quit;
            case 'r': case 'R': return Key::Restart;
            case ' ': return Key::Space;
            case '\r': case '\n': return Key::Enter;
            case 27: return Key::Escape;
            default: return Key::None;
        }
    }

    bool active_ = false;
#ifndef _WIN32
    struct termios original_{};
#endif
};

} // namespace ansi
