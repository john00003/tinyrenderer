// citation: https://github.com/ssloy/tinyrenderer/tree/f6fecb7ad493264ecd15e230411bfb1cca539a12

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <numeric>
#include "model.h"

#include <algorithm>

Model::Model(const std::string filename) : verts_() {
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
            // std::vector<int> f;
            int v, n, t; // vertex, texture, and normal
            // TODO: we no longer can support both types of input
            // the format "1193/1240/1193 1180/..." contains more data than the format "f 16504 16660 16659"
            // the first format maps faces to the respective index of the vertex, the normal, and the texture arrays
            // i.e., we assume going forwards that our .obj files contain vertex texture and normal indices

            iss >> trash;
            while (iss >> v >> trash >> t >> trash >> n) {
                facet_vrt.push_back(--v);
                facet_tex.push_back(--t);
                facet_nrm.push_back(--n);
            }
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash;
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v.raw[i];
            normals_.push_back(v);
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash;
            iss >> trash;
            Vec2f uv;
            for (int i=0;i<2;i++) iss >> uv.raw[i];
            textureUVs_.push_back({uv.x, 1-uv.y});
        }
    }
    auto load_texture = [&filename](const std::string suffix, TGAImage &img) {
        size_t dot = filename.find_last_of(".");
        if (dot==std::string::npos) return;
        std::string texfile = filename.substr(0,dot) + suffix;
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
    };
    load_texture("_nm.tga", normalmap_);
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)facet_vrt.size() / 3;
}

int Model::nnormals() {
    return (int)normals_.size();
}

int Model::ntextureUVs() {
    return (int)textureUVs_.size();
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec3f Model::vert(int face, int i) {
    return verts_[facet_vrt[face*3+i]];
}

Vec3f Model::normal(int i) {
    return normals_[i];
}

Vec3f Model::normal(int face, int i) {
    return normals_[facet_nrm[face*3+i]];
}

Vec3f Model::normal(Vec2f uv){
    TGAColor c = normalmap_.get(uv.u*normalmap_.get_width(), uv.v*normalmap_.get_height());
    return Vec3f{(float)c.raw[2], (float)c.raw[1], (float)c.raw[0]}*2.*(1/255.) - Vec3f{1, 1, 1};
}

// Vec3f Model::normal(Eigen::Vector2f &uv){
//     TGAColor c = normalmap_.get(uv(0)*normalmap_.get_width(), uv(2)*normalmap_.get_height());
//     return Vec3f{(float)c.raw[2], (float)c.raw[1], (float)c.raw[0]}*2.*(1/255.) - Vec3f{1, 1, 1};
// }

Vec2f Model::textureUV(int i) {
    return textureUVs_[i];
}

Vec2f Model::textureUV(int face, int i) {
    return textureUVs_[facet_tex[face*3+i]];
}

void Model::setVert(int i, Vec3f v)
{
    verts_[i] = v;
}

void Model::setNormal(int i, Vec3f v)
{
    normals_[i] = v;
}

