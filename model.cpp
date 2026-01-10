// citation: https://github.com/ssloy/tinyrenderer/tree/f6fecb7ad493264ecd15e230411bfb1cca539a12

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    bool check = true;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v.raw[i];
            verts_.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int itrash, idx;
            // we must handle two types of input: the format "f 1193/1240/1193 1180/1227/1180 1179/1226/1179"
            // and the format "f 16504 16660 16659"
            iss >> trash;
            iss >> idx;     // read first index
            if (check)
                std::cout << idx - 1 << std::endl;
            f.push_back(idx-1); // in wavefront obj all indices start at 1, not zero
            int c = iss.peek();
            if (c == 47) // if c == "/"
            {
                iss >> trash >> itrash >> trash >> itrash;
                while (iss >> idx >> trash >> itrash >> trash >> itrash) {
                    idx--; // in wavefront obj all indices start at 1, not zero
                    f.push_back(idx);
                    if (check)
                        std::cout << idx << std::endl;
                }
            } else
            {
                while (iss >> idx)
                {
                    idx--; // in wavefront obj all indices start at 1, not zero
                    f.push_back(idx);
                    if (check)
                        std::cout << idx << std::endl;
                }
            }

            faces_.push_back(f);
            check = false;
        }
    }
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}
