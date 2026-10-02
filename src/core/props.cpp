#include "core/props.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/capsule_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/prism_mesh.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/torus_mesh.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace urbex {

namespace {

struct P {
	LevelBuilder &b;
	Transform3D xf;
	bool shadows = true;

	P(LevelBuilder &builder, const Vector3 &pos, float yaw) :
			b(builder), xf(Basis(Vector3(0, 1, 0), yaw), pos) {}

	Transform3D at(const Vector3 &c, const Vector3 &rot = Vector3()) const {
		return xf * Transform3D(Basis::from_euler(rot), c);
	}

	MeshInstance3D *box(const Vector3 &c, const Vector3 &s, const char *m, const Vector3 &rot = Vector3()) {
		Ref<BoxMesh> mesh;
		mesh.instantiate();
		mesh->set_size(s);
		return b.visual_mesh(mesh, at(c, rot), m, shadows);
	}

	MeshInstance3D *cyl(const Vector3 &c, float r, float h, const char *m, const Vector3 &rot = Vector3(), int seg = 12, float r_top = -1.0f) {
		Ref<CylinderMesh> mesh;
		mesh.instantiate();
		mesh->set_bottom_radius(r);
		mesh->set_top_radius(r_top < 0.0f ? r : r_top);
		mesh->set_height(h);
		mesh->set_radial_segments(seg);
		mesh->set_rings(1);
		return b.visual_mesh(mesh, at(c, rot), m, shadows);
	}

	MeshInstance3D *sphere(const Vector3 &c, float r, const char *m, const Vector3 &scl = Vector3(1, 1, 1)) {
		Ref<SphereMesh> mesh;
		mesh.instantiate();
		mesh->set_radius(r);
		mesh->set_height(r * 2.0f);
		mesh->set_radial_segments(12);
		mesh->set_rings(6);
		Transform3D t = at(c);
		t.basis = t.basis.scaled_local(scl);
		return b.visual_mesh(mesh, t, m, shadows);
	}

	MeshInstance3D *torus(const Vector3 &c, float inner, float outer, const char *m, const Vector3 &rot = Vector3()) {
		Ref<TorusMesh> mesh;
		mesh.instantiate();
		mesh->set_inner_radius(inner);
		mesh->set_outer_radius(outer);
		mesh->set_rings(16);
		mesh->set_ring_segments(6);
		return b.visual_mesh(mesh, at(c, rot), m, shadows);
	}

	MeshInstance3D *capsule(const Vector3 &c, float r, float h, const char *m, const Vector3 &rot = Vector3()) {
		Ref<CapsuleMesh> mesh;
		mesh.instantiate();
		mesh->set_radius(r);
		mesh->set_height(h);
		mesh->set_radial_segments(10);
		mesh->set_rings(4);
		return b.visual_mesh(mesh, at(c, rot), m, shadows);
	}

	MeshInstance3D *prism(const Vector3 &c, const Vector3 &s, const char *m, const Vector3 &rot = Vector3(), float left = 0.5f) {
		Ref<PrismMesh> mesh;
		mesh.instantiate();
		mesh->set_size(s);
		mesh->set_left_to_right(left);
		return b.visual_mesh(mesh, at(c, rot), m, shadows);
	}

	void pipe(const Vector3 &a, const Vector3 &c, float r, const char *m) {
		b.pipe(xf.xform(a), xf.xform(c), r, m);
	}

	void collider(const Vector3 &c, const Vector3 &s, const Vector3 &rot = Vector3()) {
		Transform3D t = at(c, rot);
		b.collider_box(t.origin, t.basis, s);
	}

	Label3D *label(const Vector3 &c, const Vector3 &normal_local, const String &text, float h, const Color &col) {
		Vector3 n = xf.basis.xform(normal_local);
		return b.text(xf.xform(c), n, text, h, col, true);
	}
};

uint32_t step(uint32_t &s) {
	s = s * 1664525u + 1013904223u;
	return s >> 8;
}

float frand(uint32_t &s) {
	return float(step(s) % 10000) / 10000.0f;
}

}

namespace props {

void hospital_bed(LevelBuilder &b, const Vector3 &pos, float yaw, bool with_mattress) {
	P p(b, pos, yaw);
	for (float sx : { -0.42f, 0.42f }) {
		p.pipe(Vector3(sx, 0.5f, -0.95f), Vector3(sx, 0.5f, 0.95f), 0.022f, "rust");
		p.pipe(Vector3(sx, 0.12f, -0.95f), Vector3(sx, 0.95f, -0.95f), 0.02f, "rust");
		p.pipe(Vector3(sx, 0.12f, 0.95f), Vector3(sx, 0.75f, 0.95f), 0.02f, "rust");
		for (float sz : { -0.95f, 0.95f }) {
			p.cyl(Vector3(sx, 0.06f, sz), 0.05f, 0.03f, "tire", Vector3(0, 0, PI * 0.5f), 10);
		}
	}
	p.pipe(Vector3(-0.42f, 0.95f, -0.95f), Vector3(0.42f, 0.95f, -0.95f), 0.02f, "rust");
	p.pipe(Vector3(-0.42f, 0.75f, 0.95f), Vector3(0.42f, 0.75f, 0.95f), 0.02f, "rust");
	for (int i = 0; i < 5; i++) {
		float x = -0.3f + float(i) * 0.15f;
		p.pipe(Vector3(x, 0.5f, -0.95f), Vector3(x, 0.95f, -0.95f), 0.008f, "rust");
	}
	p.box(Vector3(0, 0.49f, 0), Vector3(0.84f, 0.02f, 1.85f), "rust_local");
	if (with_mattress) {
		p.box(Vector3(0.02f, 0.57f, 0.05f), Vector3(0.8f, 0.14f, 1.7f), "mattress", Vector3(0.0f, 0.05f, 0.03f));
	}
	p.collider(Vector3(0, 0.35f, 0), Vector3(0.9f, 0.7f, 1.95f));
}

void wheelchair(LevelBuilder &b, const Vector3 &pos, float yaw, bool tipped) {
	P p(b, pos, yaw);
	if (tipped) {
		p.xf = p.xf * Transform3D(Basis(Vector3(0, 0, 1), 1.45f), Vector3(0.0f, 0.3f, 0.0f));
	}
	for (float sx : { -0.3f, 0.3f }) {
		p.torus(Vector3(sx, 0.3f, 0.05f), 0.27f, 0.3f, "tire", Vector3(0, 0, PI * 0.5f));
		p.torus(Vector3(sx * 1.06f, 0.3f, 0.05f), 0.22f, 0.24f, "steel", Vector3(0, 0, PI * 0.5f));
		p.cyl(Vector3(sx * 0.85f, 0.07f, -0.35f), 0.07f, 0.03f, "tire", Vector3(0, 0, PI * 0.5f));
		p.pipe(Vector3(sx * 0.85f, 0.45f, 0.2f), Vector3(sx * 0.85f, 0.95f, 0.25f), 0.015f, "steel");
		p.pipe(Vector3(sx * 0.85f, 0.45f, -0.3f), Vector3(sx * 0.85f, 0.45f, 0.2f), 0.015f, "steel");
		p.pipe(Vector3(sx * 0.85f, 0.45f, -0.3f), Vector3(sx * 0.85f, 0.1f, -0.35f), 0.012f, "steel");
	}
	p.box(Vector3(0, 0.47f, -0.05f), Vector3(0.48f, 0.04f, 0.45f), "black");
	p.box(Vector3(0, 0.72f, 0.22f), Vector3(0.48f, 0.42f, 0.03f), "black", Vector3(-0.1f, 0, 0));
	p.collider(Vector3(0, 0.4f, 0), Vector3(0.65f, 0.8f, 0.7f));
}

void cabinet(LevelBuilder &b, const Vector3 &pos, float yaw, bool open) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.9f, 0), Vector3(0.8f, 1.8f, 0.45f), "metal_gray_local");
	p.box(Vector3(0, 0.9f, -0.23f), Vector3(0.76f, 1.76f, 0.01f), "black");
	for (float y : { 0.35f, 0.8f, 1.25f }) {
		p.box(Vector3(0, y, 0.0f), Vector3(0.74f, 0.02f, 0.4f), "metal_gray_local");
	}
	p.box(Vector3(-0.2f, 0.9f, -0.235f), Vector3(0.38f, 1.74f, 0.02f), "metal_gray_local");
	if (open) {
		p.box(Vector3(0.35f, 0.9f, -0.42f), Vector3(0.38f, 1.74f, 0.02f), "metal_gray_local", Vector3(0, 1.2f, 0));
		p.box(Vector3(0.1f, 0.83f, -0.05f), Vector3(0.3f, 0.06f, 0.25f), "paper");
	} else {
		p.box(Vector3(0.2f, 0.9f, -0.235f), Vector3(0.38f, 1.74f, 0.02f), "metal_gray_local");
	}
	p.box(Vector3(0.02f, 1.0f, -0.25f), Vector3(0.02f, 0.12f, 0.02f), "steel");
	p.collider(Vector3(0, 0.9f, 0), Vector3(0.8f, 1.8f, 0.45f));
}

