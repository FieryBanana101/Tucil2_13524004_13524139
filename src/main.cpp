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

    Octree *octree = new Octree(maxDepth, vertices, faceIndexes, showBuildDuration);
    octree->printStatistic(showVerboseStats);

    ObjParser::serialize(octree, resultPath, showSerializeDuration);
}