#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <expected>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
//#include "mymath.h"
//#include <Eigen/Dense>
#include "our_gl.h"
#include <Eigen/Core>
#include <Eigen/Geometry>

Model *model = NULL;
constexpr double c = 3.; // camera parameter
constexpr double e = 35.; //specular exponent

extern Eigen::Matrix4f ModelView, Viewport, Perspective; // "OpenGL" state matrices
extern std::vector<std::vector<float>> zbuffer;               // depth buffer

// const TGAColor white = TGAColor(255, 255, 255, 255);
// const TGAColor red = TGAColor(255, 0, 0, 255);
// const TGAColor green = TGAColor(0, 255, 0, 255);
// const TGAColor blue = TGAColor(0, 0, 255, 255);

const TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
const TGAColor green   = {  0, 255,   0, 255};
const TGAColor red     = {  0,   0, 255, 255};
const TGAColor blue    = {255, 128,  64, 255};
const TGAColor yellow  = {  0, 200, 255, 255};

const int width  = 800;
const int height = 800;

void line (int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color)
{
    bool steep = false;
    if (std::abs(y1-y0) > std::abs(x1-x0))
    {
        std::swap(y0, x0);
        std::swap(y1, x1);
        steep = true;
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    // float error = 0;
    // float derror = std::abs(dy/(float)dx); // rise over run... this is slope
    // question: can we do something cool to avoid using floats?
    // yes
    int error = 0;
    int derror = 2*std::abs(dy);  // multiply slope by 2 * dx
    int y=y0;

    for (int x=x0; x<=x1; ++x)
    {
        if (steep)
        {
            image.set(y,x,color);
        }
        else
        {
            image.set(x,y,color);
        }

        error += derror; // we add the error on the y axis incurred based on the slope of the line

        // branchless version
        y += (y1 > y0 ? 1 : -1) * (error > dx);
        error -= 2 * (dx)   * (error > dx);



    }
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
}

template<typename T>
void sortTriangleVerticesByX(std::vector<Vec2<T>> &vertices)
{
    auto glambda = [](Vec2<T> a, Vec2<T> b) { return a.x < b.x; };
    std::sort(vertices.begin(), vertices.end(), glambda);
}

void triangleWithFill(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    // idea, let's draw three lines at one
        // we iterate over the x coordinate
        // at each x coordinate, we compute the highest point above and below that a line appears

    // we draw three lines:
        // (ax,ay) to (bx,by)
        // (bx,by) to (cx,cy)
        // (cx,cy) to (ax,ay)

    Vec2i point1(ax, ay);
    Vec2i point2(bx, by);
    Vec2i point3(cx, cy);
    std::vector<Vec2i> vertices{point1, point2, point3};
    sortTriangleVerticesByX(vertices);

    // using raster algorithm we draw the edges of the triangle that start at vertices[0].x, up until vertices[1].x
    int total_width = vertices[2].x - vertices[0].x;

    if (vertices[0].y != vertices[1].y)
    {
        int dx = vertices[1].x - vertices[0].x;
        for (int x = vertices[0].x; x <= vertices[1].x; x++)
        {
            int y0 = vertices[0].y + (vertices[1].y - vertices[0].y) * (x - vertices[0].x) / dx;
            int y1 = vertices[0].y + (vertices[2].y - vertices[0].y) * (x - vertices[0].x) / total_width;
            for (int y = std::min(y0, y1); y < std::max(y0,y1); y++)
                framebuffer.set(x, y, color);
            //framebuffer.set(x, y1, green);
        }
    }

    // we continue with similar algorithm, continuing to draw the line from vertices[0].x to vertices[2].x,
    // and starting to draw the new line from vertices[1].x to vertices[2].x

    if (vertices[1].y != vertices[2].y)
    {
        int dx = vertices[2].x - vertices[1].x;
        for (int x = vertices[1].x; x <= vertices[2].x; x++)
        {
            int y0 = vertices[1].y + (vertices[2].y - vertices[1].y) * (x - vertices[1].x) / dx;
            int y1 = vertices[0].y + (vertices[2].y - vertices[0].y) * (x - vertices[0].x) / total_width;
            for (int y = std::min(y0, y1); y < std::max(y0,y1); y++)
                framebuffer.set(x, y, color);
        }
    }

}

template<typename T>
std::vector<Vec2i> computeBoundingBox(std::vector<T> &&vertices)
{
    // takes as input a vector of three vertices sorted by x coordinate
    std::vector<Vec2i> bbox;
    int minX = vertices[0].x;
    int maxX = vertices[2].x;

    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();

    for (auto v:vertices)
    {
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
        minX = std::min(minX, v.x);  // for more general routine that can take any number of input vertices we could do this
        maxX = std::max(maxX, v.x);
    }

    minY = std::clamp(minY, 0, height-1);
    maxY = std::clamp(maxY, 0, height-1);

    minX = std::clamp(minX, 0, width-1);
    maxX = std::clamp(maxX, 0, width-1);
    // minY = std::max(0, minY);
    // maxY = std::min(maxY, height-1);
    //
    // minX = std::max(0, maxX);
    // maxX = std::min(width-1, maxX);
    //
    bbox.emplace_back(minX, minY);
    bbox.emplace_back(maxX, maxY);

    return bbox;
}

template<typename T>
std::vector<Vec2i> computeBoundingBox(std::vector<Vec2<T>> &vertices, int width, int height)
{
    // takes as input a vector of three vertices sorted by x coordinate
    std::vector<Vec2i> bbox;
    T minX = vertices[0].x;
    T maxX = vertices[2].x;

    T minY = std::numeric_limits<T>::max();
    T maxY = std::numeric_limits<T>::min();

    for (auto v:vertices)
    {
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
        minX = std::min(minX, v.x);  // for more general routine that can take any number of input vertices we could do this
        maxX = std::max(maxX, v.x);
    }

    minY = std::clamp((int)minY, 0, height-1);
    maxY = std::clamp((int)maxY, 0, height-1);

    minX = std::clamp((int)minX, 0, width-1);
    maxX = std::clamp((int)maxX, 0, width-1);
    // minY = std::max(0, minY);
    // maxY = std::min(maxY, height-1);
    //
    // minX = std::max(0, maxX);
    // maxX = std::min(width-1, maxX);
    //
    bbox.emplace_back(minX, minY);
    bbox.emplace_back(maxX, maxY);

    return bbox;
}

template<typename T>
std::vector<Vec2i> computeBoundingBox(std::vector<Vec3<T>> &vertices, int width, int height)
{
    // takes as input a vector of three vertices sorted by x coordinate
    std::vector<Vec2i> bbox;
    T minX = vertices[0].x;
    T maxX = vertices[2].x;

    T minY = std::numeric_limits<T>::max();
    T maxY = std::numeric_limits<T>::min();

    for (auto v:vertices)
    {
        minY = std::min(minY, v.y);
        maxY = std::max(maxY, v.y);
        minX = std::min(minX, v.x);  // for more general routine that can take any number of input vertices we could do this
        maxX = std::max(maxX, v.x);
    }

    minY = std::clamp((int)minY, 0, height-1);
    maxY = std::clamp((int)maxY, 0, height-1);

    minX = std::clamp((int)minX, 0, width-1);
    maxX = std::clamp((int)maxX, 0, width-1);
    // minY = std::max(0, minY);
    // maxY = std::min(maxY, height-1);
    //
    // minX = std::max(0, maxX);
    // maxX = std::min(width-1, maxX);
    //
    bbox.emplace_back(minX, minY);
    bbox.emplace_back(maxX, maxY);

    return bbox;
}

bool pointInTriangle(int i, int j, std::vector<Vec2i> &vertices)
{
    // vertices are the three vertices that define a triangle, sorted by x values

    // first we check that vertices[0].x <= j <= vertices[2].x
        // if not we return false
    // then we check whether vertices[0].x <= j < vertices[1].x or if vertices[1].x <= j <= vertices[2].x
    // then we compute min and max y at j using either the lines from either vertices[0]  to vertices[1] and vertices[2] or the lines from vertices[1] to vertices[2] and vertices[0] to vertices[2]

    if (j > vertices[2].x || j < vertices[0].x)
        return false;

    int yMin;
    int yMax;

    int total_width = vertices[2].x - vertices[0].x;

    if (j < vertices[1].x)
    {
        int dx = vertices[1].x - vertices[0].x;
        int y0 = vertices[0].y + (vertices[1].y - vertices[0].y) * (j - vertices[0].x) / dx;
        int y1 = vertices[0].y + (vertices[2].y - vertices[0].y) * (j - vertices[0].x) / total_width;
        yMin = std::min(y0, y1);
        yMax = std::max(y0, y1);
    } else
    {
        int dx = vertices[2].x - vertices[1].x;
        int y0 = vertices[1].y + (vertices[2].y - vertices[1].y) * (j - vertices[1].x) / dx;
        int y1 = vertices[0].y + (vertices[2].y - vertices[0].y) * (j - vertices[0].x) / total_width;
        yMin = std::min(y0, y1);
        yMax = std::max(y0, y1);
    }

    if (i < yMin || i > yMax)
        return false;

    return true;
}

double signedTriangleArea(int ax, int ay, int bx, int by, int cx, int cy)
{
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
    //return .5 * ((ax - cx)*(by - ay) - (ax - bx)*(cy - ay));
    //return .5 * ((ax - bx) * (ay + by) + (cx - bx) * (cy + by) + (ax - cx) * (ay + cy));
}

template <typename T>
bool pointInTriangleBarycentricMethod(int px, int py, std::vector<T> &vertices, double totalArea)
{

    double alpha = signedTriangleArea(px, py, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y) / totalArea;
    double beta = signedTriangleArea(px, py, vertices[2].x, vertices[2].y, vertices[0].x, vertices[0].y) / totalArea;
    double gamma = signedTriangleArea(px, py, vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y) / totalArea;

    if (alpha<0 || beta<0 || gamma<0)
        return false;
    return true;
}

void triangleWithFillBoundingBoxMethod(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color)
{
    Vec2i point1(ax, ay);
    Vec2i point2(bx, by);
    Vec2i point3(cx, cy);
    std::vector<Vec2i> vertices{point1, point2, point3};

    double totalArea = signedTriangleArea(vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y);
    //std::cout << ax << " " << ay << " " << bx << " " << by << " " << cx << " " << cy << std::endl;
    if (totalArea < 1){
        // std::cout << "Total area is less than 1, skipping triangle" << std::endl;
        //std::cout << "Total area:  " << totalArea << std::endl;
        return;
    }
    // else{
    //     std::cout << "Total area is " << totalArea << ", drawing triangle" << std::endl;
    //     std::cout << "Colors: " << (int) color.raw[0] << ", " << (int) color.raw[1] << ", " << (int) color.raw[2] << ", " << (int) color.raw[3] << std::endl;
    // }
    //sortTriangleVerticesByX(vertices);
    std::vector<Vec2i> bbox = computeBoundingBox<int>(vertices, width, height);
    //std::cout << "bbox min x: " << bbox[0].x << " " << "bbox min y: " << bbox[0].y << " " << "bbox max x: " << bbox[1].x << " " << "bbox max y: " << bbox[1].y << std::endl;

    for (int i=bbox[0].y; i<=bbox[1].y; i++)
    {
        for (int j=bbox[0].x; j<=bbox[1].x; j++)
        {
            if (pointInTriangleBarycentricMethod(j,i, vertices, totalArea))
            {
                framebuffer.set(j, i, color);
            }
        }
    }
}

template <typename T>
std::expected<TGAColor, std::string> pointInTriangleBarycentricMethodWithColorInterpolation(int px, int py, std::vector<T> &vertices, std::vector<TGAColor> &colors, double totalArea)
{
    double alpha = signedTriangleArea(px, py, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y) / totalArea;
    double beta = signedTriangleArea(px, py, vertices[2].x, vertices[2].y, vertices[0].x, vertices[0].y) / totalArea;
    double gamma = signedTriangleArea(px, py, vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y) / totalArea;

    if (alpha<0 || beta<0 || gamma<0)
        return std::unexpected("Outside of triangle");;
//{std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255}
    //return TGAColor{alpha*colors[0], beta*colors[1], gamma*colors[2], colors[3]};
    int B = (int)(alpha*colors[0].b + beta*colors[1].b + gamma*colors[2].b) % 255;
    int G = (int)(alpha*colors[0].g + beta*colors[1].g + gamma*colors[2].g) % 255;
    int R = (int)(alpha*colors[0].r + beta*colors[1].r + gamma*colors[2].r) % 255;
    int A = (int)(alpha*colors[0].a + beta*colors[1].a + gamma*colors[2].a) % 255;
    return TGAColor{B,G,R,A};
}

void triangleWithLinearInterpolationOverBarycentric(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, std::vector<TGAColor> &colors)
{
    Vec2i point1(ax, ay);
    Vec2i point2(bx, by);
    Vec2i point3(cx, cy);
    std::vector<Vec2i> vertices{point1, point2, point3};

    double totalArea = signedTriangleArea(vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y);
    //std::cout << ax << " " << ay << " " << bx << " " << by << " " << cx << " " << cy << std::endl;
    // if (totalArea < 1){
    //     // std::cout << "Total area is less than 1, skipping triangle" << std::endl;
    //     //std::cout << "Total area:  " << totalArea << std::endl;
    //     return;
    // }
    // else{
    //     std::cout << "Total area is " << totalArea << ", drawing triangle" << std::endl;
    //     std::cout << "Colors: " << (int) color.raw[0] << ", " << (int) color.raw[1] << ", " << (int) color.raw[2] << ", " << (int) color.raw[3] << std::endl;
    // }
    //sortTriangleVerticesByX(vertices);
    std::vector<Vec2i> bbox = computeBoundingBox(vertices, width, height);
    //std::cout << "bbox min x: " << bbox[0].x << " " << "bbox min y: " << bbox[0].y << " " << "bbox max x: " << bbox[1].x << " " << "bbox max y: " << bbox[1].y << std::endl;

    for (int i=bbox[0].y; i<=bbox[1].y; i++)
    {
        for (int j=bbox[0].x; j<=bbox[1].x; j++)
        {
            auto result = pointInTriangleBarycentricMethodWithColorInterpolation(j,i, vertices, colors, totalArea);
            if (result)
            {
                //std::cout << "drawing at i: " << i << ", " << j << std::endl;
                framebuffer.set(j, i, *result);
            }
        }
    }
}

template <typename T>
std::expected<T, std::string> pointInTriangleBarycentricMethodWithDepthInterpolation(int px, int py, std::vector<Vec3<T>> &vertices, double totalArea)
{
    double alpha = signedTriangleArea(px, py, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y) / totalArea;
    double beta = signedTriangleArea(px, py, vertices[2].x, vertices[2].y, vertices[0].x, vertices[0].y) / totalArea;
    double gamma = signedTriangleArea(px, py, vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y) / totalArea;

    if (alpha<0 || beta<0 || gamma<0)
        return std::unexpected("Outside of triangle");;
    //{std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255}
    //return TGAColor{alpha*colors[0], beta*colors[1], gamma*colors[2], colors[3]};
    // int B = (int)(alpha*colors[0].b + beta*colors[1].b + gamma*colors[2].b) % 255;
    // int G = (int)(alpha*colors[0].g + beta*colors[1].g + gamma*colors[2].g) % 255;
    // int R = (int)(alpha*colors[0].r + beta*colors[1].r + gamma*colors[2].r) % 255;
    // int A = (int)(alpha*colors[0].a + beta*colors[1].a + gamma*colors[2].a) % 255;
    T thisZ = alpha*vertices[0].z + beta*vertices[1].z + gamma*vertices[2].z;
    // std::cout << "values in pointInTriangle: " << std::endl;
    // std::cout << thisZ << std::endl;
    // std::cout << alpha << " " << beta << " " << gamma << std::endl;
    // std::cout << vertices[0].z << " " << vertices[1].z << " " << vertices[2].z << std::endl;
    return thisZ;
}

template <typename T, typename U>
std::expected<T, std::string> pointInTriangleBarycentricMethodWithDepthInterpolation(int px, int py, std::vector<Vec3<U>> &vertices, std::vector<Vec3<T>> &verticesNonProjected, double totalArea)
{
    double alpha = signedTriangleArea(px, py, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y) / totalArea;
    double beta = signedTriangleArea(px, py, vertices[2].x, vertices[2].y, vertices[0].x, vertices[0].y) / totalArea;
    double gamma = signedTriangleArea(px, py, vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y) / totalArea;

    if (alpha<0 || beta<0 || gamma<0)
        return std::unexpected("Outside of triangle");;
    //{std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255}
    //return TGAColor{alpha*colors[0], beta*colors[1], gamma*colors[2], colors[3]};
    // int B = (int)(alpha*colors[0].b + beta*colors[1].b + gamma*colors[2].b) % 255;
    // int G = (int)(alpha*colors[0].g + beta*colors[1].g + gamma*colors[2].g) % 255;
    // int R = (int)(alpha*colors[0].r + beta*colors[1].r + gamma*colors[2].r) % 255;
    // int A = (int)(alpha*colors[0].a + beta*colors[1].a + gamma*colors[2].a) % 255;
    T thisZ = alpha*verticesNonProjected[0].z + beta*verticesNonProjected[1].z + gamma*verticesNonProjected[2].z;
    // std::cout << "values in pointInTriangle: " << std::endl;
    // std::cout << thisZ << std::endl;
    // std::cout << alpha << " " << beta << " " << gamma << std::endl;
    // std::cout << vertices[0].z << " " << vertices[1].z << " " << vertices[2].z << std::endl;
    return thisZ;
}


std::pair<int, int> convertVec3fToXY(Vec3f &&v, int width, int height)
{
    return std::make_pair((v.x+1.)*width/2., (v.y+1.)*height/2.);
}

Vec3i convertVec3fToVec3i(Vec3f v, int width, int height)
{
    //std::cout << "values in converter: " << v.x << " " << v.y << " " << v.z << std::endl;
    //std::cout << "width: " << width << " height: " << height << std::endl;
    //std::cout << (v.x+1.)*width/2. << " " << (v.y+1.)*height/2. << " " << (v.z+1.)*255/2. << std::endl;
    return Vec3i{(v.x+1.)*width/2., (v.y+1.)*height/2., (v.z+1.)*255/2.};
}

std::vector<Vec3i> transformVertices(std::vector<Vec3f> v, int width, int height)
{
    std::vector<Vec3i> result;
    for (int i = 0; i < v.size(); i++)
    {
        result.push_back(convertVec3fToVec3i(v[i],width,height));
    }
    return result;
}

Eigen::Matrix3f createRotationMatrix(float x_theta, float y_theta, float z_theta)
{
    // convert each angle to radians and form transformation matrix
    return (Eigen::AngleAxisf(EIGEN_PI / 180 * x_theta, Eigen::Vector3f::UnitX()) * Eigen::AngleAxisf(EIGEN_PI / 180 * y_theta, Eigen::Vector3f::UnitY()) * Eigen::AngleAxisf(EIGEN_PI / 180 * z_theta, Eigen::Vector3f::UnitZ())).toRotationMatrix();;
}

void rotateModel(float x_theta, float y_theta, float z_theta)
{
    Eigen::Matrix3f rotMatrix = createRotationMatrix(x_theta, y_theta, z_theta);
    for (int i=0; i<model->nverts(); i++)
    {
        Vec3f vert = model->vert(i);
        // if (vert.x < 0 || vert.y < 0 || vert.z < 0)
        //     std::cout << "one of these under zero before" << std::endl;
        //std::cout << "vertices: " << std::endl;
        //std::cout << vert << std::endl;
        Eigen::Vector3f v(vert.x, vert.y, vert.z);
        v = rotMatrix * v;
        vert.x = v[0];
        vert.y = v[1];
        vert.z = v[2];
        // if (vert.x < 0 || vert.y < 0 || vert.z < 0)
        //     std::cout << "one of these under zero" << std::endl;
        //std::cout << "new vertices: " << std::endl;
        //std::cout << vert << std::endl;
        //model->vert(i) = vert;
        model->setVert(i, vert);
        //model->verts_[i] = vert;
    }
}

void rotateModel(float x_theta, float y_theta, float z_theta, std::vector<Vec3i> &integerVertices)
{
    Eigen::Matrix3f rotMatrix = createRotationMatrix(x_theta, y_theta, z_theta);
    for (int i=0; i<integerVertices.size(); i++)
    {
        Vec3i vert = integerVertices[i];
        if (vert.x < 0 || vert.y < 0 || vert.z < 0)
            std::cout << "one of these under zero before" << std::endl;
        //std::cout << "vertices: " << std::endl;
        //std::cout << vert << std::endl;
        Eigen::Vector3f v(vert.x, vert.y, vert.z);
        v = rotMatrix * v;
        vert.x = v[0];
        vert.y = v[1];
        vert.z = v[2];
        if (vert.x < 0 || vert.y < 0 || vert.z < 0)
            std::cout << "one of these under zero" << std::endl;
        //std::cout << "new vertices: " << std::endl;
        //std::cout << vert << std::endl;
        //model->vert(i) = vert;
        // model->setVert(i, vert);
        integerVertices[i] = vert;
        //model->verts_[i] = vert;
    }
}

// projects all vertices in model using camera parameters
void projectModel()
{
    for (int i=0; i<model->nverts(); i++)
    {
        Vec3f vert = model->vert(i);
        //std::cout << vert.x << " " << vert.y << " " << vert.z << std::endl;
        if (std::abs(vert.x) > 1 || std::abs(vert.y) > 1 || std::abs(vert.z) > 1)
            std::cout << "greater than one badness BEFORE" << std::endl;
        if (vert.z/c > 1.){
            std::cout << "greater than one" << std::endl;
        }

        // Apply perspective projection: only x and y are divided by (1 - z/c)
        // z coordinate should not be transformed the same way
        float perspective_factor = 1./(1.-vert.z/c);
        vert.x = vert.x * perspective_factor;
        vert.y = vert.y * perspective_factor;
        // vert.z stays unchanged to maintain depth ordering in [-1, 1] range

        // if (std::abs(vert.x) > 1 || std::abs(vert.y) > 1 || std::abs(vert.z) > 1) {
        //     std::cout << "Vertex " << i << " exceeds bounds: x=" << vert.x << " y=" << vert.y << " z=" << vert.z
        //               << " (factor=" << perspective_factor << ")" << std::endl;
        // }
        //std::cout << vert.x << " " << vert.y << " " << vert.z << std::endl;
        model->setVert(i, vert);
    }
}

void projectModel(std::vector<Vec3i> &integerVertices)
{
    for (int i=0; i<integerVertices.size(); i++)
    {
        Vec3i vert = integerVertices[i];
        //std::cout << vert.x << " " << vert.y << " " << vert.z << std::endl;
        if (vert.z/c > 1.){
            std::cout << "greater than one" << std::endl;
        }

        // Apply perspective projection: only x and y are divided by (1 - z/c)
        // z coordinate should not be transformed the same way
        float perspective_factor = 1./(1.-vert.z/c);
        vert.x = vert.x * perspective_factor;
        vert.y = vert.y * perspective_factor;
        // vert.z stays unchanged to maintain depth ordering

        //std::cout << vert.x << " " << vert.y << " " << vert.z << std::endl;
        // model->setVert(i, vert);
        integerVertices[i] = vert;
    }
}

void convertDepthBufferToImage(std::vector<std::vector<float>>& depthBuffer, TGAImage &depthBufferImage, const std::string& name)
{
    float minDepth = std::numeric_limits<float>::max();
    float maxDepth = -std::numeric_limits<float>::max();

    for (int i=0; i<depthBuffer.size(); i++)
        for (int j=0; j<depthBuffer[i].size(); j++)
        {
            minDepth = std::min(minDepth, depthBuffer[i][j]);
            maxDepth = std::max(maxDepth, depthBuffer[i][j]);
        }

    if (maxDepth == minDepth){
        std::cout << "WARNING: no depth" << std::endl;
        return;
    }

    // maxDepth = 255
    // minDepth = 1
    // depthBuffer[i][j] = (depthBuffer[i][j] - minDepth) / (maxDepth-minDepth) * 254 + 1
    for (int i=0; i<depthBuffer.size(); i++)
        for (int j=0; j<depthBuffer[i].size(); j++)
            depthBufferImage.set(j, i, TGAColor((int)((depthBuffer[i][j] - minDepth) / (maxDepth - minDepth) * 254 + 1), 1)); // TODO: finish scaling between min and max and [0,255] and the write to bufferw


    depthBufferImage.flip_vertically();
    depthBufferImage.write_tga_file(name.c_str());
}

Vec3f generateVec3fFromHomogenous(Eigen::Vector4f &v)
{
    return Vec3f{v(0) / v(3), v(1) / v(3), v(2) / v(3)};
}

Eigen::Vector4f generateHomogeneousVector(Vec3f &v)
{
    return Eigen::Vector4f{v.x, v.y, v.z, 1};
}


void composeTransformations(std::vector<Vec3i> &integerVertices)
{
    for (int i=0; i<integerVertices.size(); i++)
    {
        Vec3i vert = integerVertices[i];
        //std::cout << vert.x << " " << vert.y << " " << vert.z << std::endl;
        if (vert.z/c > 1.){
            std::cout << "greater than one" << std::endl;
        }

        // Apply perspective projection: only x and y are divided by (1 - z/c)
        // z coordinate should not be transformed the same way
        float perspective_factor = 1./(1.-vert.z/c);
        vert.x = vert.x * perspective_factor;
        vert.y = vert.y * perspective_factor;
        // vert.z stays unchanged to maintain depth ordering

        //std::cout << vert.x << " " << vert.y << " " << vert.z << std::endl;
        // model->setVert(i, vert);
        integerVertices[i] = vert;
    }
}

struct RandomShader : IShader {
    TGAColor color = {};
    // Eigen::Vector3f tri[3];  // triangle in eye coordinates
    Eigen::Vector2f uvs[3];  // triangle in uv coordinates on 2D texture map
    Eigen::Vector4f normals[3]; // normal at each vertex
    Eigen::Vector4f tri[3]; // triangle in view coordinates
    Eigen::Vector3f l;

    RandomShader(const Eigen::Vector3f light){
        l = ((ModelView * Eigen::Vector4f(light(0), light(1), light(2), 0.)).head(3)).normalized();
    }

    virtual Eigen::Vector4f vertex(const int face, const int vert) {
        Vec3f vtemp = model->vert(face, vert);
        Vec2f uvtemp = model->textureUV(face, vert);
        Eigen::Vector3f v(vtemp.x, vtemp.y, vtemp.z); // current vertex in object coordinates
        // Eigen::Vector3f v = model.vert(face, vert);                          
        Eigen::Vector4f gl_Position = ModelView * Eigen::Vector4f(v(0), v(1), v(2), 1.);
        Eigen::Vector2f uv(uvtemp.u, uvtemp.v);
        uvs[vert] = uv;
        Vec3f ntemp = model->normal(face,vert);
        Eigen::Vector4f temp(ntemp.x, ntemp.y, ntemp.z, 0);
        normals[vert] = ModelView.inverse().transpose() * temp;
        tri[vert] = gl_Position;
        // normals[vert] = (ModelView.inverse().transpose()*Eigen::Vector4f(ntemp.x, ntemp.y, ntemp.z, 0)).head(3);
        // tri[vert] = Eigen::Vector3f(gl_Position(0),gl_Position(1),gl_Position(2));                            // in eye coordinates
        // tri[vert] = gl_Position.head(3);                            // in eye coordinates
        return Perspective * gl_Position;                         // in clip coordinates
    }

    virtual std::pair<bool,TGAColor> fragmentOld(const Eigen::Vector3f bar) const {
        return {false, color};                                    // do not discard the pixel
    }

    // computes grayscale intensity as combination of ambient, diffuse, and specular terms
    // takes as input the current position on the face being rasterized, the camera position, and the position of the sun/lighting in the scene
    // TODO: when we were trying to compute difference between position and camera, light, etc., we were accidentally passing barycentric coordinates instead of position
    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f& barycentric, const Eigen::Vector3f& camera, const Eigen::Vector3f& light) const {
        float ambientMultiplier = 0.5;
        // float diffMultiplier = 0.6;
        // float specMultiplier = 0.9;
        //
        // Eigen::Matrix<float, 2, 4> E(tri[1] - tri[0], tri[2] - tri[0]);
        Eigen::Matrix<float,2,4> E;
        E.row(0) = tri[1] - tri[0];
        E.row(1) = tri[2] - tri[0];
        // Eigen::Matrix<float, 2, 2> U(uvs[1] - uvs[0], uvs[2] - uvs[0]);
        Eigen::Matrix<float,2,2> U;
        U.row(0) = uvs[1] - uvs[0];
        U.row(1) = uvs[2] - uvs[0];

        Eigen::Matrix<float, 2, 4> tb = U.inverse() * E;
        Eigen::Matrix4f D;
        D.row(0) = tb.row(0).normalized();
        D.row(1) = tb.row(1).normalized();
        D.row(2) = (barycentric[0]*normals[0] + barycentric[1] * normals[1] + barycentric[2] * normals[2]).normalized();
        D.row(3) << 0, 0, 0, 1;

        float ambient = ambientMultiplier;

        Eigen::Vector2f uv = barycentric[0] * uvs[0] + barycentric[1] * uvs[1] + barycentric[2] * uvs[2];

        // compute specular multiplier
        float specMultiplier = (float)model->specular(Vec2f{uv(0), uv(1)}).raw[0] / (float)255.;

        // comput base color
        TGAColor texture = model->texture(Vec2f{uv(0), uv(1)});
        
        // compute normal to the surface
        // Vec3f ntemp = model->normal(Vec2f{uv(0), uv(1)});
        // Eigen::Vector3f normal = (ModelView.inverse().transpose() * Eigen::Vector4f(ntemp.x, ntemp.y, ntemp.z, 0)).head(3).normalized();

        Vec4f ntemp = model->normal(Vec2f{uv(0), uv(1)});
        Eigen::Vector3f normal = (D.transpose() * Eigen::Vector4f(ntemp.x, ntemp.y, ntemp.z, ntemp.w)).normalized().head(3);
        // std::cout << normal << std::endl;
        // Eigen::Vector3f normal = (tri[1] - tri[0]).cross(tri[2] - tri[0]).normalized();
        // Eigen::Vector3f normal = (barycentric[0] * normals[0] + barycentric[1] * normals[1] + barycentric[2] * normals[2]).normalized();

        // compute angle between normal and unit vector facing light source
        float cosa = normal.dot(l);
        // float diffuse = std::max((float)0., cosa)*diffMultiplier;
        float diffuse = std::max((float)0., cosa);
        std::cout << "diffuse: " << diffuse << std::endl;
        // float diffuse = std::clamp(cosa, (float)-1, (float)1)*diffMultiplier;

        // compute unit vector of reflected light across normal
        Eigen::Vector3f reflection = (2 * normal * cosa - l).normalized();

        // compute unit vector of direction to camera
        float cosb = reflection[2];

        // compute specular term
        // float specular = (std::pow(std::max((float)0., cosb), e))*specMultiplier;
        float specular = 3.*model->specular(Vec2f{uv(0), uv(1)}).raw[0]/255. * std::pow(std::max(cosb, (float)0.), 35);

        // float intensityMultiplier = ambient + diffuse + specular;
        // intensityMultiplier = std::min(intensityMultiplier, (float)1.);
        // float intensity = 255 * intensityMultiplier;

        // float intensityMultiplier = specular;
        // intensityMultiplier = std::min(intensityMultiplier, (float)1.);
        // float intensity = 255 * intensityMultiplier;
        // // std::cout << "intensity: " << intensity << std::endl;
        // float textureMultiplier = diffuse + ambient;
        // textureMultiplier = std::min(textureMultiplier, (float)1.);
        // TGAColor fragmentColor = { static_cast<unsigned char>(std::min(texture.raw[2]*textureMultiplier + intensity, (float)255.)),
        //                             static_cast<unsigned char>(std::min(texture.raw[1]*textureMultiplier + intensity, (float)255.)),
        //                             static_cast<unsigned char>(std::min(texture.raw[0]*textureMultiplier + intensity, (float)255.)),
        //                             255};
        float textureMultiplier = diffuse + ambient + specular;
        TGAColor fragmentColor = { static_cast<unsigned char>(std::min(texture.raw[2]*textureMultiplier, (float)255.)),
                                    static_cast<unsigned char>(std::min(texture.raw[1]*textureMultiplier, (float)255.)),
                                    static_cast<unsigned char>(std::min(texture.raw[0]*textureMultiplier, (float)255.)),
                                    255};
        // TGAColor fragmentColor = { static_cast<unsigned char>(std::min(std::max(texture.raw[2] + intensity, (float)0), (float)255.)),
        //                             static_cast<unsigned char>(std::min(std::max(texture.raw[1] + intensity, (float)0), (float)255.)),
        //                             static_cast<unsigned char>(std::min(std::max(texture.raw[0] + intensity, (float)0), (float)255.)),
                                    // 255 };
        // std::cout << (int)fragmentColor.raw[0] << (int)fragmentColor.raw[1] << (int)fragmentColor.raw[2] << std::endl;
        // TGAColor fragmentColor = { static_cast<unsigned char>(intensity),
        //                             static_cast<unsigned char>(intensity),
        //                             static_cast<unsigned char>(intensity),
                                    // 255 };
        return {false, fragmentColor};                                    // do not discard the pixel
    }
};

