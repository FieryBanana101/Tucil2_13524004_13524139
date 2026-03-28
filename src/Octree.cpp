#include "Octree.hpp"
#include <limits>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>    



void OctreeContext::reset(){
    activeThreads = 0;
    exitWorkerThreads = false;
    faceIndexesListTracker.clear();
}


OctreeContext::TaskDescriptor OctreeContext::popTask(){

    if(BuildConfig::syncMethod == SYNC_SPINLOCK){
        TaskDescriptor ret = TaskDescriptor(nullptr, 0, nullptr);
        unique_lock<mutex> tempLock(specificMutex1);

        if(!taskStack.empty()){
            ret = taskStack.top();
            taskStack.pop();
        }

        return ret;
    }


    unique_lock<mutex> tempLock(generalMutex);
    condVar.wait(tempLock, [](){
        return exitWorkerThreads || !taskStack.empty();
    });

    if(exitWorkerThreads){
        return TaskDescriptor(nullptr, 0, nullptr);
    }

    TaskDescriptor currTask = taskStack.top();
    taskStack.pop();

    activeThreads++;

    return currTask;
}


void OctreeContext::pushTask(OctreeNode *currNode, int currDepth, vector<Vector3> *regionFaceIndexes){
    if(BuildConfig::syncMethod == SYNC_SPINLOCK){

        unique_lock<mutex> tempLock(specificMutex1);
        taskStack.push(TaskDescriptor(currNode, currDepth, regionFaceIndexes));

    }
    else{

        unique_lock<mutex> tempLock(generalMutex);
        taskStack.push(TaskDescriptor(currNode, currDepth, regionFaceIndexes));
        tempLock.unlock();
        condVar.notify_one();

    }
}


void OctreeContext::addFaceIndexes(vector<Vector3> *ptr){ 
    unique_lock<mutex> tempLock(specificMutex3);
    faceIndexesListTracker.push_back(ptr);
};



void OctreeContext::flushSerializerString(ofstream *stream, stringstream &ss){
    unique_lock<mutex> tempLock(specificMutex3);
    (*stream) << ss.rdbuf();
    ss.clear();
}



int OctreeContext::getSerializerVertice(int *verticeTracker){
    unique_lock<mutex> tempLock(specificMutex4);
    int ret = *verticeTracker;
    *verticeTracker += 8;
    return ret;
}



