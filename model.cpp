#include <iostream>
#include <fstream>
#include <sstream>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_(), norms_(), uv_(), diffusemap_(), normalmap_(), specularmap_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts_.push_back(v);
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            Vec3f n;
            for (int i=0;i<3;i++) iss >> n[i];
            norms_.push_back(n);
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            Vec2f uv;
            for (int i=0;i<2;i++) iss >> uv[i];
            uv_.push_back(uv);
        }  else if (!line.compare(0, 2, "f ")) {
            std::vector<Vec3i> f;
            Vec3i tmp;
            iss >> trash;
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i=0; i<3; i++) tmp[i]--; // in wavefront obj all indices start at 1, not zero
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << " vt# " << uv_.size() << " vn# " << norms_.size() << std::endl;
    load_texture(filename, "_diffuse.tga", diffusemap_);
    load_texture(filename, "_nm.tga",      normalmap_);
    load_texture(filename, "_spec.tga",    specularmap_);
}

Model::~Model() {}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) {
    std::vector<int> face;
    for (int i=0; i<(int)faces_[idx].size(); i++) face.push_back(faces_[idx][i][0]);
    return face;
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec3f Model::vert(int iface, int nthvert) {
    return verts_[faces_[iface][nthvert][0]];
}

void Model::load_texture(std::string filename, const char *suffix, TGAImage &img) {
    std::string texfile(filename);
    size_t dot = texfile.find_last_of(".");
    if (dot!=std::string::npos) {
        texfile = texfile.substr(0,dot) + std::string(suffix);
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
}
TGAColor Model::bilerp(Vec2f uv) {
    float x0=floor(uv.x), y0=floor(uv.y), x1=x0+1, y1=y0+1;
    float u=uv.x-x0, v=uv.y-y0;
    TGAColor c00 = diffusemap_.get(x0, y0);
    TGAColor c01 = diffusemap_.get(x0, y1);
    TGAColor c10 = diffusemap_.get(x1, y0);
    TGAColor c11 = diffusemap_.get(x1, y1);
    Vec3f color = Vec3f(c00.r*(1-u)*(1-v) + c01.r*(1-u)*v + c10.r*u*(1-v) + c11.r*u*v,
                        c00.g*(1-u)*(1-v) + c01.g*(1-u)*v + c10.g*u*(1-v) + c11.g*u*v,
                        c00.b*(1-u)*(1-v) + c01.b*(1-u)*v + c10.b*u*(1-v) + c11.b*u*v);
    return color;
}
TGAColor Model::diffuse(Vec2f uvf) {
    Vec2f uv(uvf[0]*diffusemap_.get_width(), uvf[1]*diffusemap_.get_height());
    return bilerp(uv);
}

Vec3f Model::normal(Vec2f uvf) {
    Vec2i uv(uvf[0]*normalmap_.get_width(), uvf[1]*normalmap_.get_height());
    TGAColor c = normalmap_.get(uv[0], uv[1]);
    Vec3f res;
    for (int i=0; i<3; i++)
        res[2-i] = (float)c[i]/255.f*2.f - 1.f;
    return res;
}

Vec2f Model::uv(int iface, int nthvert) {
    return uv_[faces_[iface][nthvert][1]];
}

float Model::specular(Vec2f uvf) {
    Vec2i uv(uvf[0]*specularmap_.get_width(), uvf[1]*specularmap_.get_height());
    return specularmap_.get(uv[0], uv[1])[0]/1.f;
}

Vec3f Model::normal(int iface, int nthvert) {
    int idx = faces_[iface][nthvert][2];
    return norms_[idx].normalize();
}