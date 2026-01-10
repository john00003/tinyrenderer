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

    // if (y0 > y1)
    // {
    //     std::swap(y0, y1);
    // }

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

        // if (error > 1)
        // {
        //     y += 1;
        //     // error -= 1.;
        //     error -= 2*dx;    // multiply the subtracting term by 2 * dx
        // }

        // branchless version
        y += (y1 > y0 ? 1 : -1) * (error > dx);
        error -= 2 * (dx)   * (error > dx);



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

    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/diablo3_pose.obj");
    }

    TGAImage image(800, 800, TGAImage::RGB);

    for (int i=0; i<model->nfaces(); i++)
    {
        std::vector<int> face = model->face(i);
        for (int j=0; j<3; j++)     // we iterate through each vertex of the face, drawing a line between two vertices
        {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j+1)%3]);
            // for each point we add one to ensure it is positive, divide by 2 to normalize it to be within [0,1]
            int x0 = (v0.x+1.)*width/2.;
            int y0 = (v0.y+1.)*height/2.;
            int x1 = (v1.x+1.)*width/2.;
            int y1 = (v1.y+1.)*height/2.;
            line(x0, y0, x1, y1, image, white);
        }
    }


    image.flip_vertically();
    image.write_tga_file("output.tga");
    return 0;
}