// for the first pass of shadow mapping
struct BlankShader : IShader {

    BlankShader(){
    }

    virtual Eigen::Vector4f vertex(const int face, const int vert) {
        Vec3f vtemp = model->vert(face, vert);
        Eigen::Vector3f v(vtemp.x, vtemp.y, vtemp.z); // current vertex in object coordinates
        Eigen::Vector4f gl_Position = ModelView * Eigen::Vector4f(v(0), v(1), v(2), 1.);
        return Perspective * gl_Position;                         // in clip coordinates
    }

    virtual std::pair<bool,TGAColor> fragmentOld(const Eigen::Vector3f bar) const {
        return {false, TGAColor{255, 255, 255, 255}};                                    // do not discard the pixel
    }

    virtual std::pair<bool,TGAColor> fragment(const Eigen::Vector3f& barycentric, const Eigen::Vector3f& camera, const Eigen::Vector3f& light) const {
        return {false, TGAColor{255, 255, 255, 255}};                                    // do not discard the pixel
    }
};

// void transformNormals(Eigen::Matrix4f matrix){
//     for (int i=0; i<model.nnormals()){
//         Eigen::Vector4f newNormal = matrix*Eigen::Vector4f(model.normal(i)[0], model.normal(i)[1], model.normal(i)[2], 0);
//         model.setNormal(i, Vec3f(newNormal(0), newNormal(1), newNormal(2)));
//     }
// }

