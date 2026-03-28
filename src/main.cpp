#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <limits>
#include "ObjParser.hpp"
#include "Triangle.hpp"
#include "AABB.hpp"
#include "Octree.hpp"
#include "GUI.hpp"

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

    string sourcePath, resultPath;
    int maxDepth, threadCount;

    cout << "Input .obj file path: ";
    getline(cin, sourcePath);

    cout << "Output .obj file path: ";
    getline(cin, resultPath);

    cout << "Max octree depth (1-12): ";
    while(!(cin >> maxDepth) || maxDepth < 1 || maxDepth > 12){
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Enter a number between 1 and 12: ";
    }

    int hwThreads = static_cast<int>(thread::hardware_concurrency());
    cout << "Number of threads (1-" << hwThreads << ", 0 for max): ";
    while(!(cin >> threadCount) || threadCount < 0 || threadCount > hwThreads){
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Enter a number between 0 and " << hwThreads << ": ";
    }
    if(threadCount == 0) threadCount = hwThreads;

    char showViewerChoice;
    cout << "Show 3D viewer after voxelization? (y/n): ";
    while(!(cin >> showViewerChoice) || (showViewerChoice != 'y' && showViewerChoice != 'n')){
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter y or n: ";
    }
    bool showViewer = (showViewerChoice == 'y');

    BuildConfig::maxThreadToUse = threadCount;
    int syncChoice;
    cout << "Thread sync method (0 for Spinlock, 1 for Sleep): ";
    while(!(cin >> syncChoice) || (syncChoice != 0 && syncChoice != 1)){
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Enter 0 or 1: ";
    }
    BuildConfig::syncMethod = (syncChoice == 0) ? SYNC_SPINLOCK : SYNC_SLEEP;

    char minimizeChoice;
    cout << "Minimize file size? this will trade smaller file size for even more slower performance (y/n): ";
    while(!(cin >> minimizeChoice) || (minimizeChoice != 'y' && minimizeChoice != 'n')){
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter y or n: ";
    }
    BuildConfig::minimizeFileSize = (minimizeChoice == 'y') ? 1 : 0;

    vector<Vector3> vertices, faceIndexes;
    ObjParser::parse(sourcePath, true, vertices, faceIndexes);

    auto processStart = chrono::steady_clock::now();

    Octree *octree = new Octree(maxDepth, vertices, faceIndexes, true);
    ObjParser::serialize(octree, resultPath, true);
    octree->printStatistic(false);


    auto processEnd = chrono::steady_clock::now();
    auto processDuration = chrono::duration_cast<chrono::milliseconds>(processEnd - processStart).count();
    if(processDuration >= 1000){
        cout << "Total processing time: " << static_cast<float>(processDuration) / 1000 << " s\n\n";
    } else {
        cout << "Total processing time: " << processDuration << " ms\n\n";
    }


    if(showViewer){
        GUI viewer(octree);
        viewer.run();
    }

    delete octree;
}