void chair(LevelBuilder &b, const Vector3 &pos, float yaw, bool fallen) {
	P p(b, pos, yaw);
	if (fallen) {
		p.xf = p.xf * Transform3D(Basis(Vector3(1, 0, 0), -1.5f), Vector3(0.0f, 0.22f, 0.0f));
	}
	for (float sx : { -0.19f, 0.19f }) {
		for (float sz : { -0.19f, 0.19f }) {
			p.box(Vector3(sx, 0.22f, sz), Vector3(0.025f, 0.44f, 0.025f), "steel");
		}
		p.box(Vector3(sx, 0.68f, 0.19f), Vector3(0.025f, 0.48f, 0.025f), "steel");
	}
	p.box(Vector3(0, 0.45f, 0), Vector3(0.42f, 0.025f, 0.42f), "wood_local");
	p.box(Vector3(0, 0.75f, 0.2f), Vector3(0.42f, 0.25f, 0.02f), "wood_local");
}

void table(LevelBuilder &b, const Vector3 &pos, float yaw, float width, float depth) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.76f, 0), Vector3(width, 0.035f, depth), "wood_local");
	for (float sx : { -0.5f, 0.5f }) {
		for (float sz : { -0.5f, 0.5f }) {
			p.box(Vector3(sx * (width - 0.08f), 0.38f, sz * (depth - 0.08f)), Vector3(0.04f, 0.76f, 0.04f), "wood_local");
		}
	}
	p.box(Vector3(0, 0.66f, 0), Vector3(width - 0.1f, 0.12f, depth - 0.1f), "wood_local");
	p.collider(Vector3(0, 0.39f, 0), Vector3(width, 0.78f, depth));
}

void barrel(LevelBuilder &b, const Vector3 &pos, const char *material, bool open_top) {
	P p(b, pos, 0.0f);
	p.cyl(Vector3(0, 0.45f, 0), 0.29f, 0.9f, material, Vector3(), 16);
	for (float y : { 0.05f, 0.3f, 0.6f, 0.86f }) {
		p.torus(Vector3(0, y, 0), 0.285f, 0.305f, material);
	}
	if (open_top) {
		p.cyl(Vector3(0, 0.82f, 0), 0.26f, 0.02f, "black", Vector3(), 16);
	}
	p.collider(Vector3(0, 0.45f, 0), Vector3(0.58f, 0.9f, 0.58f));
}

void mattress(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.08f, 0), Vector3(0.9f, 0.16f, 1.9f), "mattress", Vector3(0.02f, 0.0f, 0.01f));
	p.box(Vector3(0.1f, 0.17f, -0.6f), Vector3(0.5f, 0.06f, 0.35f), "cloth_stained", Vector3(0.0f, 0.3f, 0.05f));
	p.box(Vector3(-0.1f, 0.19f, 0.3f), Vector3(0.8f, 0.03f, 0.9f), "canvas", Vector3(0.05f, -0.2f, 0.02f));
}

void bottles(LevelBuilder &b, const Vector3 &pos, int count, uint32_t seed) {
	P p(b, pos, 0.0f);
	p.shadows = false;
	for (int i = 0; i < count; i++) {
		float a = frand(seed) * 6.28f;
		float r = frand(seed) * 0.6f;
		Vector3 c(std::cos(a) * r, 0.0f, std::sin(a) * r);
		bool lying = frand(seed) > 0.5f;
		const char *m = frand(seed) > 0.5f ? "glass_bottle" : "glass_dirty";
		if (lying) {
			p.cyl(c + Vector3(0, 0.035f, 0), 0.035f, 0.24f, m, Vector3(PI * 0.5f, frand(seed) * 3.0f, 0), 8, 0.035f);
		} else {
			p.cyl(c + Vector3(0, 0.1f, 0), 0.035f, 0.2f, m, Vector3(), 8);
			p.cyl(c + Vector3(0, 0.24f, 0), 0.015f, 0.08f, m, Vector3(), 6);
		}
	}
}

void cardboard(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.01f, 0), Vector3(1.2f, 0.01f, 0.8f), "cardboard", Vector3(0.0f, 0.0f, 0.02f));
	p.box(Vector3(0.6f, 0.2f, 0.4f), Vector3(0.4f, 0.4f, 0.35f), "cardboard", Vector3(0.0f, 0.3f, 0.0f));
}

