// by Deepseek v4.1 flash
#include <ansi.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

namespace {

constexpr int kPressureIterations = 32;
constexpr double kTimeStep = 1.0;
constexpr double kDamping = 1.0;
constexpr double kMaxSpeed = 16.0;
constexpr double kSpeedScale = 10.0;
constexpr double kConfinement = 0.12;
constexpr double kVorticityScale = 3.0;

ansi::Color heatmap(double value){
    struct Stop {
        double position;
        ansi::Color color;
    };
    static const Stop stops[] = {
        {0.00, {  8,  10,  30}},
        {0.15, { 30,  70, 200}},
        {0.35, {  0, 190, 200}},
        {0.55, { 60, 220,  90}},
        {0.75, {250, 220,  40}},
        {0.90, {250, 120,  20}},
        {1.00, {255,  50,  40}},
    };
    constexpr int count = sizeof(stops) / sizeof(stops[0]);

    const double t = std::clamp(value, 0.0, 1.0);
    for(int i = 0; i < count - 1; ++i){
        if(t <= stops[i + 1].position){
            const double span = stops[i + 1].position - stops[i].position;
            const double f = span > 0.0 ? (t - stops[i].position) / span : 0.0;
            const auto mix = [f](int a, int b){
                return static_cast<int>(std::lround(a + (b - a) * f));
            };
            return {
                mix(stops[i].color.r, stops[i + 1].color.r),
                mix(stops[i].color.g, stops[i + 1].color.g),
                mix(stops[i].color.b, stops[i + 1].color.b),
            };
        }
    }
    return stops[count - 1].color;
}

ansi::Color diverging(double value){
    const double t = std::clamp(value, -1.0, 1.0);
    if(t >= 0.0){
        const int c = static_cast<int>(std::lround(255.0 * (1.0 - t)));
        return {255, c, c};
    }
    const int c = static_cast<int>(std::lround(255.0 * (1.0 + t)));
    return {c, c, 255};
}

class Fluid {
public:
    Fluid(int width, int height)
        : width_(width), height_(height),
          u_(cells(), 0.0), v_(cells(), 0.0),
          prev_u_(cells(), 0.0), prev_v_(cells(), 0.0),
          pressure_(cells(), 0.0), divergence_(cells(), 0.0),
          vorticity_(cells(), 0.0),
          solid_(cells(), 0), disc_(cells(), 0),
          inlet_x_(3), inlet_half_height_(3) {
        build_scene();
    }

    void step(){
        apply_inlet();

        advect();
        project();

        apply_vorticity_confinement();
        project();

        apply_inlet();
        clear_solid_velocity();

        for(std::size_t i = 0; i < u_.size(); ++i){
            u_[i] = std::clamp(u_[i] * kDamping, -kMaxSpeed, kMaxSpeed);
            v_[i] = std::clamp(v_[i] * kDamping, -kMaxSpeed, kMaxSpeed);
        }
    }

    double speed_at(int x, int y) const {
        const int i = index(x, y);
        return std::sqrt(u_[i] * u_[i] + v_[i] * v_[i]);
    }

    double vorticity_at(int x, int y) const { return vorticity_[index(x, y)]; }