Octree::Octree(
    const int maxDepth, 
    vector<Vector3> &vertices, 
    vector<Vector3> &faceIndexes, 
    const bool showDuration
) : maxDepth(maxDepth), voxelNum(0), verticesNum(0), facesNum(0)
{

    cout << "Building octree from a mesh with " << vertices.size() << " vertices and " << faceIndexes.size() << " faces...\n";
    chrono::steady_clock::time_point start, end;

    AABB globalBoundingBox(
        Vector3(std::numeric_limits<float>::max()), 
        Vector3(std::numeric_limits<float>::min())
    );

    // Record build start time if needed
    if(showDuration){
        start = chrono::steady_clock::now();
    }

    for(Vector3 vertex : vertices){
       globalBoundingBox.min.x = min(globalBoundingBox.min.x, vertex.x);
       globalBoundingBox.min.y = min(globalBoundingBox.min.y, vertex.y);
       globalBoundingBox.min.z = min(globalBoundingBox.min.z, vertex.z);

       globalBoundingBox.max.x = max(globalBoundingBox.max.x, vertex.x);
       globalBoundingBox.max.y = max(globalBoundingBox.max.y, vertex.y);
       globalBoundingBox.max.z = max(globalBoundingBox.max.z, vertex.z);
    }

    Vector3 size = globalBoundingBox.max - globalBoundingBox.min;
    float maxSide = max({size.x, size.y, size.z});
    Vector3 center = globalBoundingBox.getCenter();
    float half = maxSide / 2.0f;
    globalBoundingBox.min = center - half;
    globalBoundingBox.max = center + half;

    root = new OctreeNode(globalBoundingBox);
    root->setType(OCTREE_NON_LEAF);

    OctreeContext::reset();

    vector<Vector3> *regionFaceIndexes = new vector<Vector3>(faceIndexes); // copy to heap so other threads can access it
    OctreeContext::addFaceIndexes(regionFaceIndexes);
    OctreeContext::pushTask(root, 0, regionFaceIndexes);

    int maxThreadCount = BuildConfig::maxThreadToUse;
    thread threads[maxThreadCount];

    auto buildThreadCallback = [](OctreeNode *currNode, int currDepth, vector<Vector3> *regionFaceIndexes, vector<Vector3> &vertices, Octree *octree){ 
        octree->buildRecursively(currNode, currDepth, regionFaceIndexes, vertices, octree); 
        return false;
    };
            
    for(int i = 0; i < maxThreadCount; i++){
        threads[i] = thread(
            OctreeContext::genericThreadWorker<decltype(buildThreadCallback), vector<Vector3>&, Octree*>,
            buildThreadCallback,
            ref(vertices), 
            this
        );
    }

    for(int i = 0; i < maxThreadCount; i++){
        threads[i].join();
    }

    OctreeContext::freeFaceIndexes();
    
    // Due to the naive way we serialize each voxel, this will be as simple as this, 
    // unless there is some kind of optimization on serializing the voxel into vertices and faces
    if(BuildConfig::minimizeFileSize){
        verticesNum = uniqueVerticesMap.size();
    }
    else{
        verticesNum = voxelNum * 8;
    }
    facesNum = voxelNum * 12;


    // Record end time and show build duration if needed
    if(showDuration){

        end = chrono::steady_clock::now();
        chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        if(duration < 1000){
            cout << "[Finished building octree in "  << duration << " ms ";
            cout << '(' << maxThreadCount << " threads used)]";
        }
        else{
            cout << "[Finished building octree in "  << static_cast<float>(duration) / 1000 << " s ";
            cout << '(' << maxThreadCount << " threads used)]";
        }
        cout << "\n\n";

    }

}



void Octree::buildRecursively(
    OctreeNode *currNode, 
    int currDepth, 
    vector<Vector3> *faceIndexes, 
    vector<Vector3>& vertices,
    Octree *octree
){
    AABB &currBoundingBox = currNode->getBoundingBox();
    Vector3 center = currBoundingBox.getCenter();
    Vector3 distToCenter = center - currBoundingBox.min;
    int childIdx = 0;
    for(int xScale = 0; xScale <= 1; xScale++){
        for(int yScale = 0; yScale <= 1; yScale++){
            for(int zScale = 0; zScale <= 1; zScale++){
                Vector3 aabbMin = Vector3(
                    currBoundingBox.min.x + distToCenter.x * xScale,
                    currBoundingBox.min.y + distToCenter.y * yScale,
                    currBoundingBox.min.z + distToCenter.z * zScale
                );

                Vector3 aabbMax = aabbMin + distToCenter;

                AABB childBoundingBox(aabbMin, aabbMax);
                OctreeNode *childNode = new OctreeNode(childBoundingBox);

                currNode->setChildren(childIdx, childNode);
                childIdx++;
            }
        }
    }


    for(int i = 0; i < 8; i++){
        OctreeNode *child = currNode->getChildren(i);
        AABB &boundingBox = child->getBoundingBox();

        vector<Vector3> *regionFaceIndexes = new vector<Vector3>();  // initialize in heap so other threads can access it
        for(Vector3 faceIndex : *faceIndexes){
            Triangle triangle = Triangle(
                Vector3(vertices[static_cast<int>(faceIndex.x)]),
                Vector3(vertices[static_cast<int>(faceIndex.y)]),
                Vector3(vertices[static_cast<int>(faceIndex.z)])
            );
            if(boundingBox.intersect(triangle)){
                (*regionFaceIndexes).push_back(faceIndex);
            }
        }
                
        if((*regionFaceIndexes).empty()){
            child->setType(OCTREE_EMPTY_LEAF);
        }
        else if(currDepth + 1 >= octree->getMaxDepth()){
            child->setType(OCTREE_FILLED_LEAF);

            unique_lock<mutex> tempLock(this->octreeMutex);
            voxelNum++;

            if(BuildConfig::minimizeFileSize){
                tempLock.unlock();

                /*
                    Cube subdivision viewed from the face such that bottom-left-front is the min vertex,
                    and top-right-back is the max vertex.

                    3D view of the subdivision:
                        v[7] v[6]
                        v[4] v[5]  <-- back face
                    v[3] v[2]
                    v[0] v[1]  <-- front face
                */

                Vector3 v[8];
                v[0] = Vector3(boundingBox.min.x, boundingBox.min.y, boundingBox.min.z);
                v[1] = Vector3(boundingBox.max.x, boundingBox.min.y, boundingBox.min.z);
                v[2] = Vector3(boundingBox.max.x, boundingBox.max.y, boundingBox.min.z);
                v[3] = Vector3(boundingBox.min.x, boundingBox.max.y, boundingBox.min.z);
                v[4] = Vector3(boundingBox.min.x, boundingBox.min.y, boundingBox.max.z);
                v[5] = Vector3(boundingBox.max.x, boundingBox.min.y, boundingBox.max.z);
                v[6] = Vector3(boundingBox.max.x, boundingBox.max.y, boundingBox.max.z);
                v[7] = Vector3(boundingBox.min.x, boundingBox.max.y, boundingBox.max.z);

                for(int i = 0; i < 8; i++){
                    tempLock.lock();
            
                    (void) uniqueVerticesMap.try_emplace(v[i], 0);
                    
                    tempLock.unlock();
                }
            }
        }
        else{
            child->setType(OCTREE_NON_LEAF);
            OctreeContext::addFaceIndexes(regionFaceIndexes);
            OctreeContext::pushTask(child, currDepth + 1, regionFaceIndexes);
        }
    }
}




