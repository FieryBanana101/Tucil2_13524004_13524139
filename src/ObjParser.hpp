#pragma once

#include <vector>
#include <string>
#include "Triangle.hpp"
#include "Vector3.hpp"
#include "Octree.hpp"

class ObjParser {
    public:
        static void parse(
            const std::string& filepath, 
            const bool showDuration, 
            std::vector<Vector3>& vertices, 
            std::vector<Vector3>& faceIndexes
        );

        static void serialize(
            Octree *octree, 
            const std::string& filepath, 
            bool showDuration
        );

        static void serializeSpaceOptimized(
            Octree *octree, 
            const std::string& filepath, 
            bool showDuration
        );
};