#include "tgaimage.h"
#include "geometry.h"
#include <Eigen/Core>
#include <Eigen/Geometry>

void generateModelViewMatrix(Eigen::Vector3f eye, Eigen::Vector3f center, Eigen::Vector3f up);
void generatePerspectiveMatrix(float f);
void generateViewportMatrix(const int x, const int y, const int w, const int h);
void generateZBuffer(int width, int height);

struct IShader {
    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f bar) const = 0;
};

typedef Eigen::Vector4f Triangle[3]; // a triangle primitive is made of three ordered points
void rasterize(const Triangle &clip, const IShader &shader, TGAImage &framebuffer);
