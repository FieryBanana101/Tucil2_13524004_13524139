#include "ObjParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

void ObjParser::parse(const std::string& filename, vector<Vector3>& vertices, vector<Vector3>& faceIndexes) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        throw runtime_error("Input obj file " + filename + " cannot be opened.");
    }

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
        throw runtime_error(filename + " does not contain any triangles.");
    }
}



void ObjParser::serialize(const std::string& filename, const Octree *tree) {
    
    std::ofstream file(filename);

    if(!file.is_open()){
        throw runtime_error("Output obj file " + filename + " cannot be opened.");
    }

    int numFaces = 0;
    Octree::traverseRecursive(tree->getRoot(), file, numFaces);
    cout << "Successfully converted octree to .obj file with " << numFaces << " Faces\n";

}