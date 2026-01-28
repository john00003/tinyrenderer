#include "our_gl.h"

Eigen::Matrix4f ModelView, Viewport, Perspective; // "OpenGL" state matrices
std::vector<std::vector<float>> zbuffer;               // depth buffer

void generateZBuffer(int width, int height)
{

    zbuffer = std::vector<std::vector<float>>(height, std::vector<float>(width, std::numeric_limits<float>::lowest()));
}

void generateViewportMatrix(const int x, const int y, const int w, const int h)
{
    Viewport << w/2.f, 0.f, 0.f, x+w/2.f,
                0.f, h/2.f, 0.f, y+h/2.f,
                0.f, 0.f, 1.f, 0.f,
                0.f, 0.f, 0.f, 1.f;
}

// Eigen::Matrix4f generateViewportMatrixWithGrayscaleDepth()
// {
//       Viewport = Eigen::Matrix4f{
//                 {width/2, 0, 0, width/2},
//                 {0, height/2, 0, height/2},
//                 { 0, 0, 255/2, 255/2},
//                 {0, 0, 0, 1}};
// }

void generatePerspectiveMatrix(float f)
{
     Perspective << 1.f, 0.f, 0.f, 0.f,
                    0.f, 1.f, 0.f, 0.f,
                    0.f, 0.f, 1.f, 0.f,
                    0.f, 0.f, -1.f/f, 1.f;
}

Eigen::Vector3f generateN(Eigen::Vector3f &eye, Eigen::Vector3f &center)
{
    Eigen::Vector3f direction = eye - center;
    return direction / direction.norm();
}

Eigen::Vector3f generateL(Eigen::Vector3f &n, Eigen::Vector3f &up)
{
    Eigen::Vector3f result = up.cross(n);
    return result / result.norm();
}

Eigen::Vector3f generateM(Eigen::Vector3f &l, Eigen::Vector3f &n)
{
    Eigen::Vector3f result = n.cross(l);
    return result / result.norm();
}


void generateModelViewMatrix(Eigen::Vector3f eye, Eigen::Vector3f center, Eigen::Vector3f up)
{
    Eigen::Vector3f n = generateN(eye, center);
    Eigen::Vector3f l = generateL(n, up);
    Eigen::Vector3f m = generateM(l, n);


    Eigen::Matrix4f changeOfBasisInverse;
    changeOfBasisInverse << l(0), l(1), l(2), 0,
                            m(0), m(1), m(2), 0,
                            n(0), n(1), n(2), 0,
                            0, 0, 0, 1;

    Eigen::Matrix4f translation;
    translation << 1, 0, 0, -center(0),
                   0, 1, 0, -center(1),
                   0, 0, 1, -center(2),
                   0, 0, 0, 1;

    ModelView = changeOfBasisInverse * translation;
}


