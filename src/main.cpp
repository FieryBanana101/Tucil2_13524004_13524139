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


/* Global thread state for concurrency and synchronization, defined in static class OctreeBuilder */
int                                     OctreeBuilder::maxThreadUsed;
stack<OctreeBuilder::TaskDescriptor>    OctreeBuilder::taskStack;
int                                     OctreeBuilder::activeThreads;
vector<vector<Vector3>*>                OctreeBuilder::faceIndexesListTracker;
mutex                                   OctreeBuilder::stackLock, OctreeBuilder::counterLock, OctreeBuilder::faceIndexesLock;


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

    OctreeBuilder::setMaxThreads(threadCount);

    vector<Vector3> vertices, faceIndexes;
    ObjParser::parse(sourcePath, true, vertices, faceIndexes);

    auto processStart = chrono::steady_clock::now();

    Octree *octree = new Octree(maxDepth, vertices, faceIndexes, true);
    octree->printStatistic(false);

    auto processEnd = chrono::steady_clock::now();
    auto processDuration = chrono::duration_cast<chrono::milliseconds>(processEnd - processStart).count();
    if(processDuration >= 1000){
        cout << "Total processing time: " << static_cast<float>(processDuration) / 1000 << " s\n\n";
    } else {
        cout << "Total processing time: " << processDuration << " ms\n\n";
    }

    ObjParser::serialize(octree, resultPath, true);

    if(showViewer){
        GUI viewer(octree);
        viewer.run();
    }

    delete octree;
}