    bool is_solid(int x, int y) const { return solid_[index(x, y)] != 0; }
    bool is_disc(int x, int y) const { return disc_[index(x, y)] != 0; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    std::size_t cells() const { return static_cast<std::size_t>(width_) * height_; }
    int index(int x, int y) const { return y * width_ + x; }

    void build_scene(){
        for(int x = 0; x < width_; ++x){
            solid_[index(x, 0)] = 1;
            solid_[index(x, height_ - 1)] = 1;
        }
        for(int y = 0; y < height_; ++y){
            solid_[index(0, y)] = 1;
        }

        const double cx = width_ * 0.28;
        const double cy = height_ * 0.5 + 1.5;
        const double radius = std::min(width_, height_) * 0.09;

        for(int y = 1; y < height_ - 1; ++y){
            for(int x = 1; x < width_ - 1; ++x){
                const double dx = x - cx;
                const double dy = y - cy;
                if(dx * dx + dy * dy <= radius * radius){
                    solid_[index(x, y)] = 1;
                    disc_[index(x, y)] = 1;
                }
            }
        }
    }

    void apply_inlet(){
        const double speed = 10.0;
        const int cy = height_ / 2;

        for(int y = cy - inlet_half_height_; y <= cy + inlet_half_height_; ++y){
            if(y < 1 || y >= height_ - 1) continue;
            for(int x = 1; x <= inlet_x_; ++x){
                const int i = index(x, y);
                if(solid_[i]) continue;
                u_[i] = speed;
                v_[i] = 0.0;
            }
        }
    }

    void clear_solid_velocity(){
        for(std::size_t i = 0; i < u_.size(); ++i){
            if(solid_[i]){
                u_[i] = 0.0;
                v_[i] = 0.0;
            }
        }
    }

    bool solid_at(double x, double y) const {
        const int xi = std::clamp(static_cast<int>(std::lround(x)), 0, width_ - 1);
        const int yi = std::clamp(static_cast<int>(std::lround(y)), 0, height_ - 1);
        return solid_[index(xi, yi)] != 0;
    }

    void apply_vorticity_confinement(){
        for(int y = 1; y < height_ - 1; ++y){
            for(int x = 1; x < width_ - 1; ++x){
                const int i = index(x, y);
                if(solid_[i]){
                    vorticity_[i] = 0.0;
                    continue;
                }
                const double dv_dx = 0.5 * (v_[index(x + 1, y)] - v_[index(x - 1, y)]);
                const double du_dy = 0.5 * (u_[index(x, y + 1)] - u_[index(x, y - 1)]);
                vorticity_[i] = dv_dx - du_dy;
            }
        }

        for(int y = 1; y < height_ - 1; ++y){
            for(int x = 1; x < width_ - 1; ++x){
                const int i = index(x, y);
                if(solid_[i]) continue;

                const double gx = 0.5 * (std::abs(vorticity_[index(x + 1, y)])
                                       - std::abs(vorticity_[index(x - 1, y)]));
                const double gy = 0.5 * (std::abs(vorticity_[index(x, y + 1)])
                                       - std::abs(vorticity_[index(x, y - 1)]));
                const double length = std::sqrt(gx * gx + gy * gy) + 1e-6;
                const double nx = gx / length;
                const double ny = gy / length;
                const double w = vorticity_[i];

                u_[i] += kConfinement * (ny * w) * kTimeStep;
                v_[i] += kConfinement * (-nx * w) * kTimeStep;
            }
        }
    }

    double sample(const std::vector<double>& field, double x, double y) const {
        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        x0 = std::clamp(x0, 0, width_ - 2);
        y0 = std::clamp(y0, 0, height_ - 2);

        const double sx = x - x0;
        const double sy = y - y0;
        const int xs[2] = {x0, x0 + 1};
        const int ys[2] = {y0, y0 + 1};
        const double wx[2] = {1.0 - sx, sx};
        const double wy[2] = {1.0 - sy, sy};

        double accumulator = 0.0;
        double weight = 0.0;
        for(int j = 0; j < 2; ++j){
            for(int i = 0; i < 2; ++i){
                const int cell = index(xs[i], ys[j]);
                if(solid_[cell]) continue;
                const double w = wx[i] * wy[j];
                accumulator += field[cell] * w;
                weight += w;
            }
        }
        return weight > 0.0 ? accumulator / weight : 0.0;
    }

    std::pair<double, double> sample_extent(const std::vector<double>& field, double x, double y) const {
        int x0 = static_cast<int>(std::floor(x));
        int y0 = static_cast<int>(std::floor(y));
        x0 = std::clamp(x0, 0, width_ - 2);
        y0 = std::clamp(y0, 0, height_ - 2);

        double low = 0.0;
        double high = 0.0;
        bool found = false;
        for(int j = 0; j < 2; ++j){
            for(int i = 0; i < 2; ++i){
                const int cell = index(x0 + i, y0 + j);
                if(solid_[cell]) continue;
                const double value = field[cell];
                if(!found){
                    low = value;
                    high = value;
                    found = true;
                } else {
                    low = std::min(low, value);
                    high = std::max(high, value);
                }
            }
        }
        return found ? std::pair<double, double>{low, high}
                     : std::pair<double, double>{0.0, 0.0};
    }

    double advect_scalar(const std::vector<double>& field,
                         const std::vector<double>& vel_x,
                         const std::vector<double>& vel_y,
                         int x, int y, double current) const {
        const double back_x = x - kTimeStep * vel_x[index(x, y)];
        const double back_y = y - kTimeStep * vel_y[index(x, y)];

        if(solid_at(back_x, back_y)) return current;

        const double forward_x = back_x + kTimeStep * sample(vel_x, back_x, back_y);
        const double forward_y = back_y + kTimeStep * sample(vel_y, back_x, back_y);

        const double value_back = sample(field, back_x, back_y);
        if(solid_at(forward_x, forward_y)) return value_back;

        const double value_forward = sample(field, forward_x, forward_y);

        const double result = value_back + 0.5 * (current - value_forward);
        const auto [low, high] = sample_extent(field, back_x, back_y);
        return std::clamp(result, low, high);
    }

    void advect(){
        prev_u_ = u_;
        prev_v_ = v_;

        std::vector<double> next_u(u_.size(), 0.0);
        std::vector<double> next_v(v_.size(), 0.0);

        for(int y = 1; y < height_ - 1; ++y){
            for(int x = 1; x < width_ - 1; ++x){
                const int i = index(x, y);
                if(solid_[i]) continue;

                next_u[i] = advect_scalar(prev_u_, prev_u_, prev_v_, x, y, u_[i]);
                next_v[i] = advect_scalar(prev_v_, prev_u_, prev_v_, x, y, v_[i]);
            }
        }

        u_.swap(next_u);
        v_.swap(next_v);
    }

    void project(){
        for(int y = 1; y < height_ - 1; ++y){
            for(int x = 1; x < width_ - 1; ++x){
                const int i = index(x, y);
                if(solid_[i]){
                    divergence_[i] = 0.0;
                    pressure_[i] = 0.0;
                    continue;
                }

                const double right = solid_[index(x + 1, y)] ? 0.0 : u_[index(x + 1, y)];
                const double left = solid_[index(x - 1, y)] ? 0.0 : u_[index(x - 1, y)];
                const double up = solid_[index(x, y + 1)] ? 0.0 : v_[index(x, y + 1)];
                const double down = solid_[index(x, y - 1)] ? 0.0 : v_[index(x, y - 1)];

                divergence_[i] = -0.5 * (right - left + up - down);
                pressure_[i] = 0.0;
            }
        }

        for(int y = 1; y < height_ - 1; ++y){
            const int i = index(width_ - 1, y);
            divergence_[i] = 0.0;
            pressure_[i] = 0.0;
        }

        for(int iteration = 0; iteration < kPressureIterations; ++iteration){
            for(int y = 1; y < height_ - 1; ++y){
                for(int x = 1; x < width_ - 1; ++x){
                    const int i = index(x, y);
                    if(solid_[i]) continue;

                    double sum = divergence_[i];
                    int count = 0;
                    if(!solid_[index(x - 1, y)]){ sum += pressure_[index(x - 1, y)]; ++count; }
                    if(!solid_[index(x + 1, y)]){ sum += pressure_[index(x + 1, y)]; ++count; }
                    if(!solid_[index(x, y - 1)]){ sum += pressure_[index(x, y - 1)]; ++count; }
                    if(!solid_[index(x, y + 1)]){ sum += pressure_[index(x, y + 1)]; ++count; }

                    pressure_[i] = sum / static_cast<double>(std::max(1, count));
                }
            }
        }

        for(int y = 1; y < height_ - 1; ++y){
            for(int x = 1; x < width_ - 1; ++x){
                const int i = index(x, y);
                if(solid_[i]) continue;

                const double p_left = solid_[index(x - 1, y)] ? pressure_[i] : pressure_[index(x - 1, y)];
                const double p_right = solid_[index(x + 1, y)] ? pressure_[i] : pressure_[index(x + 1, y)];
                const double p_down = solid_[index(x, y - 1)] ? pressure_[i] : pressure_[index(x, y - 1)];
                const double p_up = solid_[index(x, y + 1)] ? pressure_[i] : pressure_[index(x, y + 1)];

                u_[i] -= 0.5 * (p_right - p_left);
                v_[i] -= 0.5 * (p_up - p_down);
            }
        }

        for(int y = 1; y < height_ - 1; ++y){
            const int right_in = index(width_ - 2, y);
            const int right_out = index(width_ - 1, y);
            u_[right_out] = u_[right_in];
            v_[right_out] = v_[right_in];
        }
    }

    int width_;
    int height_;
    std::vector<double> u_;
    std::vector<double> v_;
    std::vector<double> prev_u_;
    std::vector<double> prev_v_;
    std::vector<double> pressure_;
    std::vector<double> divergence_;
    std::vector<double> vorticity_;
    std::vector<unsigned char> solid_;
    std::vector<unsigned char> disc_;
    int inlet_x_;
    int inlet_half_height_;
};

} // namespace

int main(){
    const int width = 120;
    const int height = 60;

    ansi::Renderer renderer(width, height);
    ansi::CursorGuard cursor_guard;
    ansi::Input input;

    Fluid fluid(width, height);
    const ansi::Color wall_color{40, 42, 52};
    const ansi::Color disc_color{205, 205, 212};

    bool show_vorticity = false;

    while(true){
        for(;;){
            const ansi::Key key = input.poll();
            if(key == ansi::Key::None) break;
            if(key == ansi::Key::Quit || key == ansi::Key::Escape) return 0;
            if(key == ansi::Key::Space) show_vorticity = !show_vorticity;
        }

        fluid.step();

        for(int y = 0; y < height; ++y){
            for(int x = 0; x < width; ++x){
                ansi::Color color;
                if(fluid.is_disc(x, y)){
                    color = disc_color;
                } else if(fluid.is_solid(x, y)){
                    color = wall_color;
                } else if(show_vorticity){
                    color = diverging(fluid.vorticity_at(x, y) / kVorticityScale);
                } else {
                    color = heatmap(fluid.speed_at(x, y) / kSpeedScale);
                }
                renderer.set_pixel(x, y, color);
            }
        }

        renderer.present();

        ansi::set_cursor(0, height);
        std::cout << "\x1b[2K"
                  << (show_vorticity ? "View: vorticity (blue=- / red=+)" : "View: speed")
                  << "    [Space] switch    [Q] quit";
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
