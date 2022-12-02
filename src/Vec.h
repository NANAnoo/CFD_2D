#ifndef VEC_H
#define VEC_H

#include <cassert>
#include <cmath>
#include <ostream>

#define vec2 Vec<D2>
#define vec3 Vec<D3>
#define vec4 Vec<D4>

enum VectorSize {
    D2 = 2,
    D3 = 3,
    D4 = 4
};

/* vector size */
template<VectorSize size>
class Vec {
public:
    Vec() {
        for (float &v: data) v = 0;
    };

    explicit Vec(float x, float y, float z = 0, float w = 0) {
        data[0] = x;
        data[1] = y;
        if (size == D3 || size == D4) {
            data[2] = z;
        }
        if (size == D4) {
            data[3] = w;
        }
    };

    Vec &operator=(const Vec &other) noexcept {
        if (&other == this) return *this;
        unsigned int i = 0;
        for (float v: other.data)
            data[i++] = v;
        return *this;
    }

    Vec operator+(Vec &V) {
        if constexpr (size == D2) {
            return Vec(x() + V.x(), y() + V.y());
        }
        if constexpr (size == D3) {
            return Vec(x() + V.x(), y() + V.y(), z() + V.z());
        }
        if constexpr (size == D4) {
            return Vec(x() + V.x(), y() + V.y(), z() + V.z(), w() + V.w());
        }
    }

    Vec operator+(float bias) {
        if constexpr (size == D2) {
            return Vec(x() + bias, y() + bias);
        }
        if constexpr (size == D3) {
            return Vec(x() + bias, y() + bias, z() + bias);
        }
        if constexpr (size == D4) {
            return Vec(x() + bias, y() + bias, z() + bias, w() + bias);
        }
    }

    Vec operator-(Vec &V) {
        if constexpr (size == D2) {
            return Vec(x() - V.x(), y() - V.y());
        }
        if constexpr (size == D3) {
            return Vec(x() - V.x(), y() - V.y(), z() - V.z());
        }
        if constexpr (size == D4) {
            return Vec(x() - V.x(), y() - V.y(), z() - V.z(), w() - V.w());
        }
    }

    Vec operator-(float bias) {
        return operator+(-bias);
    }


    Vec operator*(float s) {
        if constexpr (size == D2) {
            return Vec(x() * s, y() * s);
        }
        if constexpr (size == D3) {
            return Vec(x() * s, y() * s, z() * s);
        }
        if constexpr (size == D4) {
            return Vec(x() * s, y() * s, z() * s, w() * s);
        }
    }

    Vec operator/(float s) {
        return operator*(1.f / s);
    }

    Vec operator*(Vec &V) {
        if constexpr (size == D2) {
            return Vec(x() * V.x(), y() * V.y());
        }
        if constexpr (size == D3) {
            return Vec(x() * V.x(), y() * V.y(), z() * V.z());
        }
        if constexpr (size == D4) {
            return Vec(x() * V.x(), y() * V.y(), z() * V.z(), w() * V.w());
        }
    }

    float Mul(Vec &V) {
        float res = 0;
        for (unsigned int i = 0; i < size; i++) {
            res += data[i] * V.data[i];
        }
        return res;
    }

    Vec Cross(Vec &V) {
        float a1 = x();
        float b1 = y();
        float c1 = 0;
        if constexpr (size != D2) c1 = z();
        float a2 = V.x();
        float b2 = V.y();
        float c2 = 0;
        if constexpr (size != D2) c2 = V.z();
        return Vec(b1 * c2 - b2 * c1, c1 * a2 - a1 * c2, a1 * b2 - a2 * b1, 0);
    }

    Vec normalize() {
        float L = length();
        if (L == 0.f) {
            return Vec(0, 1, 0, 0);
        }
        return this->operator/(L);
    }

    float length() {
        float sum = 0;
        for (float v: data) {
            sum += v * v;
        }
        return sqrt(sum);
    }

    float &x() {
        return data[0];
    }

    float &y() {
        return data[1];
    }

    float &z() {
        static_assert(size != D2, "can't access z of vec2");
        return data[2];
    }

    float &w() {
        static_assert(size == D4, "can't access 3 of vec2,3");
        return data[3];
    }

    friend std::ostream &operator<<(std::ostream &os, Vec &v) {
        if constexpr (size == D2) {
            os << "Vec2[" << v.x() << ", " << v.y() << "]";
        }
        if constexpr (size == D3) {
            os << "Vec2[" << v.x() << ", " << v.y() << ", " << v.z() << "]";
        }
        if constexpr (size == D4) {
            os << "Vec2[" << v.x() << ", " << v.y() << ", " << v.z() << ", " << v.w() << "]";
        }
        return os;
    }

private:
    float data[size];
};

#endif // VEC_H
