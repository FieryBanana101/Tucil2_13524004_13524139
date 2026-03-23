#include "Octree.hpp"
#include <limits>
#include <iostream>
#include <chrono>

Octree::Octree(
    const int maxDepth, 
    vector<Vector3> vertices, 
    vector<Vector3> faceIndexes, 
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

    root = new OctreeNode(globalBoundingBox);
    root->setType(OCTREE_NON_LEAF);
    buildRecursively(root, 0, vertices, faceIndexes, this);

    // Due to the naive way we serialize each voxel, this will be as simple as this, 
    // unless there is some kind of optimization on serializing the voxel into vertices and faces
    facesNum = voxelNum * 12;
    verticesNum = voxelNum * 8;


    // Record end time and show build duration if needed
    if(showDuration){

        end = chrono::steady_clock::now();
        chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        string durationUnit = (duration >= 1000 ? "s" : "ms");
        if(durationUnit == "ms"){
            cout << "[Finished building octree in "  << duration << ' ' << "ms]\n\n";
        }
        else{
            cout << "[Finished building octree in "  << static_cast<float>(duration) / 1000 << ' ' << "s]\n\n";
        }

    }

}



void Octree::buildRecursively(
    OctreeNode *currNode, 
    int currDepth, 
    vector<Vector3>& vertices, 
    vector<Vector3>& faceIndexes, 
    Octree *octree
){

    AABB currBoundingBox = currNode->getBoundingBox();
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
        AABB boundingBox = child->getBoundingBox();

        vector<Vector3> regionFaceIndexes;
        for(Vector3 faceIndex : faceIndexes){
            Triangle triangle = Triangle(
                Vector3(vertices[static_cast<int>(faceIndex.x)]),
                Vector3(vertices[static_cast<int>(faceIndex.y)]),
                Vector3(vertices[static_cast<int>(faceIndex.z)])
            );
            if(boundingBox.intersect(triangle)){
                regionFaceIndexes.push_back(faceIndex);
            }
        }

        if(regionFaceIndexes.empty()){
            child->setType(OCTREE_EMPTY_LEAF);
        }
        else if(currDepth + 1 >= octree->getMaxDepth()){
            child->setType(OCTREE_FILLED_LEAF);
            octree->incVoxelNum();
        }
        else {
            child->setType(OCTREE_NON_LEAF);
            buildRecursively(child, currDepth + 1, vertices, regionFaceIndexes, octree);
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
    for(int i = 0; i < maxDepth; i++){
        cout << "   " << i + 1 << ": " << nodeStats[i] << " Nodes\n";
    }

    cout << "Skipped nodes (AKA leaf nodes which does not intersect with any mesh face) count based on depth: \n";
    for(int i = 0; i < maxDepth; i++){
        cout << "   " << i + 1 << ": " << emptyLeafStats[i] << " Nodes\n";
    }

    cout << "===================================================================================================\n\n";
}