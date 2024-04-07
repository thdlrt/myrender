#ifndef RASTERIZE_H
#define RASTERIZE_H
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

//zBuffer单例类(SSAA4)
class ZBuffer {
private:
    int width, height;
    Vec3f *frameBuffer;
    float *deepBuffer;
public:
    ZBuffer(int width, int height) {
        this->width = width;
        this->height = height;
        frameBuffer = new Vec3f[width*height*4];
        deepBuffer = new float[width*height*4];
        for(int i=0;i<width*height*4;i++)
        {
            frameBuffer[i] = Vec3f(0,0,0);
            deepBuffer[i] = -std::numeric_limits<float>::max();
        }
    }
    ~ZBuffer() {
        delete [] frameBuffer;
        delete [] deepBuffer;
    }
    bool test(int x, int y, int i, float z, Vec3f color) {
        if(x<0||x>=width||y<0||y>=height) return false;
        if(z>deepBuffer[y*width*4+x*4+i]) {
            deepBuffer[y*width*4+x*4+i] = z;
            frameBuffer[y*width*4+x*4+i] = color;
            return true;
        }
        return false;
    }
    float getdeep(int x, int y) {
        float res=-std::numeric_limits<float>::max();
        for(int i=0;i<4;i++) {
            res = std::max(res, deepBuffer[y*width*4+x*4+i]);
        }
        return res;
    }
    Vec3f getcolor(int x, int y) {
        Vec3f res(0,0,0);
        for(int i=0;i<4;i++) {
            res = res + frameBuffer[y*width*4+x*4+i];
        }
        return res;
    }
};

struct IShader {
    virtual Vec3f vertex(int iface, int nthvert) = 0;
    virtual bool fragment(Vec3f bar, TGAColor &color, Vec3f p) = 0;
};

class Rasterize {
private:

public:
    static void line(Vec2i t0, Vec2i t1, TGAImage &image, TGAColor color);
    static void triangle(Vec3f *pts, IShader&shader, TGAImage &image, ZBuffer* zBuffer);
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
