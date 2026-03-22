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
    // octree->root->setType(OCTREE_NON_LEAF);
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



void Octree::serializeRecursively(
    OctreeNode *currNode, 
    ofstream &file, 
    int &numVertices
){

    if(currNode->getType() == OCTREE_FILLED_LEAF){

        AABB boundingBox = currNode->getBoundingBox();

        /*
            Cube subdivision viewed from the face such that bottom-left-front is the min vertex,
            and top-right-back is the max vertex.

            3D view of the subdivision:
                v7 v6
                v4 v5  <-- back face
            v3 v2
            v0 v1  <-- front face
        */

        Vector3 v0, v1, v2, v3, v4, v5, v6, v7;
        v0 = Vector3(boundingBox.min.x, boundingBox.min.y, boundingBox.min.z);
        v1 = Vector3(boundingBox.max.x, boundingBox.min.y, boundingBox.min.z);
        v2 = Vector3(boundingBox.max.x, boundingBox.max.y, boundingBox.min.z);
        v3 = Vector3(boundingBox.min.x, boundingBox.max.y, boundingBox.min.z);
        v4 = Vector3(boundingBox.min.x, boundingBox.min.y, boundingBox.max.z);
        v5 = Vector3(boundingBox.max.x, boundingBox.min.y, boundingBox.max.z);
        v6 = Vector3(boundingBox.max.x, boundingBox.max.y, boundingBox.max.z);
        v7 = Vector3(boundingBox.min.x, boundingBox.max.y, boundingBox.max.z);

        file << "# Octree node\n";
        file << "v " << v0.x << ' ' << v0.y << ' ' << v0.z << '\n';
        file << "v " << v1.x << ' ' << v1.y << ' ' << v1.z << '\n';
        file << "v " << v2.x << ' ' << v2.y << ' ' << v2.z << '\n';
        file << "v " << v3.x << ' ' << v3.y << ' ' << v3.z << '\n';
        file << "v " << v4.x << ' ' << v4.y << ' ' << v4.z << '\n';
        file << "v " << v5.x << ' ' << v5.y << ' ' << v5.z << '\n';
        file << "v " << v6.x << ' ' << v6.y << ' ' << v6.z << '\n';
        file << "v " << v7.x << ' ' << v7.y << ' ' << v7.z << '\n';

        // Front face
        file << "f " << numVertices + 2 << ' ' << numVertices + 4 << ' ' << numVertices + 1 << '\n';
        file << "f " << numVertices + 2 << ' ' << numVertices + 4 << ' ' << numVertices + 3 << '\n';
        
        // Right face
        file << "f " << numVertices + 3 << ' ' << numVertices + 6 << ' ' << numVertices + 2 << '\n';
        file << "f " << numVertices + 3 << ' ' << numVertices + 6 << ' ' << numVertices + 7 << '\n';

        // Back face
        file << "f " << numVertices + 5 << ' ' << numVertices + 7 << ' ' << numVertices + 6 << '\n';
        file << "f " << numVertices + 5 << ' ' << numVertices + 7 << ' ' << numVertices + 8 << '\n';

        // Left face
        file << "f " << numVertices + 1 << ' ' << numVertices + 8 << ' ' << numVertices + 5 << '\n';
        file << "f " << numVertices + 1 << ' ' << numVertices + 8 << ' ' << numVertices + 4 << '\n';

        // Top face
        file << "f " << numVertices + 3 << ' ' << numVertices + 8 << ' ' << numVertices + 4 << '\n';
        file << "f " << numVertices + 3 << ' ' << numVertices + 8 << ' ' << numVertices + 7 << '\n';

        // Bottom face
        file << "f " << numVertices + 1 << ' ' << numVertices + 6 << ' ' << numVertices + 2 << '\n';
        file << "f " << numVertices + 1 << ' ' << numVertices + 6 << ' ' << numVertices + 5 << '\n';

        file << "\n\n\n";
        numVertices += 8;

    }

    for(int i = 0; i < 8; i++){
        OctreeNode *child = currNode->getChildren(i);
        if(child) serializeRecursively(child, file, numVertices);
    }

}