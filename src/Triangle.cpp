#include "Triangle.hpp"
#include <algorithm>

using namespace std;

Triangle::Triangle() {}

Triangle::Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2) : v0(v0), v1(v1), v2(v2) {}

AABB Triangle::getAABB() const {
    Vector3 minPoint(min({v0.x, v1.x, v2.x}), min({v0.y, v1.y, v2.y}), min({v0.z, v1.z, v2.z}));
    Vector3 maxPoint(max({v0.x, v1.x, v2.x}), max({v0.y, v1.y, v2.y}), max({v0.z, v1.z, v2.z}));
    return AABB(minPoint, maxPoint);
}