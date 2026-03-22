#include <iostream>
#include <vector>
#include <chrono>
#include "ObjParser.hpp"
#include "Triangle.hpp"
#include "AABB.hpp"
#include "Octree.hpp"

using namespace std;

int main(void) {

    /* Modify these variable for testing, TODO: these value should came from user interaction (preferably GUI?)*/
    const string sourcePath = "test/teddy.obj", resultPath = "test/result.obj";
    const int maxDepth = 7;
    const bool 
        showParseDuration = true,
        showBuildDuration = true,
        showSerializeDuration = true,
        showVerboseStats = true;
    /******************************************************************************/

    vector<Vector3> vertices, faceIndexes;
    ObjParser::parse(sourcePath, showParseDuration, vertices, faceIndexes);

    Octree *octree = Octree::build(maxDepth, vertices, faceIndexes, showBuildDuration);
    Octree::printStatistic(octree, showVerboseStats);

    ObjParser::serialize(octree, resultPath, showSerializeDuration);
}