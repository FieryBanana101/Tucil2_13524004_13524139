#include "Octree.hpp"
#include <limits>
#include <iostream>


Octree *Octree::build(int maxDepth, vector<Vector3>& vertices, vector<Vector3>& faceIndexes){
    AABB globalBoundingBox(
        Vector3(std::numeric_limits<float>::max()), 
        Vector3(std::numeric_limits<float>::min())
    );

    for(Vector3 vertex : vertices){
       globalBoundingBox.min.x = min(globalBoundingBox.min.x, vertex.x);
       globalBoundingBox.min.y = min(globalBoundingBox.min.y, vertex.y);
       globalBoundingBox.min.z = min(globalBoundingBox.min.z, vertex.z);

       globalBoundingBox.max.x = max(globalBoundingBox.max.x, vertex.x);
       globalBoundingBox.max.y = max(globalBoundingBox.max.y, vertex.y);
       globalBoundingBox.max.z = max(globalBoundingBox.max.z, vertex.z);
    }

    Octree *octree = new Octree(maxDepth);
    octree->root = new OctreeNode(globalBoundingBox);
    octree->root->setType(OCTREE_NON_LEAF);
    buildRecursively(octree->root, 0, vertices, faceIndexes, octree);

    // Due to the naive way we serialize each voxel, this will be as simple as this, 
    // unless there is some kind of optimization on serializing the voxel into vertices and faces
    int voxelNum = octree->getVoxelNum();
    octree->setFacesNum(voxelNum * 12);
    octree->setVerticesNum(voxelNum * 8);

    return octree;
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
