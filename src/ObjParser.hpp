#pragma once

#include <vector>
#include <string>
#include "Triangle.hpp"
#include "Vector3.hpp"
#include "Octree.hpp"

class ObjParser {
    public:
        static void parse(const std::string& filename, std::vector<Vector3>& vertices, std::vector<Vector3>& faceIndexes);
        static void serialize(const std::string& filename, const Octree *tree);
};