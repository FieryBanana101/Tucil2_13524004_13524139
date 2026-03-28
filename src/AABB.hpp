#pragma once

#include "Vector3.hpp"
#include "Triangle.hpp"

struct AABB {
    Vector3 min, max;
    
    AABB() : min(Vector3()), max(Vector3()) {}
    AABB(const Vector3& min, const Vector3& max) : min(min), max(max) {}

    Vector3 getCenter() const;
    float getVolume() const;
    bool intersect(Triangle &triangle) const;
    static void _getMinMax(float a, float b, float c, float &min, float &max);
};