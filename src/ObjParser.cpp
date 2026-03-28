#include "ObjParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <map>
#include <set>

using namespace std;

void ObjParser::parse(const std::string& filepath, const bool showDuration, vector<Vector3>& vertices, vector<Vector3>& faceIndexes) {

    chrono::steady_clock::time_point start, end;
    ifstream file(filepath);
    string line;

    if (!file.is_open()) {
        throw runtime_error("Input obj file " + filepath + " cannot be opened.");
    }

    // Record start time if needed
    if(showDuration){
        start = chrono::steady_clock::now();
    }

    int lineNum = 0;
    while(getline(file, line)) {
        lineNum++;
        stringstream ss(line);
        string type;
        ss >> type;
        if (type == "v") {
            float x, y, z;
            if(!(ss >> x >> y >> z)){
                throw runtime_error("Invalid vertex format at line " + to_string(lineNum) + ": '" + line + "'");
            }
            string extra;
            if(ss >> extra){
                throw runtime_error("Invalid vertex format at line " + to_string(lineNum) + " expected 3 values: '" + line + "'");
            }
            vertices.push_back(Vector3(x, y, z));
        }
        else if (type == "f") {
            int i, j, k;
            if(!(ss >> i >> j >> k)){
                throw runtime_error("Invalid face format at line " + to_string(lineNum) + ": '" + line + "'");
            }
            string extra;
            if(ss >> extra){
                throw runtime_error("Invalid face format at line " + to_string(lineNum) + " expected 3 values: '" + line + "'");
            }
            if(i < 1 || j < 1 || k < 1){
                throw runtime_error("Face index must be positive at line " + to_string(lineNum) + ": '" + line + "'");
            }
            faceIndexes.push_back(Vector3(i-1, j-1, k-1));
        }
    }

    if(faceIndexes.empty()) {
        throw runtime_error("Mesh described in '" + filepath + "' does not contain any triangles/faces.");
    }

    int vertexCount = static_cast<int>(vertices.size());
    for(int i = 0; i < static_cast<int>(faceIndexes.size()); i++){
        Vector3 &f = faceIndexes[i];
        if(static_cast<int>(f.x) >= vertexCount || static_cast<int>(f.y) >= vertexCount || static_cast<int>(f.z) >= vertexCount){
            throw runtime_error("Face " + to_string(i+1) + " references a vertex index out of range (total vertices: " + to_string(vertexCount) + ").");
        }
    }
    cout << "Successfully parsed mesh in .obj file located at '" << filepath << "'\n";

    // Record end time and show duration if needed
    if(showDuration){

        end = chrono::steady_clock::now();
        chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        if(duration < 1000){
            cout << "[Finished parsing .obj file in "  << duration << " ms]";
        }
        else{
            cout << "[Finished parsing .obj file in "  << static_cast<float>(duration) / 1000 << " s]";
        }
        cout << "\n\n";
        
    }
}




