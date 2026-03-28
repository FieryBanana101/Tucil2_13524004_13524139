#pragma once

#include <iostream>
#include <string>
#include <cmath> // std::abs()
#include <limits>

struct Vector3 {
    static constexpr float EPS = std::numeric_limits<float>::epsilon();
    float x, y, z;

    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float num) : x(num), y(num), z(num) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const { return Vector3(this->x + other.x, this->y + other.y, this->z + other.z); }
    
    Vector3 operator-(const Vector3& other) const { return Vector3(this->x - other.x, this->y - other.y, this->z - other.z); }
    
    float operator*(const Vector3& other) const { // Scalar multiplication
        return this->x * other.x + this->y * other.y + this->z * other.z;
    }

    Vector3 operator*(const float other) const { // Dot product
        return Vector3(this->x * other, this->y * other, this->z * other);
    }

    Vector3 operator^(const Vector3& other) const { // Cross product
        return Vector3(
            this->y * other.z - this->z * other.y,
            this->z * other.x - this->x * other.z,
            this->x * other.y - this->y * other.x
        );
    }

    bool operator==(const Vector3& other) const {
        return std::fabs(this->x - other.x) < EPS && 
            std::fabs(this->y - other.y) < EPS &&
            std::fabs(this->z - other.z) < EPS;
    }

    bool operator<(const Vector3& other) const {
        if(std::fabs(this->x - other.x) < EPS){
            if(std::fabs(this->y - other.y) < EPS){
                if(std::fabs(this->z - other.z) < EPS) return false;
                return this->z < other.z;
            }
            return this->y < other.y;
        }
        return this->x < other.x;
    }

};