// template <typename T>
// void triangleWithFillPerPixelPainters(std::vector<Vec3<T>> &vertices, TGAImage &framebuffer, std::vector<std::vector<float>> &depthBuffer, TGAColor color)
// {
//     // Vec2i point1(ax, ay);
//     // Vec2i point2(bx, by);
//     // Vec2i point3(cx, cy);
//     // std::vector<Vec2i> vertices{point1, point2, point3};
//
//     std::cout << "yay here" << std::endl;
//     std::cout << vertices[0] << " " << vertices[1] << " " << vertices[2] << std::endl;
//     std::vector<Vec3i> verticesProjected;
//     std::transform(vertices.begin(), vertices.end(),
//                std::back_inserter(verticesProjected),
//                [](Vec3f v) {
//                    return Vec3i{static_cast<int>(v.x), static_cast<int>(v.y), 1};
//                });
//     std::cout << verticesProjected[0] << " " << verticesProjected[1] << " " << verticesProjected[2] << std::endl;
//     double totalArea = signedTriangleArea(verticesProjected[0].x, verticesProjected[0].y, verticesProjected[1].x, verticesProjected[1].y, verticesProjected[2].x, verticesProjected[2].y);
//     //std::cout << ax << " " << ay << " " << bx << " " << by << " " << cx << " " << cy << std::endl;
//     // std::cout << "yay" << std::endl;
//     if (totalArea < 1){
//         // std::cout << "Total area is less than 1, skipping triangle" << std::endl;
//         //std::cout << "Total area:  " << totalArea << std::endl;
//         return;
//     }
//     // else{
//     //     std::cout << "Total area is " << totalArea << ", drawing triangle" << std::endl;
//     //     std::cout << "Colors: " << (int) color.raw[0] << ", " << (int) color.raw[1] << ", " << (int) color.raw[2] << ", " << (int) color.raw[3] << std::endl;
//     // }
//     //sortTriangleVerticesByX(vertices);
//     std::vector<Vec2i> bbox = computeBoundingBox<int>(verticesProjected, width, height);
//     std::cout << "bbox min x: " << bbox[0].x << " " << "bbox min y: " << bbox[0].y << " " << "bbox max x: " << bbox[1].x << " " << "bbox max y: " << bbox[1].y << std::endl;
//     //
//     const TGAColor randColor1 = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
//     const TGAColor randColor2 = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
//     const TGAColor randColor3 = {std::rand()%255, std::rand()%255, std::rand()%255, std::rand()%255};
//     std::vector<TGAColor> colors = {randColor1, randColor2, randColor3};
//
//     for (int i=bbox[0].y; i<=bbox[1].y; i++)
//     {
//         for (int j=bbox[0].x; j<=bbox[1].x; j++)
//         {
//             // std::cout << i << ", " << j << std::endl;
//             auto result = pointInTriangleBarycentricMethodWithDepthInterpolation(j,i, verticesProjected, vertices, totalArea); // returns depth at point (i,j), averaged between z of the three points
//             auto colorResult = pointInTriangleBarycentricMethodWithColorInterpolation(j, i, verticesProjected, colors, totalArea);
//             // std::cout << "safe" << std::endl;
//
//             if (result)
//             {
//                 // std::cout << "result: " << *result << std::endl;
//                 //std::cout << "point in triangle at " << i << " " << j << std::endl;
//                 //float depth = ()
//                 if (*result > depthBuffer[i][j])
//                 {
//                     //std::cout << "drawing overtop" << std::endl;
//                     //std::cout << *result << std::endl;
//                     //std::cout << "before: " << (int)depthBuffer.get(j, i).val << std::endl;
//                     //std::cout << "closer pixel detected" << std::endl;
//                     framebuffer.set(j, i, *colorResult);
//                     depthBuffer[i][j] = *result;
//                     // depthBuffer.set(j, i, TGAColor(*result, 1));
//                     //std::cout << "after: " << (int)depthBuffer.get(j, i).val << std::endl;
//                 }
//
//             }
//         }
//     }
// }

void rasterize(const Triangle &clip, const IShader &shader, TGAImage &framebuffer, const Eigen::Vector3f &camera, const Eigen::Vector3f &light) {
    Eigen::Vector4f ndc[3]    = { clip[0]/clip[0].w(), clip[1]/clip[1].w(), clip[2]/clip[2].w() };                // normalized device coordinates
    Eigen::Vector2f screen[3] = { (Viewport*ndc[0]).head<2>(), (Viewport*ndc[1]).head<2>(), (Viewport*ndc[2]).head<2>() }; // screen coordinates

    Eigen::Matrix3f ABC;
    ABC << screen[0].x(), screen[0].y(), 1.,
           screen[1].x(), screen[1].y(), 1.,
           screen[2].x(), screen[2].y(), 1.;
    if (ABC.determinant()<1) return; // backface culling + discarding triangles that cover less than a pixel

    auto [bbminx,bbmaxx] = std::minmax({screen[0].x(), screen[1].x(), screen[2].x()}); // bounding box for the triangle
    auto [bbminy,bbmaxy] = std::minmax({screen[0].y(), screen[1].y(), screen[2].y()}); // defined by its top left and bottom right corners
#pragma omp parallel for
    for (int x=std::max<int>(bbminx, 0); x<=std::min<int>(bbmaxx, framebuffer.get_width()-1); x++) {         // clip the bounding box by the screen
        for (int y=std::max<int>(bbminy, 0); y<=std::min<int>(bbmaxy, framebuffer.get_height()-1); y++) {
            Eigen::Vector3f bc = ABC.inverse().transpose() * Eigen::Vector3f{static_cast<float>(x), static_cast<float>(y), 1.}; // barycentric coordinates of {x,y} w.r.t the triangle
            if (bc(0)<0 || bc(1)<0 || bc(2)<0) continue;                                                    // negative barycentric coordinate => the pixel is outside the triangle
            float z = bc.dot(Eigen::Vector3f{ ndc[0].z(), ndc[1].z(), ndc[2].z() });  // linear interpolation of the depth
            if (z <= zbuffer[y][x]) continue;   // discard fragments that are too deep w.r.t the z-buffer
            auto [discard, color] = shader.fragment(bc, camera, light);
            if (discard) continue;                                 // fragment shader can discard current fragment
            zbuffer[y][x] = z;                  // update the z-buffer
            framebuffer.set(x, y, color);                          // update the framebuffer
        }
    }
}