void ObjParser::serialize(Octree *octree, const std::string& filepath, const bool showDuration) {

    chrono::steady_clock::time_point start, end;
    std::ofstream file(filepath);

    if(!file.is_open()){
        throw runtime_error("Output obj file '" + filepath + "' cannot be opened.");
    }
    
    // Record start time if needed
    if(showDuration){
        start = chrono::steady_clock::now();
    }
    
    cout << "Serializing octree into a mesh file with " << octree->getVerticesNum() << " vertices and " << octree->getFacesNum() << " faces...\n";

    auto verticeSerializerCallback = [](OctreeNode *currNode, int _, vector<Vector3>* __, ofstream *file){

        (void) _, (void) __; // Due to convention of the genericThreadWorker class template, we will have these two unused variable

        if(currNode->getType() == OctreeNodeType::OCTREE_FILLED_LEAF){

            AABB &boundingBox = currNode->getBoundingBox();
            stringstream ss;

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

            ss << "v " << v0.x << ' ' << v0.y << ' ' << v0.z << '\n';
            ss << "v " << v1.x << ' ' << v1.y << ' ' << v1.z << '\n';
            ss << "v " << v2.x << ' ' << v2.y << ' ' << v2.z << '\n';
            ss << "v " << v3.x << ' ' << v3.y << ' ' << v3.z << '\n';
            ss << "v " << v4.x << ' ' << v4.y << ' ' << v4.z << '\n';
            ss << "v " << v5.x << ' ' << v5.y << ' ' << v5.z << '\n';
            ss << "v " << v6.x << ' ' << v6.y << ' ' << v6.z << '\n';
            ss << "v " << v7.x << ' ' << v7.y << ' ' << v7.z << '\n';

            ss << '\n';

            OctreeContext::flushSerializerString(file, ss);
        }

        for(int i = 0; i < 8; i++){
            OctreeNode *child = currNode->getChildren(i);
            if(child) OctreeContext::pushTask(currNode->getChildren(i), 0, nullptr);  // We only need the node pointer information
        }

        return false;

    };

    OctreeContext::reset();
    OctreeContext::pushTask(octree->getRoot(), 0, nullptr);

    ofstream *stream = new ofstream(filepath);

    int threadsCnt = BuildConfig::maxThreadToUse;
    thread threads[threadsCnt];

    (*stream) << "# Voxelize mesh's vertices (total: " << octree->getVerticesNum() << " vertices)\n\n";
    for(int i = 0; i < threadsCnt; i++){

        threads[i] = thread(
            OctreeContext::genericThreadWorker<decltype(verticeSerializerCallback), ofstream*>,
            verticeSerializerCallback,
            stream
        );

    }
    for(int i = 0; i < threadsCnt; i++) threads[i].join();

    
    auto faceSerializerCallback = [](ofstream *file, int *verticeTracker, int totalVertices){

            stringstream ss;
            int numVertices = OctreeContext::getSerializerVertice(verticeTracker);

            while(numVertices < totalVertices){

                // Front face
                ss << "f " << numVertices + 2 << ' ' << numVertices + 4 << ' ' << numVertices + 1 << '\n';
                ss << "f " << numVertices + 2 << ' ' << numVertices + 4 << ' ' << numVertices + 3 << '\n';
                
                // Right face
                ss << "f " << numVertices + 3 << ' ' << numVertices + 6 << ' ' << numVertices + 2 << '\n';
                ss << "f " << numVertices + 3 << ' ' << numVertices + 6 << ' ' << numVertices + 7 << '\n';

                // Back face
                ss << "f " << numVertices + 5 << ' ' << numVertices + 7 << ' ' << numVertices + 6 << '\n';
                ss << "f " << numVertices + 5 << ' ' << numVertices + 7 << ' ' << numVertices + 8 << '\n';

                // Left face
                ss << "f " << numVertices + 1 << ' ' << numVertices + 8 << ' ' << numVertices + 5 << '\n';
                ss << "f " << numVertices + 1 << ' ' << numVertices + 8 << ' ' << numVertices + 4 << '\n';

                // Top face
                ss << "f " << numVertices + 3 << ' ' << numVertices + 8 << ' ' << numVertices + 4 << '\n';
                ss << "f " << numVertices + 3 << ' ' << numVertices + 8 << ' ' << numVertices + 7 << '\n';

                // Bottom face
                ss << "f " << numVertices + 1 << ' ' << numVertices + 6 << ' ' << numVertices + 2 << '\n';
                ss << "f " << numVertices + 1 << ' ' << numVertices + 6 << ' ' << numVertices + 5 << '\n';

                ss << '\n';
                OctreeContext::flushSerializerString(file, ss);
                numVertices = OctreeContext::getSerializerVertice(verticeTracker);
            }
    };


    int *verticeTracker = new int(0);
    (*stream) << "\n\n# Voxelize mesh's faces (total: " << octree->getFacesNum() << " faces)\n";

    for(int i = 0; i < threadsCnt; i++){
        threads[i] = thread(
            faceSerializerCallback,
            stream,
            verticeTracker,
            octree->getVerticesNum()
        );
    }
    for(int i = 0; i < threadsCnt; i++) threads[i].join();
    delete verticeTracker;
    delete stream;


    cout << "Voxelized mesh successfully saved at '" << filepath << "'\n";

    // Record end time and show duration if needed
    if(showDuration){

        end = chrono::steady_clock::now();
        chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        if(duration < 1000){
            cout << "[Finished serializing octree into .obj file in "  << duration << " ms ";
            cout << '(' << threadsCnt << " threads used)]";
        }
        else{
            cout << "[Finished serializing octree into .obj file in "  << static_cast<float>(duration) / 1000 << " s ";
            cout << '(' << threadsCnt << " threads used)]";
        }
        cout << "\n\n";
        
    }
}





