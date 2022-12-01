#include "Vec.h"
#include <cmath>

template <VectorSize size>
Vec<size> Vec<size>::operator+(Vec<size> V) const
{
    switch (size) {
        case D2: return Vec(x() + V.x(), y() + V.y());
        case D3: return Vec(x() + V.x(), y() + V.y(), z() + V.z());
        case D4: return Vec(x() + V.x(), y() + V.y(), z() + V.z(), w() + V.w());
    }
}

template <VectorSize size>
Vec<size> Vec<size>::operator+(float bias) const
{
    switch (size) {
        case D2: return Vec(x() + bias, y() + bias);
        case D3: return Vec(x() + bias, y() + bias, z() + bias);
        case D4: return Vec(x() + bias, y() + bias, z() + bias, w() + bias);
    }
}

template <VectorSize size>
Vec<size> Vec<size>::operator-(Vec<size> V) const
{
    switch (size) {
        case D2: return Vec(x() - V.x(), y() - V.y());
        case D3: return Vec(x() - V.x(), y() - V.y(), z() - V.z());
        case D4: return Vec(x() - V.x(), y() - V.y(), z() - V.z(), w() - V.w());
    }
}

template <VectorSize size>
Vec<size> Vec<size>::operator-(float bias) const
{
    switch (size) {
        case D2: return Vec(x() - bias, y() - bias);
        case D3: return Vec(x() - bias, y() - bias, z() - bias);
        case D4: return Vec(x() - bias, y() - bias, z() - bias, w() - bias);
    }
}

template <VectorSize size>
Vec<size> Vec<size>::operator*(float s) const
{
    switch (size) {
        case D2: return Vec(x() * s, y() * s);
        case D3: return Vec(x() * s, y() * s, z() * s);
        case D4: return Vec(x() * s, y() * s, z() * s, w() * s);
    }
}

template <VectorSize size>
float Vec<size>::operator*(Vec<size> V) const
{
    switch (size) {
        case D2: return x() * V.x() + y() * V.y();
        case D3: return x() * V.x() + y() * V.y() + z() * V.z();
        case D4: return x() * V.x() + y() * V.y() + z() * V.z() + w() * V.w();
    }
}

template <VectorSize size>
Vec<size> Vec<size>::Mul(Vec<size> V) const
{
    switch (size) {
        case D2: return Vec(x() * V.x(), y() * V.y());
        case D3: return Vec(x() * V.x(), y() * V.y(), z() * V.z());
        case D4: return Vec(x() * V.x(), y() * V.y(), z() * V.z(), w() * V.w());
    }
}

template <VectorSize size>
Vec<size> Vec<size>::Cross(Vec<size> V) const
{
    float a1 = x();
    float b1 = y();
    float c1 = z();
    float a2 = V.x();
    float b2 = V.y();
    float c2 = V.z();

    return Vec(b1 * c2 - b2 * c1, c1 * a2 - a1 * c2, a1 * b2 - a2 * b1, 0);
}

template <VectorSize size>
Vec<size> Vec<size>::normalize() const
{
    float L = length();
    if (L == 0.f) {
        return Vec(0, 1, 0, 0);
    }
    return this->operator*( 1.0 / L);
}

template <VectorSize size>
float Vec<size>::length() const
{
    float sum = 0;
    for (float v : data) {
        sum += v * v;
    }
    return sqrt(sum);
}
