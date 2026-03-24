#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "ObjParser.hpp"
#include "Triangle.hpp"
#include "AABB.hpp"
#include "Octree.hpp"

using namespace std;


/* Global thread state for concurrency and synchronization, defined in static class OctreeBuilder */
int                                     OctreeBuilder::maxThreadUsed;
stack<OctreeBuilder::TaskDescriptor>    OctreeBuilder::taskStack;
int                                     OctreeBuilder::activeThreads;
vector<vector<Vector3>*>                OctreeBuilder::faceIndexesListTracker;
mutex                                   OctreeBuilder::stackLock, OctreeBuilder::counterLock, OctreeBuilder::faceIndexesLock;


int main(void){

    /* Modify these variable for testing, TODO: these value should came from user interaction (preferably GUI?) */

    const string 
        sourcePath = "test/teapot.obj",
        resultPath = "test/result.obj";
    const bool 
        showParseDuration = true,
        showBuildDuration = true,
        showSerializeDuration = true,
        showVerboseStats = false,
        maximizeConcurrency = false;
    const int 
        maxDepth = 8,
        threadsNumChoice = 4;  // Ignored when (maximizeConcurrency == true)

    /******************************************************************************/

    OctreeBuilder::setMaxThreads((maximizeConcurrency ? thread::hardware_concurrency() : threadsNumChoice));

    vector<Vector3> vertices, faceIndexes;
    ObjParser::parse(sourcePath, showParseDuration, vertices, faceIndexes);

    auto processStart = chrono::steady_clock::now();

    Octree *octree = new Octree(maxDepth, vertices, faceIndexes, showBuildDuration);
    octree->printStatistic(showVerboseStats);

    auto processEnd = chrono::steady_clock::now();
    auto processDuration = chrono::duration_cast<chrono::milliseconds>(processEnd - processStart).count();
    if(processDuration >= 1000){
        cout << "Total processing time: " << static_cast<float>(processDuration) / 1000 << " s\n\n";
    } else {
        cout << "Total processing time: " << processDuration << " ms\n\n";
    }

    ObjParser::serialize(octree, resultPath, showSerializeDuration);
}