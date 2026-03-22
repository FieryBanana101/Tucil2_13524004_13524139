#include "ObjParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>
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

    // TODO: more error handling here
    while(getline(file, line)) {
        stringstream ss(line);
        string type;
        ss >> type;
        if (type == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            vertices.push_back(Vector3(x, y, z));
        }
        else if (type == "f") {
            int i, j, k;
            ss >> i >> j >> k;
            faceIndexes.push_back(Vector3(i-1, j-1, k-1));
        }
    }

    if(faceIndexes.empty()) {
        throw runtime_error("Mesh described in '" + filepath + "' does not contain any triangles/faces.");
    }

    cout << "Successfully parsed mesh in .obj file located at '" << filepath << "'\n";

    // Record end time and show duration if needed
    if(showDuration){

        end = chrono::steady_clock::now();
        chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        string durationUnit = (duration >= 1000 ? "s" : "ms");
        if(durationUnit == "ms"){
            cout << "[Finished parsing .obj file in "  << duration << ' ' << "ms]\n\n";
        }
        else{
            cout << "[Finished parsing .obj file in "  << static_cast<float>(duration) / 1000 << ' ' << "s]\n\n";
        }
        
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
    
    int numVertices = 0, numVoxels = 0;
    octree->traverse(
    
    [&numVertices, &numVoxels, &file](OctreeNode *currNode, int currDepth){

        (void) currDepth;
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

            file << "# Octree node " << ++numVoxels << '\n';
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

    });

    cout << "Voxelized mesh successfully saved at '" << filepath << "'\n";

    // Record end time and show duration if needed
    if(showDuration){

        end = chrono::steady_clock::now();
        chrono::milliseconds::rep duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        
        string durationUnit = (duration >= 1000 ? "s" : "ms");
        if(durationUnit == "ms"){
            cout << "[Finished serializing octree into .obj file in "  << duration << ' ' << "ms]\n\n";
        }
        else{
            cout << "[Finished serializing octree into .obj file in "  << static_cast<float>(duration) / 1000 << ' ' << "s]\n\n";
        }
        
    }
}
