#pragma once

#include "Triangle.hpp"
#include "AABB.hpp"

#include <cstdint>
#include <vector>
#include <fstream>
#include <sstream>
#include <utility> // std::forward<>
#include <stack>
#include <mutex>
#include <thread>
#include <condition_variable>

using namespace std;


enum OctreeNodeType { 
    OCTREE_NON_LEAF,
    OCTREE_EMPTY_LEAF,
    OCTREE_FILLED_LEAF
};


enum ThreadSyncMethod {
    SYNC_SPINLOCK,
    SYNC_SLEEP
};



struct ThreadingConfig {
    static int maxThreadToUse;
    static ThreadSyncMethod syncMethod;
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
    AABB &getBoundingBox() { return boundingBox; }
    void setChildren(uint8_t idx, OctreeNode *child){ children[idx] = child; }
    void setType(OctreeNodeType type){ nodeType = type; }
};




class OctreeContext {

private:
    struct TaskDescriptor {
        OctreeNode *currNode;
        int currDepth;
        vector<Vector3> *regionFaceIndexes;

        TaskDescriptor(OctreeNode *currNode, int currDepth, vector<Vector3>* regionFaceIndexes) :
            currNode(currNode), currDepth(currDepth), regionFaceIndexes(regionFaceIndexes) {}
    };

    static mutex generalMutex, specificMutex1, specificMutex2, specificMutex3, specificMutex4;
    static condition_variable condVar;
    
    static stack<TaskDescriptor> taskStack;
    static int activeThreads;
    static bool exitWorkerThreads;
    static vector<vector<Vector3>*> faceIndexesListTracker;  // To avoid memory leak


public:

    static void reset();
    static TaskDescriptor popTask();
    static void pushTask(OctreeNode *currNode, int currDepth, vector<Vector3> *regionFaceIndexes);
    static void addFaceIndexes(vector<Vector3> *ptr);
    static void freeFaceIndexes(){ for(vector<Vector3> *ptr : faceIndexesListTracker){ delete ptr; } }

    static void flushSerializerString(ofstream *stream, stringstream &ss);
    static int getSerializerVertice(int *verticeTracker);

    template <typename Func, typename... Args>
    static void genericThreadWorker(Func&& func, Args&&... args){
    
        bool isActive = false;
        switch(ThreadingConfig::syncMethod){

            case SYNC_SPINLOCK:
                while(true){
                    
                    auto [currNode, currDepth, regionFaceIndexes] = popTask();
                    if(currNode != nullptr){
                        if(!isActive){
                            isActive = true;
                            unique_lock<mutex> tempLock(specificMutex2);
                            activeThreads++;
                        }

                        bool ret = forward<Func>(func)(currNode, currDepth, regionFaceIndexes, forward<Args>(args)...);
                        if(ret){
                            isActive = false;
                            unique_lock<mutex> tempLock(specificMutex2);
                            activeThreads--;
                            break;
                        }
                    }
                    else if(isActive){
                        isActive = false;
                        unique_lock<mutex> tempLock(specificMutex2);
                        if(--activeThreads == 0) break;
                    }
                    else{
                        unique_lock<mutex> tempLock(specificMutex2);
                        if(activeThreads == 0) break;
                    }

                }
                break;


            case SYNC_SLEEP:
                while(true){
                
                    auto [currNode, currDepth, regionFaceIndexes] = popTask();
                    if(currNode == nullptr) break;

                    bool ret = forward<Func>(func)(currNode, currDepth, regionFaceIndexes, forward<Args>(args)...);

                    unique_lock<mutex> tempLock(generalMutex);
                    activeThreads--;

                    if(ret) break;

                    if(taskStack.empty() && activeThreads == 0){

                        exitWorkerThreads = true;
                        tempLock.unlock();
                        condVar.notify_all();
                        break;

                    }

                }
                break;    
        }
    }
};




class Octree {
private:
    OctreeNode *root;
    int maxDepth;
    int voxelNum;
    int verticesNum;
    int facesNum;
    mutex mut;  // To synchronize access to voxelNum

    void buildRecursively(
        OctreeNode *currNode, 
        int currDepth,  
        vector<Vector3>* faceIndexes,
        vector<Vector3>& vertices,
        Octree *octree
    );


public:

    /*  Construct octree with a predetermined max depth (more max depth = more detailed voxelization),
     *  tree is constructed from the list of vertices and mesh triangles (faces) which are described by 3-tuple indexes.
     *  The construction duration can be shown using the bool in the last constructor parameter.
     */
    Octree(const int maxDepth, vector<Vector3>& vertices, vector<Vector3>& faceIndexes, const bool showDuration);
    ~Octree(){ delete root; };

    OctreeNode *getRoot() const { return root; }
    int getVoxelNum() const { return voxelNum; }
    int getVerticesNum() const { return verticesNum; }
    int getFacesNum() const { return facesNum; }
    int getMaxDepth() const { return maxDepth; }
    void setVoxelNum(int val) { voxelNum = val; }
    void setVerticesNum(int val) { verticesNum = val; }
    void setFacesNum(int val) { facesNum = val; }
    void incVoxelNum() { unique_lock<mutex>tempLock(mut); voxelNum++; }
    void incVerticesNum() { verticesNum++; }
    void incFacesNum() { facesNum++; }

    void printStatistic(const bool isVerbose);


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
    template <typename Func, typename... Args>
    void traverse(Func&& func, Args&&... args){

        stack<pair<OctreeNode *, int>> st;
        st.push(make_pair(root, 0));

        while(!st.empty()){
            auto [currNode, currDepth] = st.top(); 
            st.pop();
            forward<Func>(func)(currNode, currDepth);
            for(int i = 0; i < 8; i++){
                OctreeNode *child = currNode->getChildren(i);
                if(child) st.push(make_pair(child, currDepth + 1));
            }
        }

    }

    friend class OctreeContext;
};

