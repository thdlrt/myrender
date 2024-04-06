//
// Created by MSI on 24-4-6.
//

#include "rasterize.h"

#include <cassert>

#include "model.h"

//zBuffer单例类(SSAA4)
class zBuffer {
private:
    int width, height;
    Vec3f *frameBuffer;
    float *deepBuffer;
    zBuffer(int width, int height) {
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
    ~zBuffer() {
        delete [] frameBuffer;
        delete [] deepBuffer;
    }
public:
    static zBuffer* getInstance(int width=0, int height=0) {
        static zBuffer instance(width, height);
        return &instance;
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
//求三角形的重心坐标
Vec3f barycentric(Vec3f *pts, Vec3f P) {
    Vec3f u = Vec3f(pts[2][0]-pts[0][0], pts[1][0]-pts[0][0], pts[0][0]-P[0]) ^ Vec3f(pts[2][1]-pts[0][1], pts[1][1]-pts[0][1], pts[0][1]-P[1]);
    if (std::abs(u.z)<1-0.1) return Vec3f(-1,1,1);
    return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
}
//坐标和矩阵间的转化
Vec3f m2v(Matrix m) {
    return Vec3f(m[0][0]/m[3][0], m[1][0]/m[3][0], m[2][0]/m[3][0]);
}
Matrix v2m(Vec3f v) {
    Matrix m(4,1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1;
    return m;
}
//模型变换矩阵
Matrix M(float mx, float my, float mz, float rx, float ry, float rz, float sx, float sy, float sz) {
    Matrix m = Matrix::identity(4);
    Matrix t = Matrix::identity(4);
    Matrix r = Matrix::identity(4);
    Matrix s = Matrix::identity(4);
    t[0][3] = mx;
    t[1][3] = my;
    t[2][3] = mz;
    r[0][0] = cos(ry)*cos(rz);
    r[0][1] = cos(ry)*sin(rz);
    r[0][2] = -sin(ry);
    r[1][0] = sin(rx)*sin(ry)*cos(rz) - cos(rx)*sin(rz);
    r[1][1] = sin(rx)*sin(ry)*sin(rz) + cos(rx)*cos(rz);
    r[1][2] = sin(rx)*cos(ry);
    r[2][0] = cos(rx)*sin(ry)*cos(rz) + sin(rx)*sin(rz);
    r[2][1] = cos(rx)*sin(ry)*sin(rz) - sin(rx)*cos(rz);
    r[2][2] = cos(rx)*cos(ry);
    s[0][0] = sx;
    s[1][1] = sy;
    s[2][2] = sz;
    return m*t*r*s;

}
//视图变换矩阵
Matrix V(Vec3f camera, Vec3f center, Vec3f up, int width, int height) {

}
//投影变换（透视）矩阵
Matrix P(Vec3f camera) {
    Matrix m = Matrix::identity(4);
    m[3][2] = -1.f/camera.z;
    return m;
}
//绘制直线
void Rasterize::line(Vec2i t0, Vec2i t1, TGAImage &image, TGAColor color) {
    int x0 = t0.x, y0 = t0.y, x1 = t1.x, y1 = t1.y;
    int dx = abs(x1-x0),dy=abs(y1-y0);
    if(dx>dy){
        if(x0>x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        for (int x=x0; x<=x1; x++) {
            float t = (x-x0)/(float)(x1-x0);
            int y = y0*(1.-t) + y1*t;
            image.set(x, y, color);
        }
    }
    else{
        if(y0>y1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
        }
        for (int y=y0; y<=y1; y++) {
            float t = (y-y0)/(float)(y1-y0);
            int x = x0*(1.-t) + x1*t;
            image.set(x, y, color);
        }
    }
}
//双线性插值
TGAColor bilerp(Vec2f uv, Model*model) {
    float x0=floor(uv.x), y0=floor(uv.y), x1=x0+1, y1=y0+1;
    float u=uv.x-x0, v=uv.y-y0;
    TGAColor c00 = model->diffuse(Vec2i(x0, y0));
    TGAColor c01 = model->diffuse(Vec2i(x0, y1));
    TGAColor c10 = model->diffuse(Vec2i(x1, y0));
    TGAColor c11 = model->diffuse(Vec2i(x1, y1));
    Vec3f color = Vec3f(c00.r*(1-u)*(1-v) + c01.r*(1-u)*v + c10.r*u*(1-v) + c11.r*u*v,
                        c00.g*(1-u)*(1-v) + c01.g*(1-u)*v + c10.g*u*(1-v) + c11.g*u*v,
                        c00.b*(1-u)*(1-v) + c01.b*(1-u)*v + c10.b*u*(1-v) + c11.b*u*v);
    return color;
}
//抗锯齿ssaa
class SSAA4 {
    std::vector<Vec3f> spts;
public:
    SSAA4(float x, float y, float z) {
        spts.emplace_back(x+0.25f, y+0.25f, z);
        spts.emplace_back(x+0.75f, y+0.25f, z);
        spts.emplace_back(x+0.25f, y+0.75f, z);
        spts.emplace_back(x+0.75f, y+0.75f, z);
        //spts.emplace_back(x,y,z);
    }
    void cal(Vec3f *pts, Vec2f *uvs, Model*model,TGAImage&image) {
        Vec3f colorAccumulator;
        int num=0;
        for(int i=0;i<spts.size();i++) {
            Vec3f a = spts[i];
            Vec3f bc_screen  = barycentric(pts, a);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            a.z = 0;
            Vec2f uv;
            for (int j=0; j<3; j++)
            {
                a.z += pts[j][2]*bc_screen[j];
                uv = uv + uvs[j]*bc_screen[j];
            }
            TGAColor color = bilerp(uv, model);
            //TGAColor color = model->diffuse(Vec2i(uv.x, uv.y));
            zBuffer::getInstance()->test(a.x,a.y,i,a.z,Vec3f(color.r/4.f, color.g/4.f, color.b/4.f));
        }
    }
};
//包围盒绘制三角形
void Rasterize::triangle(Vec3f *pts, Vec2f *uvs, TGAImage &image, Model *model) {
    zBuffer::getInstance(image.get_width(), image.get_height());
    Vec3f t0 = pts[0], t1 = pts[1], t2 = pts[2];
    int minx = std::max((float)0,std::min(t0.x, std::min(t1.x, t2.x)));
    int maxx = std::min((float)image.get_width()-1,std::max(t0.x, std::max(t1.x, t2.x)));
    int miny = std::max((float)0,std::min(t0.y, std::min(t1.y, t2.y)));
    int maxy = std::min((float)image.get_height()-1,std::max(t0.y, std::max(t1.y, t2.y)));
    //遍历包围盒内的像素点
    Vec3f P;
    for (P.x=minx; P.x<=maxx; P.x++) {
        for (P.y=miny; P.y<=maxy; P.y++) {
            SSAA4 msaa4(P.x, P.y, 0);
            msaa4.cal(pts, uvs, model,image);
            image.set(P.x,P.y,zBuffer::getInstance()->getcolor(P.x,P.y));
            //非抗锯齿版本
            // Vec3f bc_screen  = barycentric(pts, P);
            // if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            // P.z = 0;
            // Vec2i uv;
            // for (int i=0; i<3; i++)
            // {
            //     P.z += pts[i][2]*bc_screen[i];
            //     uv = uv + uvs[i]*bc_screen[i];
            // }
            // TGAColor color = model->diffuse(uv);
            // zBuffer::getInstance()->test(P.x,P.y,0,P.z,Vec3f(color.r, color.g, color.b));
            // image.set(P.x,P.y,zBuffer::getInstance()->getcolor(P.x,P.y));
        }
    }
}

