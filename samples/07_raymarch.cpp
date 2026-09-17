// by Deepseek v4.1 flash
#include <ansi.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace {

constexpr int kMaxSteps = 96;
constexpr int kShadowSteps = 28;
constexpr int kAoSamples = 5;
constexpr double kMaxDistance = 24.0;
constexpr double kSurfaceEpsilon = 0.001;
constexpr int kReflectionDepth = 1;

double g_time = 0.0;
double g_sphere_radius = 0.87;
double g_morph = 0.0;
double g_rot_cos = 1.0;
double g_rot_sin = 0.0;

ansi::Vec3 g_light_dir = ansi::normalize(ansi::Vec3{0.6, 0.75, -0.45});

using ansi::Vec3;

double sd_sphere(const Vec3& p, double radius){
    return ansi::length(p) - radius;
}

double sd_box(const Vec3& p, const Vec3& half){
    const Vec3 d{std::abs(p.x) - half.x, std::abs(p.y) - half.y, std::abs(p.z) - half.z};
    const Vec3 outside{std::max(d.x, 0.0), std::max(d.y, 0.0), std::max(d.z, 0.0)};
    const double inside = std::min(std::max(d.x, std::max(d.y, d.z)), 0.0);
    return std::sqrt(outside.x * outside.x + outside.y * outside.y + outside.z * outside.z) + inside;
}

double map_blob(const Vec3& p){
    const Vec3 q{g_rot_cos * p.x + g_rot_sin * p.z, p.y, -g_rot_sin * p.x + g_rot_cos * p.z};

    const double cube = sd_box(q, Vec3{0.7, 0.7, 0.7});
    const double sphere = sd_sphere(p, g_sphere_radius);

    return cube * (1.0 - g_morph) + sphere * g_morph;
}

double map_scene(const Vec3& p){
    return std::min(map_blob(p), p.y + 1.0);
}

Vec3 calc_normal(const Vec3& p){
    const double e = 0.0015;
    const double dx = map_scene(Vec3{p.x + e, p.y, p.z}) - map_scene(Vec3{p.x - e, p.y, p.z});
    const double dy = map_scene(Vec3{p.x, p.y + e, p.z}) - map_scene(Vec3{p.x, p.y - e, p.z});
    const double dz = map_scene(Vec3{p.x, p.y, p.z + e}) - map_scene(Vec3{p.x, p.y, p.z - e});
    return ansi::normalize(Vec3{dx, dy, dz});
}

bool march(const Vec3& origin, const Vec3& dir, double& out_t){
    double t = 0.05;
    for(int i = 0; i < kMaxSteps && t < kMaxDistance; ++i){
        const double d = map_scene(origin + dir * t);
        if(d < kSurfaceEpsilon){
            out_t = t;
            return true;
        }
        t += d;
    }
    return false;
}

double soft_shadow(const Vec3& origin, const Vec3& dir){
    double result = 1.0;
    double t = 0.03;
    for(int i = 0; i < kShadowSteps && t < 8.0; ++i){
        const double h = map_scene(origin + dir * t);
        if(h < 0.0005) return 0.0;
        result = std::min(result, 9.0 * h / t);
        t += std::clamp(h, 0.03, 0.35);
    }
    return std::clamp(result, 0.0, 1.0);
}

double calc_ao(const Vec3& p, const Vec3& n){
    double occlusion = 0.0;
    double weight = 1.0;
    for(int i = 0; i < kAoSamples; ++i){
        const double h = 0.03 + 0.14 * i;
        const double d = map_scene(p + n * h);
        occlusion += (h - d) * weight;
        weight *= 0.7;
    }
    return std::clamp(1.0 - 1.8 * occlusion, 0.0, 1.0);
}

Vec3 sky_color(const Vec3& dir){
    const double t = std::clamp(0.5 * (dir.y + 1.0), 0.0, 1.0);
    const Vec3 horizon{0.12, 0.14, 0.20};
    const Vec3 zenith{0.02, 0.03, 0.07};
    Vec3 color = horizon * (1.0 - t) + zenith * t;

    const double sun = std::pow(std::max(0.0, ansi::dot(dir, g_light_dir)), 48.0);
    color += Vec3{1.0, 0.86, 0.62} * (sun * 0.8);
    return color;
}