void ObjParser::serializeSpaceOptimized(Octree *octree, const std::string& filepath, const bool showDuration) {

    chrono::steady_clock::time_point start, end;
    std::ofstream file(filepath);

    if(!file.is_open()){
        throw runtime_error("Output obj file '" + filepath + "' cannot be opened.");
    }
    
    // Record start time if needed
    if(showDuration){
        start = chrono::steady_clock::now();
    }
    
    cout << "Serializing octree into a mesh file with " << octree->getVerticesNum() << " vertices and " << octree->getFacesNum() << " faces...\n";

    auto verticeSerializerCallback = [](ofstream *file, Octree *octree, int *tempVerticesTracker){
        
        map<Vector3, uint32_t> &uniqueVerticesMap = octree->getVerticesMap();
        int totalVertices = octree->getVerticesNum();

        bool done = false;
        while(!done){
            
            for(auto &[vec, idx] : uniqueVerticesMap){

                unique_lock<mutex> tempLock(octree->getMutex());

                if((*tempVerticesTracker) == totalVertices){
                    done = true;
                    break;
                }

                if(idx == 0){
                    idx = ++(*tempVerticesTracker);
                    (*file) << "v " << vec.x << ' ' << vec.y << ' ' << vec.z << '\n';

                    if((*tempVerticesTracker) == totalVertices){
                        done = true;
                        break;
                    }
                }

            }

        }

        return false;
    };


    auto faceSerializerCallback = [](ofstream *file, Octree *octree, set<tuple<uint32_t, uint32_t, uint32_t>> &uniqueFacesSet){

        while(true){

            unique_lock<mutex> tempLock(octree->getMutex());
            if(uniqueFacesSet.empty()) break;
            
            tuple<int, int, int> faceIdx = *uniqueFacesSet.begin();
            uniqueFacesSet.erase(uniqueFacesSet.begin());
            (*file) << "f " << get<0>(faceIdx) << ' ' << get<1>(faceIdx) << ' ' << get<2>(faceIdx) << '\n';
        }

    };


    OctreeContext::reset();

    ofstream *outfile = new ofstream(filepath);

    int threadsCnt = BuildConfig::maxThreadToUse;
    thread threads[threadsCnt];

    (*outfile) << "# Voxelized mesh's vertices (total: " << octree->getVerticesNum() << " vertices)\n";

    int *tempVerticesTracker = new int(0);
    for(int i = 0; i < threadsCnt; i++){

        threads[i] = thread(
            verticeSerializerCallback,
            outfile,
            octree,
            tempVerticesTracker
        );

    }
    for(int i = 0; i < threadsCnt; i++) threads[i].join();
    delete tempVerticesTracker;


    octree->deduplicateFaces();
    
    (*outfile) << "\n\n# Voxelized mesh's faces (total: " << octree->getFacesNum() << " faces)\n";

    auto *facesSetCopy = new set<tuple<uint32_t, uint32_t, uint32_t>> (octree->getFacesSet());
    for(int i = 0; i < threadsCnt; i++){
        threads[i] = thread(
            faceSerializerCallback,
            outfile,
            octree,
            ref(*facesSetCopy)
        );
    }
    for(int i = 0; i < threadsCnt; i++){
        threads[i].join();
    }
    delete outfile;


    cout << "Voxelized mesh successfully saved at '" << filepath << "'\n";

    // Record end time and show duration if needed
    if(showDuration){

        end = chrono::steady_clock::now();
        chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        if(duration < 1000){
            cout << "[Finished serializing octree into .obj file in "  << duration << " ms ";
            cout << '(' << threadsCnt << " threads used)]";
        }
        else{
            cout << "[Finished serializing octree into .obj file in "  << static_cast<float>(duration) / 1000 << " s ";
            cout << '(' << threadsCnt << " threads used)]";
        }
        cout << "\n\n";
        
    }
}
