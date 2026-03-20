#include <iostream>
#include <vector>
#include <algorithm>
#include "ObjParser.hpp"
#include "Triangle.hpp"
#include "AABB.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    // string filename = "../test/pumpkin.obj";
    // try {
    //     vector<Triangle> triangles = ObjParser::parse(filename);
    //     cout << "Successfully parsed " << filename << " with " << triangles.size() << " triangles." << endl;
    //     AABB aabb = triangles[0].getAABB();
    //     for(size_t i = 1; i < triangles.size(); ++i) {
    //         AABB currentAABB = triangles[i].getAABB();
    //         aabb.min.x = min(aabb.min.x, currentAABB.min.x);
    //         aabb.min.y = min(aabb.min.y, currentAABB.min.y);
    //         aabb.min.z = min(aabb.min.z, currentAABB.min.z);
    //         aabb.max.x = max(aabb.max.x, currentAABB.max.x);
    //         aabb.max.y = max(aabb.max.y, currentAABB.max.y);
    //         aabb.max.z = max(aabb.max.z, currentAABB.max.z);
    //     }
    //     cout << "Minimum point (X, Y, Z): " << aabb.min.x << ", " << aabb.min.y << ", " << aabb.min.z << endl;
    //     cout << "Maximum point (X, Y, Z): " << aabb.max.x << ", " << aabb.max.y << ", " << aabb.max.z << endl;
    //     cout << "Center point (X, Y, Z): " << aabb.getCenter().x << ", " << aabb.getCenter().y << ", " << aabb.getCenter().z << endl;
    // } catch (const runtime_error& e) {
    //     cerr << "Error: " << e.what() << endl;
    //     return 1;
    // }
}