void car(LevelBuilder &b, const Vector3 &pos, float yaw, const char *paint, bool lights_on) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.55f, 0), Vector3(1.72f, 0.55f, 4.2f), paint);
	p.box(Vector3(0, 0.83f, -1.55f), Vector3(1.68f, 0.06f, 1.1f), paint, Vector3(0.06f, 0, 0));
	p.box(Vector3(0, 0.83f, 1.75f), Vector3(1.68f, 0.06f, 0.7f), paint, Vector3(-0.08f, 0, 0));
	p.box(Vector3(0, 1.1f, 0.25f), Vector3(1.5f, 0.5f, 2.0f), paint);
	p.box(Vector3(0, 1.1f, -0.77f), Vector3(1.42f, 0.44f, 0.04f), "glass_dirty", Vector3(-0.55f, 0, 0));
	p.box(Vector3(0, 1.1f, 1.27f), Vector3(1.42f, 0.42f, 0.04f), "glass_dirty", Vector3(0.5f, 0, 0));
	for (float sx : { -0.76f, 0.76f }) {
		p.box(Vector3(sx, 1.12f, 0.25f), Vector3(0.02f, 0.36f, 1.8f), "glass_dirty");
		for (float sz : { -1.35f, 1.3f }) {
			p.cyl(Vector3(sx, 0.32f, sz), 0.32f, 0.2f, "tire", Vector3(0, 0, PI * 0.5f), 14);
			p.cyl(Vector3(sx * 1.08f, 0.32f, sz), 0.18f, 0.03f, "steel", Vector3(0, 0, PI * 0.5f), 10);
		}
		p.box(Vector3(sx * 1.08f, 1.0f, -0.65f), Vector3(0.12f, 0.08f, 0.04f), paint);
	}
	for (float sx : { -0.6f, 0.6f }) {
		p.box(Vector3(sx, 0.68f, -2.11f), Vector3(0.3f, 0.12f, 0.03f), lights_on ? "car_light" : "plastic_white");
		p.box(Vector3(sx, 0.72f, 2.11f), Vector3(0.3f, 0.1f, 0.03f), "car_tail");
	}
	p.box(Vector3(0, 0.42f, -2.15f), Vector3(1.74f, 0.15f, 0.1f), "plastic_dark");
	p.box(Vector3(0, 0.42f, 2.15f), Vector3(1.74f, 0.15f, 0.1f), "plastic_dark");
	p.box(Vector3(0, 0.5f, -2.17f), Vector3(0.52f, 0.11f, 0.01f), "white_paint");
	p.collider(Vector3(0, 0.75f, 0), Vector3(1.8f, 1.3f, 4.3f));
}

void uaz_van(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 1.15f, 0), Vector3(1.94f, 1.5f, 4.4f), "car_green");
	p.box(Vector3(0, 1.9f, 0.2f), Vector3(1.8f, 0.05f, 3.9f), "car_green");
	p.box(Vector3(0, 1.45f, -2.21f), Vector3(1.6f, 0.55f, 0.03f), "glass_dirty");
	for (float sx : { -0.98f, 0.98f }) {
		p.box(Vector3(sx, 1.5f, -1.4f), Vector3(0.02f, 0.45f, 0.7f), "glass_dirty");
		p.box(Vector3(sx, 1.5f, 0.3f), Vector3(0.02f, 0.4f, 1.6f), "glass_dirty");
		for (float sz : { -1.45f, 1.45f }) {
			p.cyl(Vector3(sx, 0.38f, sz), 0.38f, 0.26f, "tire", Vector3(0, 0, PI * 0.5f), 14);
			p.cyl(Vector3(sx * 1.08f, 0.38f, sz), 0.2f, 0.03f, "steel", Vector3(0, 0, PI * 0.5f), 10);
		}
	}
	p.box(Vector3(0, 0.55f, -2.25f), Vector3(1.9f, 0.2f, 0.12f), "black");
	for (float sx : { -0.65f, 0.65f }) {
		p.cyl(Vector3(sx, 0.95f, -2.22f), 0.11f, 0.04f, "car_light", Vector3(PI * 0.5f, 0, 0), 12);
	}
	p.box(Vector3(0, 1.15f, -2.22f), Vector3(0.4f, 0.18f, 0.02f), "black");
	p.label(Vector3(1.0f, 1.1f, 0.4f), Vector3(1, 0, 0), "ОХРАНА · ГБР"_u, 0.22f, Color(0.95f, 0.85f, 0.2f));
	p.label(Vector3(-1.0f, 1.1f, 0.4f), Vector3(-1, 0, 0), "ОХРАНА · ГБР"_u, 0.22f, Color(0.95f, 0.85f, 0.2f));
	p.collider(Vector3(0, 1.0f, 0), Vector3(2.0f, 2.0f, 4.5f));
}

void truck(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 1.45f, -2.2f), Vector3(2.1f, 1.3f, 1.6f), "car_green");
	p.box(Vector3(0, 1.0f, -3.4f), Vector3(1.6f, 0.6f, 1.0f), "car_green", Vector3(0.08f, 0, 0));
	p.box(Vector3(0, 1.8f, -3.01f), Vector3(1.8f, 0.55f, 0.03f), "glass_dirty", Vector3(-0.15f, 0, 0));
	p.box(Vector3(0, 1.05f, 1.0f), Vector3(2.3f, 0.12f, 4.6f), "rust_local");
	p.box(Vector3(0, 1.4f, 3.25f), Vector3(2.3f, 0.6f, 0.06f), "wood_local");
	for (float sx : { -1.13f, 1.13f }) {
		p.box(Vector3(sx, 1.4f, 1.0f), Vector3(0.06f, 0.6f, 4.6f), "wood_local");
		for (float sz : { -3.0f, 1.0f, 2.3f }) {
			p.cyl(Vector3(sx * 0.9f, 0.45f, sz), 0.45f, 0.3f, "tire", Vector3(0, 0, PI * 0.5f), 14);
		}
	}
	p.box(Vector3(0, 0.75f, -0.5f), Vector3(0.6f, 0.2f, 6.5f), "black");
	p.collider(Vector3(0, 1.2f, -0.2f), Vector3(2.4f, 2.4f, 7.2f));
}

void bench(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	for (float sx : { -0.75f, 0.75f }) {
		p.box(Vector3(sx, 0.22f, 0), Vector3(0.12f, 0.44f, 0.45f), "concrete");
		p.box(Vector3(sx, 0.62f, 0.2f), Vector3(0.08f, 0.5f, 0.06f), "steel");
	}
	for (int i = 0; i < 3; i++) {
		p.box(Vector3(0, 0.46f, -0.15f + float(i) * 0.15f), Vector3(1.8f, 0.04f, 0.11f), "wood_local");
	}
	for (int i = 0; i < 2; i++) {
		p.box(Vector3(0, 0.68f + float(i) * 0.16f, 0.22f), Vector3(1.8f, 0.1f, 0.03f), "wood_local", Vector3(0.15f, 0, 0));
	}
	p.collider(Vector3(0, 0.25f, 0), Vector3(1.8f, 0.5f, 0.5f));
}

void trash_container(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.62f, 0), Vector3(1.3f, 0.95f, 0.9f), "metal");
	p.box(Vector3(0, 1.13f, 0.05f), Vector3(1.34f, 0.05f, 0.95f), "metal", Vector3(-0.18f, 0, 0));
	for (float sx : { -0.55f, 0.55f }) {
		for (float sz : { -0.35f, 0.35f }) {
			p.cyl(Vector3(sx, 0.08f, sz), 0.08f, 0.05f, "tire", Vector3(0, 0, PI * 0.5f));
		}
	}
	p.box(Vector3(0.4f, 1.18f, -0.1f), Vector3(0.4f, 0.25f, 0.3f), "plastic_dark", Vector3(0.1f, 0.3f, 0.2f));
	p.collider(Vector3(0, 0.6f, 0), Vector3(1.3f, 1.2f, 0.9f));
}

