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

Model *model = NULL;

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

    bbox.emplace_back(minX, minY);
    bbox.emplace_back(maxX, maxY);

    return bbox;
}

template<typename T>
std::vector<Vec2i> computeBoundingBox(std::vector<T> &vertices)
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
    std::vector<Vec2i> bbox = computeBoundingBox<Vec2i>(vertices);
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

std::expected<TGAColor, std::string> pointInTriangleBarycentricMethodWithColorInterpolation(int px, int py, std::vector<Vec2i> &vertices, std::vector<TGAColor> &colors, double totalArea)
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
    std::vector<Vec2i> bbox = computeBoundingBox(vertices);
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

std::expected<int, std::string> pointInTriangleBarycentricMethodWithDepthInterpolation(int px, int py, std::vector<Vec3i> &vertices, double totalArea)
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
    int thisZ = alpha*vertices[0].z + beta*vertices[1].z + gamma*vertices[2].z;
    // std::cout << "values in pointInTriangle: " << std::endl;
    // std::cout << thisZ << std::endl;
    // std::cout << alpha << " " << beta << " " << gamma << std::endl;
    // std::cout << vertices[0].z << " " << vertices[1].z << " " << vertices[2].z << std::endl;
    return thisZ;
}

void triangleWithFillPerPixelPainters(std::vector<Vec3i> &vertices, TGAImage &framebuffer, TGAImage &depthBuffer, TGAColor color)
{
    // Vec2i point1(ax, ay);
    // Vec2i point2(bx, by);
    // Vec2i point3(cx, cy);
    // std::vector<Vec2i> vertices{point1, point2, point3};

    double totalArea = signedTriangleArea(vertices[0].x, vertices[0].y, vertices[1].x, vertices[1].y, vertices[2].x, vertices[2].y);
    //std::cout << ax << " " << ay << " " << bx << " " << by << " " << cx << " " << cy << std::endl;
    //std::cout << "yay" << std::endl;
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
    std::vector<Vec2i> bbox = computeBoundingBox<Vec3i>(vertices);
    //std::cout << "bbox min x: " << bbox[0].x << " " << "bbox min y: " << bbox[0].y << " " << "bbox max x: " << bbox[1].x << " " << "bbox max y: " << bbox[1].y << std::endl;

    for (int i=bbox[0].y; i<=bbox[1].y; i++)
    {
        for (int j=bbox[0].x; j<=bbox[1].x; j++)
        {
            auto result = pointInTriangleBarycentricMethodWithDepthInterpolation(j,i, vertices, totalArea); // returns depth normalized between [0,255]

            if (result)
            {
                //std::cout << "result: " << *result << std::endl;
                //std::cout << "point in triangle at " << i << " " << j << std::endl;
                //float depth = ()
                if (*result > depthBuffer.get(j, i).val)
                {
                    //std::cout << "drawing overtop" << std::endl;
                    //std::cout << *result << std::endl;
                    //std::cout << "before: " << (int)depthBuffer.get(j, i).val << std::endl;
                    //std::cout << "closer pixel detected" << std::endl;
                    framebuffer.set(j, i, color);
                    depthBuffer.set(j, i, TGAColor(*result, 1));
                    //std::cout << "after: " << (int)depthBuffer.get(j, i).val << std::endl;
                }

            }
        }
    }
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
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/diablo3_pose.obj");
    }

    // model->sortFaces();
    // for (int i=0; i<model->nfaces(); i++)
    // {
    //     std::cout << "min Z coord: " << std::min(model->vert(model->face(i)[0]).z, std::min(model->vert(model->face(i)[1]).z, model->vert(model->face(i)[2]).z));
    // }

    int width = 800;
    int height = 800;

    TGAImage image(width, height, TGAImage::RGB);
    TGAImage depthBuffer(width, height, TGAImage::GRAYSCALE);
    std::srand(std::time({}));

    for (int i=0; i<model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        std::vector<Vec3f> verticesf{model->vert(face[0]), model->vert(face[1]), model->vert(face[2])};
        std::vector<Vec3i> vertices = transformVertices(verticesf, width, height);
        // std::transform(verticesf.begin(), verticesf.end(),
        //            std::back_inserter(vertices),
        //            [&width, &height](Vec3f element) {
        //                return convertVec3fToVec3i(element, width, height);
        //            });
        // auto [ax, ay] = convertVec3fToXY(model->vert(face[0]), width, height);
        // auto [bx, by] = convertVec3fToXY(model->vert(face[1]), width, height);
        // auto [cx, cy] = convertVec3fToXY(model->vert(face[2]), width, height);
        // TGAColor rnd;
        // for (int c=0; c<3; c++) rnd[c] = std::rand()%255;
        //std::cout << vertices[0].x << " " << vertices[0].y << " " << vertices[0].z << std::endl;
        //std::cout << verticesf[0].x << " " << verticesf[0].y << " " << verticesf[0].z << std::endl;
        const TGAColor randColor = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
        triangleWithFillPerPixelPainters(vertices, image, depthBuffer, randColor);
    }
    image.flip_vertically();
    image.write_tga_file("model_per_pixel_painters_alg.tga");
    depthBuffer.flip_vertically();
    depthBuffer.write_tga_file("depth_buffer_per_pixel_painters_alg.tga");
    return 0;

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
}


