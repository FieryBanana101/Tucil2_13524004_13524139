#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "ObjParser.hpp"
#include "Triangle.hpp"
#include "AABB.hpp"
#include "Octree.hpp"

using namespace std;


/* Build configuration */
int                 BuildConfig::maxThreadToUse;
ThreadSyncMethod    BuildConfig::syncMethod;
int                 BuildConfig::minimizeFileSize;

/* Global octree operation utility, especially for concurrency and synchronization, defined in static class OctreeContext */
mutex                                   OctreeContext::generalMutex, OctreeContext::specificMutex1, OctreeContext::specificMutex2,
                                        OctreeContext::specificMutex3, OctreeContext::specificMutex4;
condition_variable                      OctreeContext::condVar;

stack<OctreeContext::TaskDescriptor>    OctreeContext::taskStack;
int                                     OctreeContext::activeThreads;
bool                                    OctreeContext::exitWorkerThreads;
vector<vector<Vector3>*>                OctreeContext::faceIndexesListTracker;



int main(void){

    /* Modify these variable for testing, TODO: these value should came from user interaction (preferably GUI?) */

    const string 
        sourcePath = "test/cow.obj",
        resultPath = "test/result.obj";
    const bool 
        showParseDuration = true,
        showBuildDuration = true,
        showSerializeDuration = true,
        showVerboseStats = false,
        maximizeConcurrency = false,
        minimizeFileSize = true;
    const int
        maxDepth = 9,
        threadsNumChoice = 8;  // Ignored when (maximizeConcurrency == true)
    const ThreadSyncMethod
        syncMethod = SYNC_SPINLOCK; // Spinlock vs sleep on multi-threading

    /******************************************************************************/

    BuildConfig::maxThreadToUse = (maximizeConcurrency ? thread::hardware_concurrency() : threadsNumChoice);
    BuildConfig::syncMethod = syncMethod;
    BuildConfig::minimizeFileSize = minimizeFileSize;

    vector<Vector3> vertices, faceIndexes;
    ObjParser::parse(sourcePath, showParseDuration, vertices, faceIndexes);

    auto processStart = chrono::steady_clock::now();
    
    Octree *octree = new Octree(maxDepth, vertices, faceIndexes, showBuildDuration);
    ObjParser::serialize(octree, resultPath, showSerializeDuration);
    octree->printStatistic(showVerboseStats);

    auto processEnd = chrono::steady_clock::now();
    auto processDuration = chrono::duration_cast<chrono::milliseconds>(processEnd - processStart).count();
    if(processDuration >= 1000){
        cout << "Total processing time: " << static_cast<float>(processDuration) / 1000 << " s\n\n";
    } else {
        cout << "Total processing time: " << processDuration << " ms\n\n";
    }

}