void swing(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	for (float sx : { -1.0f, 1.0f }) {
		p.pipe(Vector3(sx, 0.0f, -0.7f), Vector3(sx, 2.3f, 0.0f), 0.04f, "orange_paint");
		p.pipe(Vector3(sx, 0.0f, 0.7f), Vector3(sx, 2.3f, 0.0f), 0.04f, "orange_paint");
	}
	p.pipe(Vector3(-1.0f, 2.3f, 0.0f), Vector3(1.0f, 2.3f, 0.0f), 0.05f, "orange_paint");
	for (float sx : { -0.25f, 0.25f }) {
		p.pipe(Vector3(sx, 2.3f, 0.0f), Vector3(sx, 0.5f, 0.05f), 0.008f, "steel");
	}
	p.box(Vector3(0, 0.5f, 0.05f), Vector3(0.55f, 0.04f, 0.25f), "wood_local");
	p.collider(Vector3(0, 1.1f, 0), Vector3(2.1f, 2.3f, 0.2f));
}

void slide(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.7f, -0.8f), Vector3(0.9f, 1.4f, 0.9f), "plastic_red");
	p.box(Vector3(0, 0.75f, 0.6f), Vector3(0.6f, 0.05f, 2.2f), "steel", Vector3(-0.55f, 0, 0));
	for (int i = 0; i < 5; i++) {
		p.box(Vector3(0, 0.25f + float(i) * 0.28f, -1.35f), Vector3(0.6f, 0.04f, 0.2f), "steel");
	}
	p.collider(Vector3(0, 0.7f, -0.8f), Vector3(0.9f, 1.4f, 0.9f));
}

void birch(LevelBuilder &b, const Vector3 &pos, float height, uint32_t seed) {
	P p(b, pos, frand(seed) * 6.28f);
	float lean = (frand(seed) - 0.5f) * 0.12f;
	float trunk_r = 0.12f + height * 0.006f;
	p.cyl(Vector3(0, height * 0.35f, 0), trunk_r, height * 0.7f, "birch_bark", Vector3(lean, 0, 0), 8, trunk_r * 0.6f);
	for (int i = 0; i < 9; i++) {
		float y = 0.6f + frand(seed) * height * 0.6f;
		p.box(Vector3(0, y, 0), Vector3(trunk_r * 2.05f, 0.05f + frand(seed) * 0.06f, trunk_r * 0.7f), "black", Vector3(0, frand(seed) * 6.28f, 0));
	}
	for (int i = 0; i < 5; i++) {
		float a = frand(seed) * 6.28f;
		float y = height * (0.45f + frand(seed) * 0.3f);
		Vector3 start(0, y, 0);
		Vector3 end(std::cos(a) * height * 0.22f, y + height * 0.2f, std::sin(a) * height * 0.22f);
		p.pipe(start, end, 0.04f, "birch_bark");
	}
	for (int i = 0; i < 6; i++) {
		float a = frand(seed) * 6.28f;
		float r = frand(seed) * height * 0.18f;
		float s = height * (0.13f + frand(seed) * 0.08f);
		const char *m = frand(seed) > 0.4f ? "tree_crown_autumn" : "tree_crown";
		p.sphere(Vector3(std::cos(a) * r, height * (0.62f + frand(seed) * 0.3f), std::sin(a) * r), s, m, Vector3(1.0f, 0.8f, 1.0f));
	}
	Transform3D base = p.at(Vector3(0, height * 0.3f, 0));
	b.collider_box(base.origin, base.basis, Vector3(trunk_r * 2.0f, height * 0.6f, trunk_r * 2.0f));
}

void soviet_lamp(LevelBuilder &b, const Vector3 &pos, float yaw, bool lit) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 3.5f, 0), Vector3(0.18f, 7.0f, 0.12f), "concrete_dark");
	p.box(Vector3(0, 0.3f, 0), Vector3(0.26f, 0.6f, 0.2f), "concrete_dark");
	p.pipe(Vector3(0, 6.8f, 0), Vector3(0, 7.2f, -0.6f), 0.04f, "metal_gray");
	p.pipe(Vector3(0, 7.2f, -0.6f), Vector3(0, 7.15f, -1.3f), 0.04f, "metal_gray");
	p.box(Vector3(0, 7.08f, -1.45f), Vector3(0.28f, 0.14f, 0.55f), "metal_gray");
	p.box(Vector3(0, 7.0f, -1.45f), Vector3(0.22f, 0.03f, 0.42f), lit ? "lamp_warm" : "plastic_dark");
	p.collider(Vector3(0, 3.5f, 0), Vector3(0.26f, 7.0f, 0.2f));
}

void radiator(LevelBuilder &b, const Vector3 &pos, float yaw, int sections) {
	P p(b, pos, yaw);
	float w = float(sections) * 0.09f;
	for (int i = 0; i < sections; i++) {
		float x = -w * 0.5f + 0.045f + float(i) * 0.09f;
		p.box(Vector3(x, 0.45f, 0), Vector3(0.07f, 0.5f, 0.09f), "white_paint");
	}
	p.pipe(Vector3(-w * 0.5f - 0.2f, 0.25f, 0.02f), Vector3(w * 0.5f, 0.25f, 0.02f), 0.02f, "white_paint");
	p.pipe(Vector3(-w * 0.5f - 0.2f, 0.65f, 0.02f), Vector3(w * 0.5f, 0.65f, 0.02f), 0.02f, "white_paint");
}

void garbage_chute(LevelBuilder &b, const Vector3 &pos, float yaw, float height) {
	P p(b, pos, yaw);
	p.cyl(Vector3(0, height * 0.5f, 0), 0.24f, height, "metal_gray", Vector3(), 14);
	p.box(Vector3(0, 0.9f, -0.26f), Vector3(0.42f, 0.4f, 0.12f), "metal_gray_local");
	p.box(Vector3(0, 0.93f, -0.33f), Vector3(0.36f, 0.06f, 0.04f), "black");
	p.collider(Vector3(0, height * 0.5f, 0), Vector3(0.5f, height, 0.5f));
}

void electric_panel(LevelBuilder &b, const Vector3 &pos, float yaw, bool open) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0, 0), Vector3(0.6f, 0.9f, 0.18f), "metal_gray_local");
	if (open) {
		p.box(Vector3(-0.5f, 0, -0.32f), Vector3(0.58f, 0.88f, 0.02f), "metal_gray_local", Vector3(0, -1.2f, 0));
		for (int i = 0; i < 6; i++) {
			p.box(Vector3(-0.2f + float(i) * 0.08f, 0.15f, -0.08f), Vector3(0.05f, 0.1f, 0.04f), "plastic_white");
		}
		p.box(Vector3(0, -0.15f, -0.08f), Vector3(0.25f, 0.15f, 0.04f), "black");
		p.pipe(Vector3(-0.15f, -0.3f, -0.07f), Vector3(-0.1f, -0.6f, -0.05f), 0.01f, "black");
		p.pipe(Vector3(0.1f, -0.3f, -0.07f), Vector3(0.2f, -0.65f, -0.1f), 0.01f, "red_paint");
	} else {
		p.box(Vector3(0, 0, -0.095f), Vector3(0.58f, 0.88f, 0.01f), "metal_gray_local");
	}
	p.label(Vector3(0.12f, 0.3f, -0.105f), Vector3(0, 0, -1), "Щ-12"_u, 0.07f, Color(0.9f, 0.85f, 0.2f));
}

