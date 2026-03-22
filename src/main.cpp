#include <iostream>
#include <vector>
#include <algorithm>
#include "ObjParser.hpp"
#include "Triangle.hpp"
#include "AABB.hpp"
#include "Octree.hpp"

using namespace std;

int main(void) {
    vector<Vector3> vertices, faceIndexes;
    ObjParser::parse("test/teddy.obj", vertices, faceIndexes);
    Octree *octree = Octree::build(5, vertices, faceIndexes);
    ObjParser::serialize("test/result.obj", octree);
}