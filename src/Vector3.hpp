#pragma once

#include <iostream>
#include <string>

struct Vector3 {
    float x, y, z;
    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float num) : x(num), y(num), z(num) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vector3 operator+(const Vector3& other) const { return Vector3(this->x + other.x, this->y + other.y, this->z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(this->x - other.x, this->y - other.y, this->z - other.z); }
    float operator*(const Vector3& other) const {
        return this->x * other.x + this->y * other.y + this->z * other.z;
    }
    Vector3 operator*(const float other) const {
        return Vector3(this->x * other, this->y * other, this->z * other);
    }
    Vector3 operator^(const Vector3 other) const {
        return Vector3(
            this->y * other.z - this->z * other.y,
            this->z * other.x - this->x * other.z,
            this->x * other.y - this->y * other.x
        );
    }
};