void water_tank(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.cyl(Vector3(0, 0.95f, 0), 0.6f, 2.2f, "metal_blue", Vector3(0, 0, PI * 0.5f), 18);
	for (float x : { -0.7f, 0.7f }) {
		p.box(Vector3(x, 0.25f, 0), Vector3(0.12f, 0.5f, 1.0f), "metal_gray");
	}
	p.pipe(Vector3(1.1f, 0.6f, 0), Vector3(1.4f, 0.6f, 0), 0.03f, "steel");
	p.label(Vector3(0, 1.05f, -0.61f), Vector3(0, 0, -1), "ВОДА ПИТЬЕВАЯ"_u, 0.12f, Color(0.95f, 0.95f, 0.9f));
	p.collider(Vector3(0, 0.8f, 0), Vector3(2.2f, 1.6f, 1.2f));
}

void valve(LevelBuilder &b, const Vector3 &pos, const Vector3 &axis, float radius) {
	Vector3 a = axis.normalized();
	Vector3 ref = std::fabs(a.y) > 0.9f ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
	Vector3 x = ref.cross(a).normalized();
	Vector3 z = x.cross(a).normalized();
	Basis bs(x, a, z);
	Ref<TorusMesh> t;
	t.instantiate();
	t->set_inner_radius(radius * 0.85f);
	t->set_outer_radius(radius);
	t->set_rings(16);
	t->set_ring_segments(6);
	b.visual_mesh(t, Transform3D(bs, pos), "red_paint");
	for (int i = 0; i < 3; i++) {
		float ang = float(i) * PI / 3.0f;
		Vector3 d = x * std::cos(ang) + z * std::sin(ang);
		b.pipe(pos - d * radius * 0.9f, pos + d * radius * 0.9f, 0.01f, "red_paint");
	}
	b.pipe(pos, pos - a * 0.15f, 0.02f, "steel");
}

void vent_duct(LevelBuilder &b, const Vector3 &a, const Vector3 &c, float width, float height, float ceiling) {
	Vector3 d = c - a;
	float len = d.length();
	if (len < 0.1f) {
		return;
	}
	Vector3 dir = d / len;
	float yaw = std::atan2(-dir.x, -dir.z);
	Basis bs(Vector3(0, 1, 0), yaw);
	b.visual_box((a + c) * 0.5f, Vector3(width, height, len), "metal_gray", bs);
	int flanges = int(len / 1.5f);
	for (int i = 0; i <= flanges; i++) {
		Vector3 p = a + dir * (len * float(i) / float(std::max(1, flanges)));
		b.visual_box(p, Vector3(width + 0.06f, height + 0.06f, 0.04f), "metal_gray", bs, false);
		if (i % 2 == 0) {
			Vector3 side = bs.xform(Vector3(width * 0.5f + 0.03f, 0, 0));
			b.pipe(p + side + Vector3(0, height * 0.5f, 0), Vector3(p.x + side.x, ceiling, p.z + side.z), 0.008f, "steel");
			b.pipe(p - side + Vector3(0, height * 0.5f, 0), Vector3(p.x - side.x, ceiling, p.z - side.z), 0.008f, "steel");
		}
	}
}

void cable_tray(LevelBuilder &b, const Vector3 &a, const Vector3 &c) {
	Vector3 d = c - a;
	float len = d.length();
	if (len < 0.1f) {
		return;
	}
	Vector3 dir = d / len;
	float yaw = std::atan2(-dir.x, -dir.z);
	Basis bs(Vector3(0, 1, 0), yaw);
	b.visual_box((a + c) * 0.5f, Vector3(0.3f, 0.02f, len), "metal_gray", bs, false);
	Vector3 side = bs.xform(Vector3(1, 0, 0));
	for (int i = 0; i < 3; i++) {
		Vector3 off = side * (-0.08f + float(i) * 0.08f) + Vector3(0, 0.03f, 0);
		b.pipe(a + off, c + off, 0.018f, i == 1 ? "plastic_dark" : "black");
	}
}

void hanging_cable(LevelBuilder &b, const Vector3 &a, const Vector3 &c, float sag) {
	const int segs = 8;
	Vector3 prev = a;
	for (int i = 1; i <= segs; i++) {
		float t = float(i) / float(segs);
		Vector3 pt = a.lerp(c, t) - Vector3(0, sag * 4.0f * t * (1.0f - t), 0);
		b.pipe(prev, pt, 0.012f, "black");
		prev = pt;
	}
}

void stretcher(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	for (float sx : { -0.27f, 0.27f }) {
		p.pipe(Vector3(sx, 0.12f, -1.1f), Vector3(sx, 0.12f, 1.1f), 0.02f, "wood_local");
		p.box(Vector3(sx, 0.06f, -0.7f), Vector3(0.03f, 0.12f, 0.03f), "steel");
		p.box(Vector3(sx, 0.06f, 0.7f), Vector3(0.03f, 0.12f, 0.03f), "steel");
	}
	p.box(Vector3(0, 0.13f, 0), Vector3(0.52f, 0.01f, 1.8f), "canvas");
}

void sink(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.8f, 0), Vector3(0.55f, 0.16f, 0.42f), "plastic_white");
	p.box(Vector3(0, 0.86f, -0.02f), Vector3(0.44f, 0.06f, 0.3f), "black");
	p.box(Vector3(0, 0.4f, 0.1f), Vector3(0.12f, 0.7f, 0.12f), "plastic_white");
	p.pipe(Vector3(0, 0.92f, 0.16f), Vector3(0, 1.05f, 0.16f), 0.015f, "chrome");
	p.pipe(Vector3(0, 1.05f, 0.16f), Vector3(0, 1.02f, 0.02f), 0.012f, "chrome");
	p.box(Vector3(0, 1.45f, 0.19f), Vector3(0.45f, 0.55f, 0.02f), "glass_dirty");
}

void wall_phone(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0, 0), Vector3(0.2f, 0.26f, 0.09f), "plastic_dark");
	p.capsule(Vector3(0.12f, 0.0f, -0.03f), 0.03f, 0.22f, "plastic_dark");
	p.cyl(Vector3(0, 0.03f, -0.05f), 0.06f, 0.01f, "plastic_white", Vector3(PI * 0.5f, 0, 0));
	p.pipe(Vector3(0.12f, -0.1f, -0.03f), Vector3(0.05f, -0.25f, -0.04f), 0.006f, "black");
}

void wall_clock(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.cyl(Vector3(0, 0, 0), 0.16f, 0.05f, "black", Vector3(PI * 0.5f, 0, 0), 16);
	p.cyl(Vector3(0, 0, -0.026f), 0.14f, 0.005f, "white_paint", Vector3(PI * 0.5f, 0, 0), 16);
	p.box(Vector3(0.0f, 0.04f, -0.03f), Vector3(0.012f, 0.09f, 0.004f), "black", Vector3(0, 0, -0.4f));
	p.box(Vector3(0.03f, 0.0f, -0.032f), Vector3(0.012f, 0.12f, 0.004f), "black", Vector3(0, 0, 1.2f));
}

