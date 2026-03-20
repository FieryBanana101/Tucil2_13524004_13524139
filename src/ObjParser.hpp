#pragma once

#include <vector>
#include <string>
#include "Triangle.hpp"
#include "Vector3.hpp"

class ObjParser {
    public:
        static std::vector<Triangle> parse(const std::string& filename);
};