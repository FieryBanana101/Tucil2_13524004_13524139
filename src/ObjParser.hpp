#pragma once

#include <vector>
#include <string>
#include "Triangle.hpp"
#include "Vector3.hpp"

class ObjParser {
    public:
        static vector<Triangle> parse(const string& filename);
};