void fire_shield(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 1.1f, 0), Vector3(1.2f, 1.4f, 0.04f), "red_paint");
	p.box(Vector3(0, 1.1f, -0.025f), Vector3(1.1f, 1.3f, 0.01f), "white_paint");
	p.cyl(Vector3(-0.35f, 1.45f, -0.12f), 0.16f, 0.32f, "red_paint", Vector3(PI, 0, 0), 12, 0.0f);
	p.box(Vector3(0.0f, 1.1f, -0.06f), Vector3(0.05f, 1.0f, 0.03f), "wood_local");
	p.box(Vector3(0.0f, 0.55f, -0.07f), Vector3(0.24f, 0.28f, 0.02f), "metal_gray_local");
	p.box(Vector3(0.35f, 1.25f, -0.06f), Vector3(0.04f, 0.7f, 0.03f), "wood_local");
	p.prism(Vector3(0.35f, 1.62f, -0.06f), Vector3(0.18f, 0.1f, 0.02f), "red_paint", Vector3(0, 0, 0));
	p.pipe(Vector3(-0.35f, 0.5f, -0.06f), Vector3(-0.35f, 1.05f, -0.06f), 0.012f, "red_paint");
	p.label(Vector3(0, 1.75f, -0.03f), Vector3(0, 0, -1), "ПОЖАРНЫЙ ЩИТ"_u, 0.07f, Color(0.95f, 0.95f, 0.9f));
	p.box(Vector3(0.75f, 0.35f, -0.25f), Vector3(0.6f, 0.7f, 0.45f), "red_paint");
	p.label(Vector3(0.75f, 0.45f, -0.48f), Vector3(0, 0, -1), "ПЕСОК"_u, 0.08f, Color(0.95f, 0.95f, 0.9f));
	p.collider(Vector3(0.75f, 0.35f, -0.25f), Vector3(0.6f, 0.7f, 0.45f));
}

void extinguisher(LevelBuilder &b, const Vector3 &pos) {
	P p(b, pos, 0.0f);
	p.cyl(Vector3(0, 0.3f, 0), 0.08f, 0.6f, "red_paint", Vector3(), 12);
	p.sphere(Vector3(0, 0.6f, 0), 0.08f, "red_paint", Vector3(1.0f, 0.6f, 1.0f));
	p.box(Vector3(0, 0.68f, 0), Vector3(0.04f, 0.06f, 0.1f), "black");
	p.pipe(Vector3(0, 0.66f, 0.05f), Vector3(0.05f, 0.3f, 0.1f), 0.012f, "black");
}

void lockers(LevelBuilder &b, const Vector3 &pos, float yaw, int count) {
	P p(b, pos, yaw);
	float w = 0.4f;
	for (int i = 0; i < count; i++) {
		float x = (float(i) - float(count - 1) * 0.5f) * w;
		p.box(Vector3(x, 0.9f, 0), Vector3(w - 0.01f, 1.8f, 0.5f), "metal_blue");
		for (int k = 0; k < 3; k++) {
			p.box(Vector3(x, 1.55f - float(k) * 0.04f, -0.252f), Vector3(0.25f, 0.015f, 0.005f), "black");
		}
		p.box(Vector3(x + 0.13f, 0.95f, -0.255f), Vector3(0.02f, 0.1f, 0.01f), "steel");
	}
	p.collider(Vector3(0, 0.9f, 0), Vector3(w * float(count), 1.8f, 0.5f));
}

void crates(LevelBuilder &b, const Vector3 &pos, float yaw, int count, const String &label, uint32_t seed) {
	P p(b, pos, yaw);
	for (int i = 0; i < count; i++) {
		int col = i % 3;
		int row = i / 3;
		Vector3 c(float(col) * 0.62f, 0.2f + float(row) * 0.4f, (frand(seed) - 0.5f) * 0.06f);
		p.box(c, Vector3(0.58f, 0.38f, 0.42f), "wood_local", Vector3(0, (frand(seed) - 0.5f) * 0.1f, 0));
		if (!label.is_empty()) {
			p.label(c + Vector3(0, 0.02f, -0.215f), Vector3(0, 0, -1), label, 0.05f, Color(0.15f, 0.12f, 0.1f));
		}
	}
	int cols = std::min(count, 3);
	int rows = (count + 2) / 3;
	p.collider(Vector3(float(cols - 1) * 0.31f, float(rows) * 0.2f, 0), Vector3(float(cols) * 0.62f, float(rows) * 0.4f, 0.45f));
}

void window_frame(LevelBuilder &b, const Vector3 &center, float yaw, float width, float height, float broken) {
	P p(b, center, yaw);
	p.shadows = false;
	float hw = width * 0.5f;
	float hh = height * 0.5f;
	p.box(Vector3(-hw + 0.03f, 0, 0), Vector3(0.05f, height, 0.07f), "rust_local");
	p.box(Vector3(hw - 0.03f, 0, 0), Vector3(0.05f, height, 0.07f), "rust_local");
	p.box(Vector3(0, hh - 0.03f, 0), Vector3(width, 0.05f, 0.07f), "rust_local");
	p.box(Vector3(0, -hh + 0.03f, 0), Vector3(width, 0.05f, 0.07f), "rust_local");
	p.box(Vector3(0, 0, 0), Vector3(0.04f, height, 0.05f), "rust_local");
	p.box(Vector3(0, hh * 0.35f, 0), Vector3(width, 0.04f, 0.05f), "rust_local");
	uint32_t seed = uint32_t(std::fabs(center.x * 13.0f + center.z * 7.0f)) + 1;
	for (int i = 0; i < 4; i++) {
		if (frand(seed) < broken) {
			continue;
		}
		float sx = (i % 2 == 0) ? -hw * 0.5f : hw * 0.5f;
		float sy = (i < 2) ? hh * 0.67f : -hh * 0.3f;
		float ph = i < 2 ? hh * 0.6f : hh * 1.3f;
		p.box(Vector3(sx, sy, 0.0f), Vector3(hw - 0.06f, ph - 0.05f, 0.005f), "glass_dirty");
	}
}

void rebar(LevelBuilder &b, const Vector3 &pos, const Vector3 &dir, int count, uint32_t seed) {
	Vector3 d = dir.normalized();
	Vector3 side = Vector3(0, 1, 0).cross(d);
	if (side.length() < 0.1f) {
		side = Vector3(1, 0, 0);
	}
	side.normalize();
	for (int i = 0; i < count; i++) {
		Vector3 start = pos + side * ((float(i) - float(count - 1) * 0.5f) * 0.2f);
		float len = 0.3f + frand(seed) * 0.6f;
		Vector3 mid = start + d * len * 0.6f;
		Vector3 end = mid + (d * 0.4f + Vector3(0, -0.6f - frand(seed) * 0.5f, 0) + side * (frand(seed) - 0.5f)) * len * 0.6f;
		b.pipe(start, mid, 0.009f, "rust");
		b.pipe(mid, end, 0.009f, "rust");
	}
}

