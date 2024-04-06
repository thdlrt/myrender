#ifndef RASTERIZE_H
#define RASTERIZE_H
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

struct IShader {
    virtual Vec3f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

class Rasterize {
private:

public:
    static void line(Vec2i t0, Vec2i t1, TGAImage &image, TGAColor color);
    static void triangle(Vec3f *pts, IShader&shader, TGAImage &image);
};

Vec3f barycentric(Vec2i *pts, Vec2i P);
Vec3f world2screen(Vec3f v,int width, int height);
Vec3f m2v(Matrix m);
Matrix v2m(Vec3f v);
Matrix M(float mx=0, float my=0, float mz=0, float rx=0, float ry=0, float rz=0, float sx=1, float sy=1, float sz=1);
Matrix V(Vec3f camera, Vec3f center, Vec3f up);
Matrix P(Vec3f camera, Vec3f center);
Matrix Viewport(int x, int y, int w, int h,int depth=255);
TGAColor bilerp(Vec2f uv, Model*model);
#endif //RASTERIZE_H
