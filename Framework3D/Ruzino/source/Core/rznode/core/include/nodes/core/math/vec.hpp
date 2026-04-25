#pragma once

#include <array>
#include <cmath>

namespace Ruzino {

template<typename T, size_t N>
class Vec {
public:
    std::array<T, N> data;

    Vec() { data.fill(T{}); }
    
    template<typename... Args>
    Vec(Args... args) : data{static_cast<T>(args)...} {
        static_assert(sizeof...(args) == N, "Wrong number of arguments");
    }

    T& operator[](size_t i) { return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }

    Vec operator+(const Vec& other) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] + other[i];
        }
        return result;
    }

    Vec operator-(const Vec& other) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] - other[i];
        }
        return result;
    }

    Vec operator*(T scalar) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) {
            result[i] = data[i] * scalar;
        }
        return result;
    }

    T dot(const Vec& other) const {
        T result = T{};
        for (size_t i = 0; i < N; ++i) {
            result += data[i] * other[i];
        }
        return result;
    }

    T length() const {
        return std::sqrt(dot(*this));
    }

    Vec normalized() const {
        T len = length();
        if (len > T{}) {
            return *this * (T{1} / len);
        }
        return *this;
    }
};

using Vec2f = Vec<float, 2>;
using Vec3f = Vec<float, 3>;
using Vec4f = Vec<float, 4>;
using Vec2d = Vec<double, 2>;
using Vec3d = Vec<double, 3>;
using Vec4d = Vec<double, 4>;

} // namespace Ruzino