Vec3 shade_surface(const Vec3& p, const Vec3& n, const Vec3& rd,
                   const Vec3& albedo, double specular_power, double reflectivity){
    const double n_dot_l = std::max(0.0, ansi::dot(n, g_light_dir));
    const double shadow = n_dot_l > 0.0 ? soft_shadow(p + n * 0.003, g_light_dir) : 0.0;
    const double ao = calc_ao(p, n);

    const Vec3 view = rd * -1.0;
    const Vec3 halfway = ansi::normalize(g_light_dir + view);
    const double spec = std::pow(std::max(0.0, ansi::dot(n, halfway)), specular_power) * shadow;
    const double fresnel = std::pow(1.0 - std::max(0.0, ansi::dot(n, view)), 5.0);

    Vec3 color = albedo * ((0.06 + n_dot_l * shadow * 0.94) * ao);
    color += Vec3{1.0, 0.95, 0.88} * (spec * 0.8);
    color += Vec3{0.35, 0.5, 0.8} * (fresnel * reflectivity * 0.4);
    return color;
}

Vec3 render(const Vec3& origin, const Vec3& dir, int depth){
    double t = 0.0;
    if(!march(origin, dir, t)) return sky_color(dir);

    const Vec3 p = origin + dir * t;
    const Vec3 n = calc_normal(p);

    const bool is_floor = (p.y + 1.0) < map_blob(p);

    Vec3 albedo;
    double specular_power = 90.0;
    double reflectivity = 0.2;

    if(is_floor){
        const int cx = static_cast<int>(std::floor(p.x));
        const int cz = static_cast<int>(std::floor(p.z));
        const bool dark = ((cx + cz) & 1) != 0;
        albedo = dark ? Vec3{0.05, 0.05, 0.07} : Vec3{0.78, 0.79, 0.82};
        specular_power = 70.0;
        reflectivity = dark ? 0.45 : 0.10;
    } else {
        albedo = Vec3{0.88, 0.86, 0.82};
        specular_power = 16.0;
        reflectivity = 0.0;
    }

    Vec3 color = shade_surface(p, n, dir, albedo, specular_power, reflectivity);

    if(depth > 0 && reflectivity > 0.0){
        const Vec3 reflected = dir - n * (2.0 * ansi::dot(dir, n));
        const Vec3 reflected_color = render(p + n * 0.004, reflected, depth - 1);

        const Vec3 view = dir * -1.0;
        const double fresnel = std::pow(1.0 - std::max(0.0, ansi::dot(n, view)), 5.0);
        const double factor = std::clamp(reflectivity + (1.0 - reflectivity) * fresnel, 0.0, 1.0);
        color = color * (1.0 - factor) + reflected_color * factor;
    }

    return color;
}

ansi::Color to_color(const Vec3& linear){
    const auto channel = [](double value){
        value = std::max(0.0, value) * 1.3;
        value = value / (1.0 + value);
        value = std::pow(value, 1.0 / 2.2);
        return std::clamp(static_cast<int>(std::lround(value * 255.0)), 0, 255);
    };
    return {channel(linear.x), channel(linear.y), channel(linear.z)};
}

void camera_basis(const Vec3& forward, Vec3& right, Vec3& up){
    right = ansi::normalize(ansi::cross(forward, Vec3{0.0, 1.0, 0.0}));
    up = ansi::cross(right, forward);
}

} // namespace

int main(){
    const int width = 160;
    const int height = 80;
    const double aspect = width / static_cast<double>(height);
    const double fov_scale = std::tan(0.5 * 55.0 * 3.14159265358979323846 / 180.0);

    ansi::Renderer renderer(width, height);
    ansi::CursorGuard cursor_guard;

    const Vec3 target{0.0, -0.15, 0.0};

    while(true){
        g_time += 0.07;
        g_morph = 0.5 - 0.5 * std::cos(g_time * 1.0);
        g_rot_cos = std::cos(g_time * 0.9);
        g_rot_sin = std::sin(g_time * 0.9);

        const double orbit = g_time * 0.4;
        const Vec3 origin{3.4 * std::sin(orbit), 1.15, 3.4 * std::cos(orbit)};

        const Vec3 forward = ansi::normalize(target - origin);
        Vec3 right{};
        Vec3 up{};
        camera_basis(forward, right, up);

        for(int y = 0; y < height; ++y){
            const double py = 1.0 - 2.0 * (y + 0.5) / height;
            for(int x = 0; x < width; ++x){
                const double px = (2.0 * (x + 0.5) / width - 1.0) * aspect;

                const Vec3 dir = ansi::normalize(
                    forward + right * (px * fov_scale) + up * (py * fov_scale));

                renderer.set_pixel(x, y, to_color(render(origin, dir, kReflectionDepth)));
            }
        }

        renderer.present();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