void balcony(LevelBuilder &b, const Vector3 &pos, float yaw, bool glazed, uint32_t seed) {
	P p(b, pos, yaw);
	p.box(Vector3(0, -0.08f, -0.6f), Vector3(2.6f, 0.16f, 1.2f), "concrete");
	if (glazed) {
		p.box(Vector3(0, 0.5f, -1.18f), Vector3(2.6f, 1.0f, 0.06f), "white_paint");
		p.box(Vector3(0, 1.6f, -1.18f), Vector3(2.6f, 1.2f, 0.03f), "glass_dirty");
		for (float x : { -1.28f, -0.43f, 0.43f, 1.28f }) {
			p.box(Vector3(x, 1.6f, -1.18f), Vector3(0.05f, 1.2f, 0.06f), "white_paint");
		}
		p.box(Vector3(0, 2.23f, -0.6f), Vector3(2.6f, 0.06f, 1.2f), "white_paint");
		for (float sx : { -1.28f, 1.28f }) {
			p.box(Vector3(sx, 1.1f, -0.6f), Vector3(0.06f, 2.2f, 1.2f), "white_paint");
		}
	} else {
		p.box(Vector3(0, 0.5f, -1.18f), Vector3(2.6f, 1.0f, 0.05f), frand(seed) > 0.5f ? "fence_gray" : "metal");
		for (float sx : { -1.28f, 1.28f }) {
			p.box(Vector3(sx, 0.5f, -0.6f), Vector3(0.05f, 1.0f, 1.2f), "metal");
		}
		if (frand(seed) > 0.5f) {
			p.pipe(Vector3(-1.2f, 1.7f, -1.1f), Vector3(1.2f, 1.7f, -1.1f), 0.01f, "steel");
			p.box(Vector3(-0.4f, 1.4f, -1.1f), Vector3(0.5f, 0.6f, 0.01f), frand(seed) > 0.5f ? "cloth_stained" : "plastic_blue");
		}
	}
}

void ac_unit(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0, -0.15f), Vector3(0.8f, 0.55f, 0.3f), "plastic_white");
	p.cyl(Vector3(0.12f, 0, -0.31f), 0.18f, 0.02f, "black", Vector3(PI * 0.5f, 0, 0), 14);
	p.box(Vector3(0, -0.35f, -0.15f), Vector3(0.7f, 0.04f, 0.3f), "steel");
}

void antenna(LevelBuilder &b, const Vector3 &pos, float height) {
	P p(b, pos, 0.0f);
	p.pipe(Vector3(0, 0, 0), Vector3(0, height, 0), 0.025f, "steel");
	for (int i = 0; i < 4; i++) {
		float y = height * (0.55f + float(i) * 0.12f);
		float w = 0.9f - float(i) * 0.15f;
		p.pipe(Vector3(-w * 0.5f, y, 0), Vector3(w * 0.5f, y, 0), 0.01f, "steel");
	}
	p.pipe(Vector3(0, height * 0.5f, 0), Vector3(0, height * 0.5f, -0.9f), 0.012f, "steel");
	p.pipe(Vector3(0, height, 0), Vector3(1.2f, 0.0f, 0.8f), 0.004f, "black");
}

void satellite_dish(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.sphere(Vector3(0, 0, 0), 0.32f, "plastic_white", Vector3(1.0f, 1.0f, 0.18f));
	p.pipe(Vector3(0, -0.2f, -0.02f), Vector3(0, -0.05f, -0.45f), 0.012f, "steel");
	p.box(Vector3(0, -0.05f, -0.47f), Vector3(0.05f, 0.05f, 0.08f), "plastic_dark");
	p.pipe(Vector3(0, 0, 0.06f), Vector3(0, 0, 0.3f), 0.02f, "steel");
}

void tec_chimney(LevelBuilder &b, const Vector3 &pos, float height, float radius) {
	P p(b, pos, 0.0f);
	int bands = 8;
	float band_h = height / float(bands);
	for (int i = 0; i < bands; i++) {
		float r0 = radius * (1.0f - 0.35f * float(i) / float(bands));
		float r1 = radius * (1.0f - 0.35f * float(i + 1) / float(bands));
		p.cyl(Vector3(0, band_h * (float(i) + 0.5f), 0), r0, band_h, i % 2 == 0 ? "red_paint" : "white_paint", Vector3(), 16, r1);
	}
	for (float y : { height * 0.5f, height * 0.98f }) {
		for (int k = 0; k < 4; k++) {
			float a = float(k) * PI * 0.5f;
			float r = radius * (1.0f - 0.35f * y / height) + 0.3f;
			p.sphere(Vector3(std::cos(a) * r, y, std::sin(a) * r), 0.6f, "red_light");
		}
	}
}

void shipping_container(LevelBuilder &b, const Vector3 &pos, float yaw, const char *material) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 1.3f, 0), Vector3(2.44f, 2.6f, 6.06f), material);
	for (float sz : { -3.04f, 3.04f }) {
		p.box(Vector3(0, 1.3f, sz), Vector3(2.5f, 2.62f, 0.06f), "metal_gray");
	}
	for (float sx : { -0.5f, -0.2f, 0.2f, 0.5f }) {
		p.pipe(Vector3(sx, 0.1f, 3.08f), Vector3(sx, 2.5f, 3.08f), 0.025f, "steel");
	}
	p.collider(Vector3(0, 1.3f, 0), Vector3(2.44f, 2.6f, 6.06f));
}

void pallets(LevelBuilder &b, const Vector3 &pos, float yaw, int count) {
	P p(b, pos, yaw);
	for (int i = 0; i < count; i++) {
		float y = float(i) * 0.15f;
		for (int k = 0; k < 5; k++) {
			p.box(Vector3(-0.48f + float(k) * 0.24f, y + 0.13f, 0), Vector3(0.12f, 0.02f, 1.2f), "wood_local");
		}
		for (float z : { -0.5f, 0.0f, 0.5f }) {
			p.box(Vector3(0, y + 0.06f, z), Vector3(1.0f, 0.1f, 0.1f), "wood_local");
		}
	}
	p.collider(Vector3(0, float(count) * 0.075f, 0), Vector3(1.0f, float(count) * 0.15f, 1.2f));
}

void ritual_circle(LevelBuilder &b, const Vector3 &center, float radius) {
	P p(b, center, 0.0f);
	p.shadows = false;
	p.torus(Vector3(0, 0.005f, 0), radius - 0.06f, radius, "red_paint", Vector3());
	for (int i = 0; i < 5; i++) {
		float a0 = float(i) * PI * 2.0f / 5.0f;
		float a1 = float((i + 2) % 5) * PI * 2.0f / 5.0f;
		Vector3 s(std::sin(a0) * radius * 0.92f, 0.005f, std::cos(a0) * radius * 0.92f);
		Vector3 e(std::sin(a1) * radius * 0.92f, 0.005f, std::cos(a1) * radius * 0.92f);
		Vector3 d = e - s;
		float len = d.length();
		float yaw = std::atan2(-d.x, -d.z);
		b.visual_box(center + (s + e) * 0.5f, Vector3(0.04f, 0.004f, len), "red_paint", Basis(Vector3(0, 1, 0), yaw), false);
	}
	for (int i = 0; i < 7; i++) {
		float a = float(i) * PI * 2.0f / 7.0f + 0.3f;
		Vector3 c(std::sin(a) * (radius + 0.25f), 0.0f, std::cos(a) * (radius + 0.25f));
		p.cyl(c + Vector3(0, 0.06f, 0), 0.025f, 0.12f - float(i % 3) * 0.03f, "paper", Vector3(), 8);
	}
}

