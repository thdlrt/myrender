//
// Created by MSI on 24-4-6.
//

#include "rasterize.h"
//zBuffer单例类
class zBuffer {
private:
    zBuffer(int width, int height) {
        this->width = width;
        this->height = height;
        buffer = new float[width*height];
        for(int i=0;i<width*height;i++){
            buffer[i] = -std::numeric_limits<float>::max();
        }
    }
    ~zBuffer() {
        delete [] buffer;
    }
public:
    int width, height;
    float *buffer;
    static zBuffer* getInstance(int width, int height) {
        static zBuffer instance(width, height);
        return &instance;
    }
    bool test(int x, int y, float z) {
        if(x<0||x>=width||y<0||y>=height) return false;
        if(z>buffer[y*width+x]) {
            buffer[y*width+x] = z;
            return true;
        }
        return false;
    }
};
//求三角形的重心坐标
Vec3f barycentric(Vec3f *pts, Vec3f P) {
    Vec3f u = Vec3f(pts[2][0]-pts[0][0], pts[1][0]-pts[0][0], pts[0][0]-P[0]) ^ Vec3f(pts[2][1]-pts[0][1], pts[1][1]-pts[0][1], pts[0][1]-P[1]);
    if (std::abs(u.z)<1-0.1) return Vec3f(-1,1,1);
    return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
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

//包围盒绘制三角形
void Rasterize::triangle(Vec3f *pts, TGAImage &image, TGAColor color) {
    Vec3f t0 = pts[0], t1 = pts[1], t2 = pts[2];
    float minx = std::max((float)0,std::min(t0.x, std::min(t1.x, t2.x)));
    float maxx = std::min((float)image.get_width()-1,std::max(t0.x, std::max(t1.x, t2.x)));
    float miny = std::max((float)0,std::min(t0.y, std::min(t1.y, t2.y)));
    float maxy = std::min((float)image.get_height()-1,std::max(t0.y, std::max(t1.y, t2.y)));
    //遍历包围盒内的像素点
    Vec3f P;
    for (P.x=minx; P.x<=maxx; P.x++) {
        for (P.y=miny; P.y<=maxy; P.y++) {
            Vec3f bc_screen  = barycentric(pts, P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            P.z = 0;
            for (int i=0; i<3; i++) P.z += pts[i][2]*bc_screen[i];
            //深度测试
            if(zBuffer::getInstance(image.get_width(), image.get_height())->test(P.x, P.y, P.z))
                image.set(P.x, P.y, color);
        }
    }
}