void Octree::printStatistic(const bool isVerbose) {

    cout << "\n========================================= OCTREE STATISTICS ========================================\n";
    cout << "Octree max depth: " << maxDepth << '\n';
    cout << "Number of voxel formed: " << voxelNum << '\n';
    cout << "Total number of vertices: " << verticesNum << '\n';
    cout << "Total number of faces: " << facesNum << '\n';

    if(isVerbose){ 
        cout << "\nOctree Structure: \n";
        for(int i = 0; i < 100; i++) cout << '-'; 
        cout << '\n'; 
    }

    // Do DFS to acquire needed statistics
    vector<int> nodeStats(maxDepth+1);
    vector<int> emptyLeafStats(maxDepth+1);
    int nodeIdx = 1;
    
    traverse(
    [&nodeStats, &emptyLeafStats, &nodeIdx, isVerbose](OctreeNode *currNode, int currDepth){

        // Add extra information if verbosity is enabled
        if(isVerbose){
            cout << '|';
            for(int i = 0; i < currDepth + 1; i++) cout << "--";

            string nodeTypeStr;
            switch(currNode->getType()){
                case OCTREE_EMPTY_LEAF:
                    nodeTypeStr = "NON-INTERSECTING LEAF";
                    break;
                case OCTREE_FILLED_LEAF:
                    nodeTypeStr = "INTERSECTING LEAF";
                    break;
                default:
                    nodeTypeStr = "INTERNAL";
            }

            cout << " Node " << nodeIdx++ << " (Depth = " << currDepth;
            cout << ", Bounding box volume = " << currNode->getBoundingBox().getVolume();
            cout << ", type = " << nodeTypeStr << ")\n";
        }

        nodeStats[currDepth]++;
        if(currNode->getType() == OCTREE_EMPTY_LEAF){
            emptyLeafStats[currDepth]++;
        }

    });

    if(isVerbose){ 
        for(int i = 0; i < 100; i++) cout << '-'; 
        cout << '\n'; 
    }


    cout << "Nodes count based on depth: \n";
    for(int i = 0; i <= maxDepth; i++){
        cout << "   " << i << ": " << nodeStats[i] << " Nodes";
        if(i == 0) cout << " (root)";
        cout << '\n';
    }

    cout << "Skipped nodes (AKA leaf nodes which does not intersect with any mesh face) count based on depth: \n";
    for(int i = 0; i <= maxDepth; i++){
        cout << "   " << i << ": " << emptyLeafStats[i] << " Nodes";
        if(i == 0) cout << " (root)";
        cout << '\n';
    }

    cout << "===================================================================================================\n\n";
}



