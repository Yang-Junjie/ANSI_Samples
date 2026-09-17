// by Artichoke
#pragma once
#include <cmath>

namespace ansi {

struct Vec2 {
    double x{};
    double y{};

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(double scalar) const { return {x * scalar, y * scalar}; }
};

struct Vec3 {
    double x{};
    double y{};
    double z{};

    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(double scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
};

inline double dot(const Vec3& a, const Vec3& b){
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b){
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline double length(const Vec3& v){
    return std::sqrt(dot(v, v));
}

inline Vec3 normalize(const Vec3& v){
    const double len = length(v);
    return len > 0.0 ? v * (1.0 / len) : v;
}

inline Vec2 rotate(const Vec2& v, double angle){
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

inline Vec3 rotate_x(const Vec3& v, double angle){
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

inline Vec3 rotate_y(const Vec3& v, double angle){
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

inline Vec3 rotate_z(const Vec3& v, double angle){
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    return {v.x * c - v.y * s, v.x * s + v.y * c, v.z};
}

inline double edge(const Vec2& a, const Vec2& b, const Vec2& p){
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

} // namespace ansi