void mailboxes(LevelBuilder &b, const Vector3 &pos, float yaw, int columns, int rows) {
	P p(b, pos, yaw);
	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < columns; c++) {
			Vector3 cc(-float(columns - 1) * 0.15f + float(c) * 0.3f, -float(r) * 0.32f, 0);
			p.box(cc, Vector3(0.29f, 0.31f, 0.18f), "metal_blue");
			p.box(cc + Vector3(0, 0.08f, -0.092f), Vector3(0.18f, 0.025f, 0.005f), "black");
			p.box(cc + Vector3(0.1f, -0.05f, -0.095f), Vector3(0.025f, 0.03f, 0.01f), "steel");
		}
	}
}

void flower_pot(LevelBuilder &b, const Vector3 &pos) {
	P p(b, pos, 0.0f);
	p.cyl(Vector3(0, 0.12f, 0), 0.12f, 0.24f, "orange_paint", Vector3(), 10, 0.16f);
	p.sphere(Vector3(0, 0.35f, 0), 0.17f, "tree_crown", Vector3(1.0f, 0.8f, 1.0f));
}

void doormat(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.shadows = false;
	p.box(Vector3(0, 0.006f, 0), Vector3(0.8f, 0.012f, 0.5f), "rubber");
}

void wheel_valve_door_frame(LevelBuilder &b, const Vector3 &pos, float yaw, float width, float height) {
	P p(b, pos, yaw);
	float t = 0.14f;
	p.box(Vector3(-width * 0.5f - t * 0.5f, height * 0.5f, 0), Vector3(t, height + t, 0.55f), "metal_gray");
	p.box(Vector3(width * 0.5f + t * 0.5f, height * 0.5f, 0), Vector3(t, height + t, 0.55f), "metal_gray");
	p.box(Vector3(0, height + t * 0.5f, 0), Vector3(width + t * 2.0f, t, 0.55f), "metal_gray");
	p.box(Vector3(0, 0.03f, 0), Vector3(width + t * 2.0f, 0.06f, 0.55f), "metal_gray");
	for (float side : { -1.0f, 1.0f }) {
		for (float y : { 0.4f, height - 0.4f }) {
			p.cyl(Vector3(-width * 0.5f - t * 0.5f, y, side * 0.3f), 0.04f, 0.06f, "steel", Vector3(PI * 0.5f, 0, 0), 8);
		}
	}
}

void drain_pipe(LevelBuilder &b, const Vector3 &top, float height) {
	P p(b, top, 0.0f);
	p.cyl(Vector3(0, -height * 0.5f, 0), 0.06f, height, "metal_gray", Vector3(), 8);
	p.cyl(Vector3(0, 0.05f, 0), 0.12f, 0.2f, "metal_gray", Vector3(), 8, 0.06f);
	for (int i = 1; i < int(height / 2.0f); i++) {
		p.box(Vector3(0, -float(i) * 2.0f, 0.04f), Vector3(0.15f, 0.03f, 0.04f), "steel");
	}
	p.cyl(Vector3(0, -height + 0.1f, -0.1f), 0.06f, 0.3f, "metal_gray", Vector3(0.9f, 0, 0), 8);
}

void lightning_rod(LevelBuilder &b, const Vector3 &pos, float height) {
	P p(b, pos, 0.0f);
	p.pipe(Vector3(0, 0, 0), Vector3(0, height, 0), 0.015f, "steel");
	p.box(Vector3(0, 0.05f, 0), Vector3(0.2f, 0.1f, 0.2f), "concrete_dark");
}

void radio_station(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.18f, 0), Vector3(0.6f, 0.36f, 0.35f), "metal");
	p.box(Vector3(0, 0.2f, -0.178f), Vector3(0.56f, 0.3f, 0.005f), "black");
	for (int i = 0; i < 4; i++) {
		p.cyl(Vector3(-0.2f + float(i) * 0.13f, 0.12f, -0.19f), 0.03f, 0.03f, "plastic_white", Vector3(PI * 0.5f, 0, 0), 8);
	}
	p.box(Vector3(-0.1f, 0.27f, -0.185f), Vector3(0.3f, 0.07f, 0.005f), "lamp_warm");
	p.capsule(Vector3(0.38f, 0.1f, 0.0f), 0.04f, 0.2f, "plastic_dark", Vector3(0, 0, PI * 0.5f));
	p.pipe(Vector3(0.25f, 0.36f, 0.1f), Vector3(0.3f, 0.9f, 0.15f), 0.006f, "steel");
}

void tv_old(LevelBuilder &b, const Vector3 &pos, float yaw) {
	P p(b, pos, yaw);
	p.box(Vector3(0, 0.22f, 0), Vector3(0.55f, 0.44f, 0.45f), "wood_local");
	p.box(Vector3(-0.05f, 0.24f, -0.226f), Vector3(0.38f, 0.3f, 0.01f), "tv_glow");
	p.cyl(Vector3(0.21f, 0.3f, -0.23f), 0.025f, 0.02f, "plastic_dark", Vector3(PI * 0.5f, 0, 0), 8);
	p.cyl(Vector3(0.21f, 0.18f, -0.23f), 0.025f, 0.02f, "plastic_dark", Vector3(PI * 0.5f, 0, 0), 8);
	p.pipe(Vector3(0, 0.44f, 0.1f), Vector3(-0.2f, 0.8f, 0.15f), 0.004f, "steel");
	p.pipe(Vector3(0, 0.44f, 0.1f), Vector3(0.2f, 0.8f, 0.15f), 0.004f, "steel");
}

void kettle(LevelBuilder &b, const Vector3 &pos) {
	P p(b, pos, 0.0f);
	p.cyl(Vector3(0, 0.1f, 0), 0.09f, 0.2f, "chrome", Vector3(), 12, 0.07f);
	p.torus(Vector3(0, 0.2f, 0.07f), 0.05f, 0.065f, "black", Vector3(PI * 0.5f, 0, 0));
	p.cyl(Vector3(0, 0.15f, -0.11f), 0.015f, 0.12f, "chrome", Vector3(-0.8f, 0, 0), 6);
}

void puddle(LevelBuilder &b, const Vector3 &pos, float size, uint32_t seed) {
	P p(b, pos, frand(seed) * 6.28f);
	p.shadows = false;
	for (int i = 0; i < 3; i++) {
		float a = frand(seed) * 6.28f;
		float r = size * 0.3f * frand(seed);
		Vector3 c(std::cos(a) * r, 0.006f + float(i) * 0.001f, std::sin(a) * r);
		p.cyl(c, size * (0.35f + frand(seed) * 0.25f), 0.004f, "water", Vector3(), 16);
	}
}

}

}
