#include <algorithm>
#include <cassert>

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
Vec3f light_p = Vec3f(0.3,2,2);
Vec3f camera(0,1,3);
Vec3f center(0,0,0);
Vec3f up(0,1,0);
Matrix ModelView, ShadowView;
ZBuffer shadowBuffer(width, height);
struct ShadowShader : public IShader {
	Vec3f vertex(int iface, int nthvert) override {
		return m2v(ShadowView*v2m(model->vert(iface, nthvert)));
	}
	bool fragment(Vec3f bar, TGAColor &color, Vec3f p) override {
		color = TGAColor(255*p.z/depth, 255*p.z/depth, 255*p.z/depth);
		return false;
	}
};
struct PhongShader : public IShader {
	Vec2f varying_uv[3];
	Vec3f sb_p[3];
	Vec3f vertex(int iface, int nthvert) override {
		varying_uv[nthvert] = model->uv(iface, nthvert);
		Vec3f gl_Vertex = model->vert(iface, nthvert);
		sb_p[nthvert] = gl_Vertex;
		return m2v(ModelView*v2m(gl_Vertex));
	}

	bool fragment(Vec3f bar, TGAColor &color, Vec3f p) override {
		Vec3f shadowp = m2v(ShadowView*v2m(sb_p[0]*bar.x + sb_p[1]*bar.y + sb_p[2]*bar.z));
		float shadowdeep = shadowBuffer.getdeep(shadowp.x, shadowp.y);
		float shadow_deffuse = 0.7+.3*(shadowdeep<shadowp.z+43.34);
		float shadow_reflect = .3+.7*(shadowdeep<shadowp.z+43.34);

		Vec2f uv = varying_uv[0]*bar.x + varying_uv[1]*bar.y + varying_uv[2]*bar.z;
		Vec3f normal = m2v((ModelView).transpose()*v2m(model->normal(uv))).normalize();
		Vec3f light = m2v(ModelView*v2m(light_p)).normalize();
		Vec3f see = (m2v(ModelView*v2m(camera))-m2v(ModelView*v2m(p))).normalize();
		Vec3f mid = (light+see).normalize();
		float dis = (m2v(ModelView*v2m(p))-m2v(ModelView*v2m(camera))).norm()/100;
		float attenuation = 300.0 / (0.5 * dis + 0.1 * dis * dis);
		Vec3f ambient = Vec3f(0.8, 0.8, 0.8);
		Vec3f diffuse = Vec3f(1, 1, 1)*std::max(0.f, normal*light)*attenuation;
		Vec3f specular = Vec3f(0.3, 0.3, 0.3)*pow(std::max(0.f, normal*mid), model->specular(uv))*attenuation;
		Vec3f result = ambient*shadow_deffuse+(diffuse+specular)*shadow_reflect;
		TGAColor c = model->diffuse(uv);
		color = TGAColor(std::min(255.f,c[2]*result[2]), std::min(255.f,c[1]*result[1]), std::min(255.f,c[0]*result[0]));
		return false;
	}
};

int main(int argc, char** argv) {

	TGAImage image(width, height, TGAImage::RGB);
	TGAImage shadowimage(width, height, TGAImage::RGB);
	if (2==argc) {
		model = new Model(argv[1]);
	} else {
		model = new Model("../obj/african_head/african_head.obj");
	}
	ModelView = Viewport(width/8, height/8, width*3/4, height*3/4)
						* V(camera, center, up)
						* P(camera, center)
						* M();
	ShadowView = Viewport(width/8, height/8, width*3/4, height*3/4)
						* V(light_p, center, up)
						* P(light_p, center)
						* M();
	PhongShader shader;
	ShadowShader shadowShader;
	for(int i=0;i<model->nfaces(); i++)
	{
		std::vector<int> face = model->face(i);
		Vec3f screen_coords[3];
		for(int j=0;j<3;j++)
		{
			screen_coords[j] = shadowShader.vertex(i, j);
		}
		Rasterize::triangle(screen_coords, shadowShader, shadowimage, &shadowBuffer);
	}
	ZBuffer zBuffer(width, height);
	for (int i=0; i<model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec3f screen_coords[3];
		for (int j=0; j<3; j++) {
			screen_coords[j] = shader.vertex(i, j);
		}
		Rasterize::triangle(screen_coords, shader, image, &zBuffer);
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	shadowimage.flip_vertically();
	shadowimage.write_tga_file("shadow.tga");
	return 0;
}
