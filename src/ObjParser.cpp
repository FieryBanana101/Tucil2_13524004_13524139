#include "ObjParser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

vector<Triangle> ObjParser::parse(const string& filename) {
    vector<Triangle> triangles;
    vector<Vector3> vertices;
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        throw runtime_error(filename + " cannot be opened.");
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
            triangles.push_back(Triangle(vertices[i-1], vertices[j-1], vertices[k-1]));
        }
    }

    if (triangles.empty()) {
        throw runtime_error(filename + " does not contain any triangles.");
    }
    return triangles;

}