void drop_zbuffer(const char* filename, std::vector<std::vector<float>> &zbuffer, int width, int height) {
    TGAImage zimg(width, height, TGAImage::GRAYSCALE);
    float minz = +1000;
    float maxz = -1000;
    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            // float z = zbuffer[x+y*width];
            float z = zbuffer[y][x];
            if (z<-100) continue;
            minz = std::min(z, minz);
            maxz = std::max(z, maxz);
        }
    }
    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            // float z = zbuffer[x+y*width];
            float z = zbuffer[y][x];
            if (z<-100) continue;
            z = (z - minz)/(maxz-minz) * 255;
            zimg.set(x, y, {255, 255, z, 255}); // for grayscale images, only the blue value is used to determine intensity, so with RGBA constructor, write distance as third value
        }
    }
    zimg.flip_vertically();
    zimg.write_tga_file(filename);
}

int main(int argc, char **argv)
{
    // DRAW FOUR LINES

    // constexpr int width  = 64;
    // constexpr int height = 64;
    // TGAImage framebuffer(width, height, TGAImage::RGB);
    //
    // int ax =  7, ay =  3;
    // int bx = 12, by = 37;
    // int cx = 62, cy = 53;
    //
    // line(ax, ay, bx, by, framebuffer, blue);
    // line(cx, cy, bx, by, framebuffer, green);
    // line(cx, cy, ax, ay, framebuffer, yellow);
    // line(ax, ay, cx, cy, framebuffer, red);
    //
    // framebuffer.set(ax, ay, white);
    // framebuffer.set(bx, by, white);
    // framebuffer.set(cx, cy, white);
    //
    // framebuffer.write_tga_file("framebuffer.tga");
    // return 0;


    // RANDOM LINE DRAWING

    // constexpr int width  = 64;
    // constexpr int height = 64;
    // TGAImage framebuffer(width, height, TGAImage::RGB);
    //
    // std::srand(std::time({}));
    // for (int i=0; i<(1<<24); i++) {
    //     int ax = rand()%width, ay = rand()%height;
    //     int bx = rand()%width, by = rand()%height;
    //     line(ax, ay, bx, by, framebuffer, { rand()%255, rand()%255, rand()%255, rand()%255 });
    // }
    //
    // framebuffer.write_tga_file("framebuffer.tga");
    // return 0;



    // DRAW BETWEEN VERTICES

    // if (2==argc) {
    //     model = new Model(argv[1]);
    // } else {
    //     model = new Model("obj/stanford-bunny.obj");
    // }
    //
    // TGAImage image(800, 800, TGAImage::RGB);
    //
    // for (int i=0; i<model->nfaces(); i++)
    // {
    //     std::vector<int> face = model->face(i);
    //     for (int j=0; j<3; j++)     // we iterate through each vertex of the face, drawing a line between two vertices
    //     {
    //         Vec3f v0 = model->vert(face[j]);
    //         Vec3f v1 = model->vert(face[(j+1)%3]);
    //         // for each point we add one to ensure it is positive, divide by 2 to normalize it to be within [0,1]
    //         int x0 = (v0.x+1.)*width/2.;
    //         int y0 = (v0.y+1.)*height/2.;
    //         int x1 = (v1.x+1.)*width/2.;
    //         int y1 = (v1.y+1.)*height/2.;
    //         line(x0, y0, x1, y1, image, white);
    //     }
    // }


    // image.flip_vertically();
    // image.write_tga_file("output.tga");
    // return 0;

    
    // DRAW FILLED TRIANGLES

    // TGAImage framebuffer(width, height, TGAImage::RGB);
    // triangleWithFillBoundingBoxMethod(  7, 45, 35, 100, 45,  60, framebuffer, red);
    // triangle(120, 35, 90,   5, 45, 110, framebuffer, white);
    // triangleWithFillBoundingBoxMethod(115, 83, 80,  90, 85, 120, framebuffer, green);
    // framebuffer.write_tga_file("framebuffer.tga");
    // return 0;
    //
    //


    // DRAW FILLED TRAINGLES ON MODEL
    // if (2==argc) {
    //     model = new Model(argv[1]);
    // } else {
    //     model = new Model("../obj/diablo3_pose.obj");
    // }

    // model->sortFaces();
    // for (int i=0; i<model->nfaces(); i++)
    // {
    //     std::cout << "min Z coord: " << std::min(model->vert(model->face(i)[0]).z, std::min(model->vert(model->face(i)[1]).z, model->vert(model->face(i)[2]).z));
    // }

    // int width = 800;
    // int height = 800;

    // TGAImage image(width, height, TGAImage::RGB);
    // TGAImage depthBufferImage(width, height, TGAImage::GRAYSCALE);
    //TGAImage depthBuffer(width, height, TGAImage::GRAYSCALE);
    // std::vector<std::vector<float>> depthBuffer(height, std::vector<float>(width, std::numeric_limits<float>::lowest()));

    //initializeDepthBuffer();
    std::srand(std::time({}));

    // std::vector<Vec3i> integerVertices;
    // for (int i=0; i<model->nverts(); i++)
    // {
    //     integerVertices.push_back(convertVec3fToVec3i(model->vert(i), width, height));
    // }
    // Eigen::Vector3f eye{-1, 0, -2};
    // Eigen::Vector3f center{0, 0, 0};
    // Eigen::Vector3f up{0, 1, 0};
    // Eigen::Matrix4f perspectiveMatrix = generatePerspectiveMatrix((eye-center).norm());
    // Eigen::Matrix4f viewportMatrix = generateViewportMatrix();
    // Eigen::Matrix4f modelViewMatrix = generateModelViewMatrix(eye, center, up);

    // Eigen::Matrix4f totalMatrix = viewportMatrix * perspectiveMatrix * modelViewMatrix;
    // Eigen::Matrix4f preMatrix =  perspectiveMatrix * modelViewMatrix;
    //
    // for (int i=0; i<model->nverts(); i++)
    // {
    //     Eigen::Vector4f vert{model->vert(i).x, model->vert(i).y, model->vert(i).z, 1};
    //     std::cout << "vert before: " << vert << std::endl;
    //     vert = preMatrix * vert;
    //     std::cout << "vert after first matrix: " << vert << std::endl;
    //     vert = vert/vert(3);
    //     std::cout << "vert after scaling: " << vert << std::endl;
    //     vert = viewportMatrix * vert;
    //     std::cout << "vert after: " << vert << std::endl;
    //     vert(3) = 1;
    //
    //     model->setVert(i, generateVec3fFromHomogenous(vert));
    //     // Eigen::Vector4f vert{model->vert(i).x, model->vert(i).y, model->vert(i).z, 1};
    //     // std::cout << "vert before: " << vert << std::endl;
    //     // vert = totalMatrix * vert;
    //     // std::cout << "vert after: " << vert << std::endl;
    //     // model->setVert(i, generateVec3fFromHomogenous(vert));
    //     // model->vert(i) = generateVec3fFromHomogenous(vert);
    //     // std::cout << "vert totally after: " << model->vert(i).x << model->vert(i).y << model->vert(i).z << std::endl;
    //
    //     // model->vert(i).x = vert(0)/vert(3);
    //     // model->vert(i).y = vert(1)/vert(3);
    //     // model->vert(i).z = vert(2)/vert(3);
    //
    // }

    // rotateModel(0, 30, 0);
    // projectModel();

    // for (int i=0; i<model->nfaces(); i++)
    // {
    //     std::vector<int> face = model->face(i);
    //     // std::vector<Vec3i> vertices{integerVertices[face[0]], integerVertices[face[1]], integerVertices[face[2]]};
    //     std::vector<Vec3f> verticesf{model->vert(face[0]), model->vert(face[1]), model->vert(face[2])};
    //     std::vector<Vec3i> vertices;
    //     // std::transform(verticesf.begin(), verticesf.end(),
    //     //            std::back_inserter(vertices),
    //     //            [](Vec3f v) {
    //     //                return Vec3i{static_cast<int>(v.x), static_cast<int>(v.y), static_cast<int>(v.z)};
    //     //            });
    //      // std::cout << "vertices after transform: " << vertices[0] << " " << vertices[1] << " " << vertices[2] << std::endl;
    //     // auto [ax, ay] = convertVec3fToXY(model->vert(face[0]), width, height);
    //     // auto [bx, by] = convertVec3fToXY(model->vert(face[1]), width, height);
    //     // auto [cx, cy] = convertVec3fToXY(model->vert(face[2]), width, height);
    //     // TGAColor rnd;
    //     // for (int c=0; c<3; c++) rnd[c] = std::rand()%255;
    //     //std::cout << vertices[0].x << " " << vertices[0].y << " " << vertices[0].z << std::endl;
    //     //std::cout << verticesf[0].x << " " << verticesf[0].y << " " << verticesf[0].z << std::endl;
    //     const TGAColor randColor = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
    //     std::cout << "in the loop" << std::endl;
    //     triangleWithFillPerPixelPainters(verticesf, image, depthBuffer, randColor);
    // }
    // // std::cout << "out the loop" << std::endl;
    // image.flip_vertically();
    // image.write_tga_file("model_matrices.tga");

    // convertDepthBufferToImage(depthBuffer, depthBufferImage, "depth_projected.tga");
    //depthBuffer.flip_vertically();
    //depthBuffer.write_tga_file("depth_projected.tga");
    // return 0;

    // TGAImage framebuffer(width, height, TGAImage::RGB);
    // std::srand(std::time({}));
    // const TGAColor randColor1 = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
    // const TGAColor randColor2 = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
    // const TGAColor randColor3 = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
    // std::vector<TGAColor> colors = {randColor1, randColor2, randColor3};
    // triangleWithLinearInterpolationOverBarycentric(  7, 45, 35, 100, 45,  60, framebuffer, colors);
    // triangle(120, 35, 90,   5, 45, 110, framebuffer, white);
    // triangleWithLinearInterpolationOverBarycentric(115, 83, 80,  90, 85, 120, framebuffer, colors);
    // framebuffer.write_tga_file("framebuffer.tga");
    // return 0;

    // Load the model
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("../obj/diablo3_pose.obj");
    }

    const int width = 800;
    const int height = 800;

    const Eigen::Vector3f    eye(-1, 0, 2); // camera position
    const Eigen::Vector3f center( 0, 0, 0); // camera direction
    const Eigen::Vector3f     up( 0, 1, 0); // camera up vector
    const Eigen::Vector3f  light( 1, 1, 1); // position of sun/lighting

    generateModelViewMatrix(eye, center, up);                                   // build the ModelView   matrix
    generatePerspectiveMatrix((eye-center).norm());                        // build the Perspective matrix
    generateViewportMatrix(width/16, height/16, width*7/8, height*7/8); // build the Viewport    matrix
    generateZBuffer(width, height);

    // transformNormals();
    // model.transformNormals(ModelView.inverse().transpose());

    TGAImage framebuffer(width, height, TGAImage::RGB);
    RandomShader shader(light);
    for (int f=0; f<model->nfaces(); f++) {      // iterate through all facets
        // shader.color = { std::rand()%255, std::rand()%255, std::rand()%255, 255 };
        Triangle clip = { shader.vertex(f, 0),  // assemble the primitive
                          shader.vertex(f, 1),
                          shader.vertex(f, 2) };
        rasterize(clip, shader, framebuffer, eye, light);   // rasterize the primitive
    }
    framebuffer.flip_vertically();
    framebuffer.write_tga_file("framebuffer_before_postprocess.tga");
    framebuffer.flip_vertically(); // flip back so subsequent mask application aligns with zbuffer coordinates
    drop_zbuffer("zbuffer1.tga", zbuffer, width, height);

    // save zbuffer for manipulating rendered points later
    std::vector<std::vector<bool>> mask(height, std::vector<bool>(width));
    std::vector<std::vector<float>> zbuffer_copy(zbuffer);
    Eigen::Matrix4f M = (Viewport * Perspective * ModelView).inverse();

    // second pass for global shadow detection
    generateModelViewMatrix(light, center, up);                                   // build the ModelView   matrix from the perspective of the light
    generatePerspectiveMatrix((eye-center).norm());                        // build the Perspective matrix
    generateViewportMatrix(width/16, height/16, width*7/8, height*7/8); // build the Viewport    matrix
    generateZBuffer(width, height);

    TGAImage shadows(width, height, TGAImage::RGB);
    // TGAImage shadows(width, height, TGAImage::RGB, {177, 195, 209, 255});
    BlankShader blankshader{};
    for (int f=0; f<model->nfaces(); f++) {      // iterate through all facets
        // shader.color = { std::rand()%255, std::rand()%255, std::rand()%255, 255 };
        Triangle clip = { blankshader.vertex(f, 0),  // assemble the primitive
                          blankshader.vertex(f, 1),
                          blankshader.vertex(f, 2) };
        rasterize(clip, blankshader, shadows, eye, light);   // rasterize the primitive
    }
    shadows.flip_vertically();
    shadows.write_tga_file("framebuffer_shadows.tga");
    drop_zbuffer("zbuffer2.tga", zbuffer, width, height);
    Eigen::Matrix4f N = (Viewport * Perspective * ModelView);

    // identify lit fragments
    for (int i=0; i<height; i++){
        for (int j=0; j<width; j++){
            Eigen::Vector4f fragment = M * Eigen::Vector4f(j, i, zbuffer_copy[i][j], 1);
            Eigen::Vector4f fragmentFromLight = N*fragment;
            Eigen::Vector3f fragmentFromLightNonHomogeneous = fragmentFromLight.head(3) / fragmentFromLight(3);
            bool lit = (fragment[2] < (float)-100 || // fragment is in background, not object being rendered
                        fragmentFromLightNonHomogeneous[0] < 0 || fragmentFromLightNonHomogeneous[0] >= width || fragmentFromLightNonHomogeneous[1] < 0 || fragmentFromLightNonHomogeneous[1] >= height || // fragment outside of light view
                        fragmentFromLightNonHomogeneous[2] > zbuffer[(int)fragmentFromLightNonHomogeneous[1]][(int)fragmentFromLightNonHomogeneous[0]]); // fragment not visible from light
            mask[i][j] = lit;
        }
    }

    // write image representing shadows and where they appear
    TGAImage maskimg(width, height, TGAImage::GRAYSCALE);
    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            if (mask[y][x]) continue;
            maskimg.set(x, y, {255, 255, 255, 255});
        }
    }
    maskimg.flip_vertically();
    maskimg.write_tga_file("framebuffer_mask.tga");

    // apply shadow mask onto original image
    for (int x=0; x<width; x++) {
        for (int y=0; y<height; y++) {
            if (mask[y][x]) continue;
            TGAColor c = framebuffer.get(x, y);
            Eigen::Vector3f a{c.raw[0], c.raw[1], c.raw[2]};
            if (a.norm()<80) continue;
            a = a.normalized()*80;
            framebuffer.set(x, y, { a(0), a(1), a(2), 255 });
        }
    }
    framebuffer.write_tga_file("framebuffer_shadow_final.tga");

    return 0;
}


