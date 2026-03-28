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

    static inline bool axisTest(
        const Vector3& axis,
        const Vector3& v0,
        const Vector3& v1,
        const Vector3& v2,
        const Vector3& boxHalfSize
    );
};