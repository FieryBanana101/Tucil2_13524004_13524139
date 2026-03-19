#pragma once

#include "Vector3.hpp"

struct AABB {
    Vector3 min, max;
    
    AABB() : min(Vector3()), max(Vector3()) {}
    AABB(const Vector3& min, const Vector3& max) : min(min), max(max) {}

    Vector3 getCenter() const {
        return Vector3((min.x + max.x) / 2.0f, (min.y + max.y) / 2.0f, (min.z + max.z) / 2.0f);
    }
};