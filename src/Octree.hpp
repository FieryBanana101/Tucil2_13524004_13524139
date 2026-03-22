#pragma once

#include "Triangle.hpp"
#include "AABB.hpp"

#include <cstdint>
#include <vector>
#include <fstream>

using namespace std;

enum OctreeNodeType { 
    OCTREE_NON_LEAF,
    OCTREE_EMPTY_LEAF,
    OCTREE_FILLED_LEAF
};

class OctreeNode {
private:
    OctreeNode *children[8];
    AABB boundingBox;
    OctreeNodeType nodeType;


public:
    OctreeNode(AABB boundingBox) : boundingBox(boundingBox), nodeType(OCTREE_EMPTY_LEAF) { for(int i = 0; i < 8; i++) children[i] = nullptr; };
    ~OctreeNode(){ for(int i = 0; i < 8; i++) delete children[i]; };

    OctreeNode **getChildren() const;
    OctreeNode *getChildren(uint8_t idx) const;
    OctreeNodeType getType() const;
    void setType(OctreeNodeType type);
    void setChildren(uint8_t idx, OctreeNode *child);
    AABB getBoundingBox() const;
};



class Octree {
private:
    OctreeNode *root;
    int maxDepth;

public:
    Octree(int maxDepth) : maxDepth(maxDepth) {};
    ~Octree(){ delete root; };

    OctreeNode *getRoot() const;
    static Octree *build(int maxDepth, vector<Vector3>& vertices, vector<Vector3>& faceIndexes);
    static void buildRecursively(OctreeNode *currNode, int currDepth, int maxDepth, vector<Vector3>& vertices, vector<Vector3>& faceIndexes);
    static void traverseRecursively(OctreeNode *currNode, ofstream &file, int &numVertices);
};