void Octree::deduplicateFaces(){
    
    auto deduplicatorCallback = 
    [](OctreeNode *currNode, int _, vector<Vector3> *__, Octree *octree)
    {
            (void) _, (void) __; // Due to convention of the genericThreadWorker class template, we will have these two unused variable

            if(currNode->getType() == OctreeNodeType::OCTREE_FILLED_LEAF){
                
                AABB &boundingBox = currNode->getBoundingBox();

                Vector3 v[8];
                v[0] = Vector3(boundingBox.min.x, boundingBox.min.y, boundingBox.min.z);
                v[1] = Vector3(boundingBox.max.x, boundingBox.min.y, boundingBox.min.z);
                v[2] = Vector3(boundingBox.max.x, boundingBox.max.y, boundingBox.min.z);
                v[3] = Vector3(boundingBox.min.x, boundingBox.max.y, boundingBox.min.z);
                v[4] = Vector3(boundingBox.min.x, boundingBox.min.y, boundingBox.max.z);
                v[5] = Vector3(boundingBox.max.x, boundingBox.min.y, boundingBox.max.z);
                v[6] = Vector3(boundingBox.max.x, boundingBox.max.y, boundingBox.max.z);
                v[7] = Vector3(boundingBox.min.x, boundingBox.max.y, boundingBox.max.z);


                vector<vector<uint32_t>> triangleIdx = {
                    {1, 3, 0},  // Front face
                    {1, 3, 2},
                    {2, 5, 1},  // Right face
                    {2, 5, 6},
                    {4, 6, 5},  // Back face
                    {4, 6, 7},
                    {0, 7, 4},  // Left face
                    {0, 7, 3},
                    {2, 7, 3},  // Top face
                    {2, 7, 6},
                    {0, 5, 1},  // Bottom face
                    {0, 5, 4}
                };

                for(int i = 0; i < 12; i++){

                    uint32_t faceIdx[3];
                    Vector3 faceVertice[3];

                    faceVertice[0] = v[triangleIdx[i][0]];
                    faceVertice[1] = v[triangleIdx[i][1]];
                    faceVertice[2] = v[triangleIdx[i][2]];

                    faceIdx[0] = octree->uniqueVerticesMap[faceVertice[0]];
                    faceIdx[1] = octree->uniqueVerticesMap[faceVertice[1]];
                    faceIdx[2] = octree->uniqueVerticesMap[faceVertice[2]];

                    /* Make sure the face indexes is unique per combination (not permutation), so we sort it */
                    if(faceIdx[0] > faceIdx[1]) swap(faceIdx[0], faceIdx[1]);
                    if(faceIdx[0] > faceIdx[2]) swap(faceIdx[0], faceIdx[2]);
                    if(faceIdx[1] > faceIdx[2]) swap(faceIdx[1], faceIdx[2]);

                    tuple<uint32_t, uint32_t, uint32_t> face = make_tuple(faceIdx[0], faceIdx[1], faceIdx[2]);
                    
                    unique_lock<mutex> tempLock(octree->octreeMutex);

                    if(octree->uniqueFacesSet.find(face) == octree->uniqueFacesSet.end()){
                        octree->uniqueFacesSet.insert(face);
                    }

                }

            }

            for(int i = 0; i < 8; i++){
                OctreeNode *child = currNode->getChildren(i);
                if(child) OctreeContext::pushTask(currNode->getChildren(i), 0, nullptr);  // We only need the node pointer information
            }

            return false;
    };


    OctreeContext::pushTask(root, 0, nullptr);

    int threadsCnt = BuildConfig::maxThreadToUse;
    thread threads[threadsCnt];
    vector<stringstream *> ss(threadsCnt);

    for(int i = 0; i < threadsCnt; i++){
        ss[i] = new stringstream();

        threads[i] = thread(
            OctreeContext::genericThreadWorker<
                decltype(deduplicatorCallback), 
                Octree*
            >,
            deduplicatorCallback,
            this
        );
    }

    for(int i = 0; i < threadsCnt; i++){
        threads[i].join();
        delete ss[i];
    }


    facesNum = uniqueFacesSet.size();
}