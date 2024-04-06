#ifndef RASTERIZE_H
#define RASTERIZE_H
#include "geometry.h"
#include "tgaimage.h"


class Rasterize {
private:

public:
    static void line(Vec2i t0, Vec2i t1, TGAImage &image, TGAColor color);
    static void triangle(Vec3f *pts, TGAImage &image, TGAColor color);
};

Vec3f barycentric(Vec2i *pts, Vec2i P);

#endif //RASTERIZE_H
