#ifndef VEC_H
#define VEC_H
#include <cassert>

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
    Vec()
    {
        for (float &v : data) v = 0;
    };

    explicit Vec(float x, float y, float z = 0, float w = 0)
    {
        data[0] = x;
        data[1] = y;
        if (size == D3 || size == D4) {
            data[2] = z;
        }
        if (size == D4) {
            data[3] = w;
        }
    };

    Vec operator+(Vec V) const;

    Vec operator+(float bias) const;

    Vec operator-(Vec V) const;

    Vec operator-(float bias) const;

    Vec operator*(float s) const;

    float operator*(Vec V) const;

    Vec Mul(Vec V) const;

    Vec Cross(Vec V) const;

    Vec normalize() const;

    float length() const;

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

private:
    float data[size];
};

#endif // VEC_H
