// by Deepseek v4.1 flash
#include <ansi.hpp>

#include <chrono>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

struct Cell {
    int x{};
    int y{};
};

struct Dir {
    int x{};
    int y{};
};

bool operator==(const Cell& a, const Cell& b){
    return a.x == b.x && a.y == b.y;
}

const ansi::Color kBackground{10, 10, 14};
const ansi::Color kBorder{60, 70, 96};
const ansi::Color kHead{130, 255, 150};
const ansi::Color kBody{40, 175, 85};
const ansi::Color kFood{240, 70, 70};

class SnakeGame {
public:
    SnakeGame(int width, int height)
        : renderer_(width, height), width_(width), height_(height), rng_(std::random_device{}()) {
        reset();
    }

    void run(){
        auto last_step = std::chrono::steady_clock::now();

        while(running_){
            handle_input();

            const auto now = std::chrono::steady_clock::now();
            if(state_ == State::Playing &&
               now - last_step >= std::chrono::milliseconds(kTickMs)){
                step();
                last_step = now;
            }

            draw();
            renderer_.present();
            print_status();

            ansi::sleep_ms(4);
        }
    }

private:
    enum class State { Playing, GameOver };

    static constexpr int kTickMs = 110;

    void reset(){
        snake_.clear();
        const int cx = width_ / 2;
        const int cy = height_ / 2;
        snake_.push_back(Cell{cx, cy});
        snake_.push_back(Cell{cx - 1, cy});
        snake_.push_back(Cell{cx - 2, cy});
        dir_ = Dir{1, 0};
        pending_ = dir_;
        score_ = 0;
        state_ = State::Playing;
        spawn_food();
    }

    bool inside(const Cell& c) const {
        return c.x > 0 && c.x < width_ - 1 && c.y > 0 && c.y < height_ - 1;
    }

    bool hits_snake(const Cell& c, std::size_t body_limit) const {
        for(std::size_t i = 0; i < body_limit; ++i){
            if(snake_[i] == c) return true;
        }
        return false;
    }

    void spawn_food(){
        std::vector<Cell> free_cells;
        free_cells.reserve(static_cast<std::size_t>(width_) * height_);

        for(int y = 1; y < height_ - 1; ++y){
            for(int x = 1; x < width_ - 1; ++x){
                const Cell candidate{x, y};
                if(!hits_snake(candidate, snake_.size())) free_cells.push_back(candidate);
            }
        }

        if(free_cells.empty()) return;
        std::uniform_int_distribution<int> dist(0, static_cast<int>(free_cells.size()) - 1);
        food_ = free_cells[static_cast<std::size_t>(dist(rng_))];
    }

    void handle_input(){
        for(int guard = 0; guard < 16; ++guard){
            const ansi::Key key = input_.poll();
            if(key == ansi::Key::None) break;

            if(key == ansi::Key::Quit || key == ansi::Key::Escape){
                running_ = false;
                return;
            }
            if(key == ansi::Key::Restart){
                reset();
                continue;
            }
            if(state_ != State::Playing) continue;

            Dir candidate = dir_;
            switch(key){
                case ansi::Key::Up: candidate = Dir{0, -1}; break;
                case ansi::Key::Down: candidate = Dir{0, 1}; break;
                case ansi::Key::Left: candidate = Dir{-1, 0}; break;
                case ansi::Key::Right: candidate = Dir{1, 0}; break;
                default: continue;
            }

            if(candidate.x + dir_.x == 0 && candidate.y + dir_.y == 0) continue;
            pending_ = candidate;
        }
    }

    void step(){
        dir_ = pending_;

        const Cell head{snake_.front().x + dir_.x, snake_.front().y + dir_.y};
        const bool grow = head == food_;

        if(!inside(head) || hits_snake(head, snake_.size() - (grow ? 0u : 1u))){
            state_ = State::GameOver;
            return;
        }

        snake_.push_front(head);
        if(grow){
            ++score_;
            spawn_food();
        } else {
            snake_.pop_back();
        }
    }

    void draw(){
        renderer_.clear(kBackground);

        for(int x = 0; x < width_; ++x){
            renderer_.set_pixel(x, 0, kBorder);
            renderer_.set_pixel(x, height_ - 1, kBorder);
        }
        for(int y = 0; y < height_; ++y){
            renderer_.set_pixel(0, y, kBorder);
            renderer_.set_pixel(width_ - 1, y, kBorder);
        }

        renderer_.set_pixel(food_.x, food_.y, kFood);

        for(std::size_t i = 0; i < snake_.size(); ++i){
            renderer_.set_pixel(snake_[i].x, snake_[i].y, i == 0 ? kHead : kBody);
        }
    }

    void print_status(){
        ansi::set_cursor(0, height_);
        std::cout << "\x1b[2K";

        if(state_ == State::Playing){
            std::cout << "Score: " << score_
                      << "    WASD / Arrows to move    [Q] quit";
        } else {
            std::cout << "GAME OVER    Score: " << score_
                      << "    [R] restart    [Q] quit";
        }
        std::cout.flush();
    }

    ansi::Renderer renderer_;
    ansi::Input input_;
    int width_;
    int height_;
    std::deque<Cell> snake_;
    Cell food_{};
    Dir dir_{1, 0};
    Dir pending_{1, 0};
    int score_ = 0;
    State state_ = State::Playing;
    bool running_ = true;
    std::mt19937 rng_;
};

} // namespace

int main(){
    ansi::CursorGuard cursor_guard;
    ansi::clear_screen();

    SnakeGame game(60, 30);
    game.run();

    ansi::set_cursor(0, 31);
    std::cout << ansi::RESET << "\n";
    return 0;
}
