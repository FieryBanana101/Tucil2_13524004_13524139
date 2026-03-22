#include <iostream>
#include <vector>
#include <chrono>
#include "ObjParser.hpp"
#include "Triangle.hpp"
#include "AABB.hpp"
#include "Octree.hpp"

using namespace std;


int main(void) {

    /* Modify this to test */
    const string sourcePath = "test/teddy.obj", resultPath = "test/result.obj";
    const int maxDepth = 8;
    /******************************************************************************/

    vector<Vector3> vertices, faceIndexes;
    ObjParser::parse(sourcePath, vertices, faceIndexes);

    chrono::time_point start = chrono::steady_clock::now();
    Octree *octree = Octree::build(maxDepth, vertices, faceIndexes);
    chrono::time_point end = chrono::steady_clock::now();

    cout << "Tree max depth: " << octree->getMaxDepth() << '\n';
    cout << "Number of voxel formed: " << octree->getVoxelNum() << '\n';
    cout << "Total number of vertex: " << octree->getVerticesNum() << '\n';
    cout << "Total number of faces: " << octree->getFacesNum() << '\n';

    chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    string durationUnit = (duration >= 1000 ? "s" : "ms");
    if(durationUnit == "ms"){
        cout << "[Finished building octree in "  << duration << ' ' << "ms]\n";
    }
    else{
        cout << "[Finished building octree in "  << static_cast<float>(duration) / 1000 << ' ' << "s]\n";
    }
    cout << '\n';


    start = chrono::steady_clock::now();
    ObjParser::serialize(resultPath, octree);
    end = chrono::steady_clock::now();

    duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    durationUnit = (duration >= 1000 ? "s" : "ms");
    if(durationUnit == "ms"){
        cout << "[Finished serializing octree into .obj file in "  << duration << ' ' << "ms]\n";
    }
    else{
        cout << "[Finished serializing octree into .obj file in "  << static_cast<float>(duration) / 1000 << ' ' << "s]\n";
    }

    cout << "Voxelized mesh successfully saved at '" << resultPath << "'\n";

}