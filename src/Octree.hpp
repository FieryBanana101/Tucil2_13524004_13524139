#pragma once

#include "Triangle.hpp"
#include "AABB.hpp"

#include <cstdint>
#include <vector>
#include <fstream>
#include <utility> // std::forward<>

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
    OctreeNode(AABB boundingBox) : 
        boundingBox(boundingBox), 
        nodeType(OCTREE_EMPTY_LEAF) 
        { for(int i = 0; i < 8; i++) children[i] = nullptr; };

    ~OctreeNode(){ for(int i = 0; i < 8; i++) delete children[i]; };

    OctreeNodeType getType() const { return nodeType; }
    OctreeNode* const* getChildren() const { return children; }
    OctreeNode *getChildren(uint8_t idx) const { return children[idx]; }
    AABB getBoundingBox() const { return boundingBox; }
    void setChildren(uint8_t idx, OctreeNode *child){ children[idx] = child; }
    void setType(OctreeNodeType type){ nodeType = type; }
};



class Octree {
private:
    OctreeNode *root;
    int maxDepth;
    int voxelNum;
    int verticesNum;
    int facesNum;

    static void buildRecursively(
        OctreeNode *currNode, 
        int currDepth, 
        vector<Vector3>& vertices, 
        vector<Vector3>& faceIndexes,
        Octree *octree
    );


    template <typename Func>
    static void traverseRecursively(OctreeNode *currNode, int currDepth, Func&& func){

        forward<Func>(func)(currNode, currDepth);

        for(int i = 0; i < 8; i++){
            OctreeNode *child = currNode->getChildren(i);
            if(child != nullptr){
                traverseRecursively(child, currDepth + 1, func);
            }
        }

    }


public:
    Octree(int maxDepth) : 
        maxDepth(maxDepth), 
        voxelNum(0), 
        verticesNum(0), 
        facesNum(0)
        {};

    ~Octree(){ delete root; };

    OctreeNode *getRoot() const { return root; }
    int getVoxelNum() const { return voxelNum; }
    int getVerticesNum() const { return verticesNum; }
    int getFacesNum() const { return facesNum; }
    int getMaxDepth() const { return maxDepth; }
    void setVoxelNum(int val) { voxelNum = val; }
    void setVerticesNum(int val) { verticesNum = val; }
    void setFacesNum(int val) { facesNum = val; }
    void incVoxelNum() { voxelNum++; }
    void incVerticesNum() { verticesNum++; }
    void incFacesNum() { facesNum++; }

    static Octree *build(
        int maxDepth, 
        vector<Vector3>& vertices, 
        vector<Vector3>& faceIndexes,
        bool showDuration
    );
    static void printStatistic(const Octree *octree, const bool isVerbose);


    /* General purpose DFS to traverse octree and call a lambda function on each node,
     * The lambda function must accept two parameter with signature (OctreeNode *, int),
     *
     * Example usage,
     * 
     * vector<float> volumeList;
     * Octree::traverse(octree, 
     * 
     * [&volumeList](OctreeNode *currNode, int currDepth){
     *      float volume = currNode->getBoundingBox().getVolume();
     *      volumeList.push_back(volume);
     *      cout << "Volume is " <<  volume << '\n';  
     * 
     *      (void) currDepth;
     * });
     */
    template <typename Func>
    static void traverse(const Octree *octree, Func&& func){
        traverseRecursively(octree->root, 0, func);
    }
};