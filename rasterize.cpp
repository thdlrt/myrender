//
// Created by MSI on 24-4-6.
//

#include "rasterize.h"

#include <cassert>

#include "model.h"

//求三角形的重心坐标
 Vec3f barycentric(Vec3f *pts, Vec3f P) {
     Vec3f u = Vec3f(pts[2][0]-pts[0][0], pts[1][0]-pts[0][0], pts[0][0]-P[0]) ^ Vec3f(pts[2][1]-pts[0][1], pts[1][1]-pts[0][1], pts[0][1]-P[1]);
     if (std::abs(u.z)<1-0.1) return Vec3f(-1,1,1);
     return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
 }
 // Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
 //     Vec3f s[2];
 //     for (int i=2; i--; ) {
 //         s[i][0] = C[i]-A[i];
 //         s[i][1] = B[i]-A[i];
 //         s[i][2] = A[i]-P[i];
 //     }
 //     Vec3f u = s[0]^s[1];
 //     if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
 //         return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
 //     return Vec3f(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
 // }
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
Matrix V(Vec3f camera, Vec3f center, Vec3f up) {
    Vec3f z = (camera-center).normalize();
    Vec3f x = (up^z).normalize();
    Vec3f y = (z^x).normalize();
    Matrix Minv = Matrix::identity(4);
    Matrix Tr = Matrix::identity(4);
    for (int i=0; i<3; i++) {
        Minv[0][i] = x[i];
        Minv[1][i] = y[i];
        Minv[2][i] = z[i];
        Tr[i][3] = -center[i];
    }
    return Minv*Tr;
}
//投影变换（透视）矩阵
Matrix P(Vec3f camera, Vec3f center) {
    Matrix p = Matrix::identity(4);
    p[3][2] = -1.f/(camera-center).norm();
    return p;
}
//视口变换矩阵
Matrix Viewport(int x, int y, int w, int h, int depth) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x+w/2.f;
    m[1][3] = y+h/2.f;
    m[2][3] = depth/2.f;
    m[0][0] = w/2.f;
    m[1][1] = h/2.f;
    m[2][2] = depth/2.f;
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
    void cal(Vec3f *pts, IShader&shader, TGAImage&image, ZBuffer* zBuffer) {
        Vec3f colorAccumulator;
        for(int i=0;i<spts.size();i++) {
            Vec3f a = spts[i];
            Vec3f bc_screen = barycentric(pts, a);
            //Vec3f bc_screen = barycentric(Vec2f(pts[0][0], pts[0][1]), Vec2f(pts[1][0], pts[1][1]), Vec2f(pts[2][0], pts[2][1]), Vec2f(a[0], a[1]));
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            a.z = 0;
            Vec2f uv;
            for (int j=0; j<3; j++)
            {
                a.z += pts[j][2]*bc_screen[j];
            }
            TGAColor color;
            if(shader.fragment(bc_screen, color, a)) continue;
                //TGAColor color = model->diffuse(Vec2i(uv.x, uv.y));
                zBuffer->test(a.x,a.y,i,a.z,Vec3f(color.r/4.f, color.g/4.f, color.b/4.f));
        }
    }
};
//包围盒绘制三角形
void Rasterize::triangle(Vec3f *pts, IShader&shader, TGAImage &image, ZBuffer* zBuffer) {
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
            msaa4.cal(pts, shader, image, zBuffer);
            image.set(P.x,P.y,zBuffer->getcolor(P.x,P.y));
            //非抗锯齿版本
            // Vec3f bc_screen  = barycentric(pts, P);
            // if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            // P.z = 0;
            // Vec2i uv;
            // for (int i=0; i<3; i++)
            // {
            //     P.z += pts[i][2]*bc_screen[i];
            // }
            // TGAColor color;
            // if(shader.fragment(bc_screen, color)) continue;
            // zBuffer::getInstance()->test(P.x,P.y,0,P.z,Vec3f(color.r, color.g, color.b));
            // image.set(P.x,P.y,zBuffer::getInstance()->getcolor(P.x,P.y));
        }
    }
}

