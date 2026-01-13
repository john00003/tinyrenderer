#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
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

    TGAImage framebuffer(width, height, TGAImage::RGB);
    triangleWithFill(  7, 45, 35, 100, 45,  60, framebuffer, red);
    triangle(120, 35, 90,   5, 45, 110, framebuffer, white);
    triangleWithFill(115, 83, 80,  90, 85, 120, framebuffer, green);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}


