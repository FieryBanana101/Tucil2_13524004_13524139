#include "Triangle.hpp"
#include <algorithm>

using namespace std;

Triangle::Triangle() {}

Triangle::Triangle(const Vector3& v0, const Vector3& v1, const Vector3& v2) : v0(v0), v1(v1), v2(v2) {}