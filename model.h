// citation: https://github.com/ssloy/tinyrenderer/tree/f6fecb7ad493264ecd15e230411bfb1cca539a12
#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:
    std::vector<Vec3f> verts_;
    std::vector<Vec3f> normals_;
    // std::vector<std::vector<int> > faces_;
    std::vector<Vec2f> textureUVs_;
    std::vector<int> facet_vrt = {}; //  ┐ per-triangle indices in the above arrays,
    std::vector<int> facet_nrm = {}; //  │ the size is supposed to be
    std::vector<int> facet_tex = {}; //  ┘ nfaces()*3
    TGAImage normalmap_;
    TGAImage texturemap_;
    TGAImage specmap_;
public:
    Model(const std::string filename);
    ~Model();
    int nverts();
    int nfaces();
    int nnormals();
    int ntextureUVs();
    Vec3f vert(int i);
    Vec3f vert(int face, int i);
    Vec3f normal(int i);
    Vec3f normal(int face, int i);
    Vec3f normal(Vec2f uv);
    Vec2f textureUV(int i);
    Vec2f textureUV(int face, int i);
    TGAColor texture(Vec2f uv);
    TGAColor specular(Vec2f uv);
    void setVert(int i, Vec3f v);
    void setNormal(int i, Vec3f n);
    std::vector<int> face(int idx);
    void sortFaces();
};

#endif //__MODEL_H__
