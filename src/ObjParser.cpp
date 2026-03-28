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


    if(BuildConfig::minimizeFileSize) octree->deduplicateFaces();
    
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
