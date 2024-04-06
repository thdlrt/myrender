#include <algorithm>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "rasterize.h"
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(0,   255, 0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;
const int depth  = 255;
Vec3f light_dir = Vec3f(0,0,1);
Vec3f camera(1,1,6);
Vec3f center(0,0,0);
Vec3f up(0,1,0);
Matrix ModelView;
struct GouraudShader : public IShader {
	Vec3f varying_intensity;
	Vec2f varying_uv[3];
	Vec3f vertex(int iface, int nthvert) override {
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert)*light_dir);
		varying_uv[nthvert] = model->uv(iface, nthvert);
		Vec3f gl_Vertex = model->vert(iface, nthvert);
		return m2v(ModelView*v2m(gl_Vertex));
	}

	bool fragment(Vec3f bar, TGAColor &color) override {
		float intensity = varying_intensity*bar;
		TGAColor t = model->diffuse(Vec2f(varying_uv[0]*bar[0] + varying_uv[1]*bar[1] + varying_uv[2]*bar[2]));
		color = TGAColor(t.r*intensity, t.g*intensity, t.b*intensity);
		return false;
	}
};

int main(int argc, char** argv) {

	TGAImage image(width, height, TGAImage::RGB);
	if (2==argc) {
		model = new Model(argv[1]);
	} else {
		model = new Model("../obj/african_head/african_head.obj");
	}
	ModelView = Viewport(width/8, height/8, width*3/4, height*3/4)
						* V(camera, center, up)
						* P(camera, center)
						* M();
	GouraudShader shader;
	for (int i=0; i<model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec3f screen_coords[3];
		for (int j=0; j<3; j++) {
			screen_coords[j] = shader.vertex(i, j);
		}
		Rasterize::triangle(screen_coords, shader, image);
	}

	image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}
