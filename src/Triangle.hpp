#pragma once

#include "Vector3.hpp"

struct Triangle {
    Vector3 v0, v1, v2;

    Triangle();
    Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2);
};