#include "core/builder.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/convex_polygon_shape3d.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/text_server.hpp>

#include <algorithm>

using namespace godot;

namespace urbex {

Basis basis_looking(const Vector3 &dir) {
	Vector3 z = -dir.normalized();
	Vector3 up = std::fabs(z.y) > 0.98f ? Vector3(0, 0, 1) : Vector3(0, 1, 0);
	Vector3 x = up.cross(z).normalized();
	Vector3 y = z.cross(x).normalized();
	return Basis(x, y, z);
}

LevelBuilder::LevelBuilder(Node3D *p_dynamic_root, Node3D *p_static_root, const MaterialLibrary *p_materials, uint64_t seed) :
		dynamic_root(p_dynamic_root), static_root(p_static_root), materials(p_materials), rng(seed) {
	parents.push_back(static_root);
	transforms.push_back(Transform3D());
}

Ref<StandardMaterial3D> LevelBuilder::mat(const char *name) const {
	return materials->get(name);
}

Node3D *LevelBuilder::current() const {
	return parents.back();
}

void LevelBuilder::push(const Transform3D &local) {
	Node3D *group = memnew(Node3D);
	group->set_transform(local);
	current()->add_child(group);
	parents.push_back(group);
	transforms.push_back(transforms.back() * local);
}

void LevelBuilder::push_yaw(const Vector3 &origin, float yaw) {
	push(Transform3D(Basis(Vector3(0, 1, 0), yaw), origin));
}

void LevelBuilder::pop() {
	if (parents.size() > 1) {
		parents.pop_back();
		transforms.pop_back();
	}
}

Vector3 LevelBuilder::g(const Vector3 &local) const {
	return transforms.back().xform(local);
}

Vector3 LevelBuilder::gdir(const Vector3 &local_dir) const {
	return transforms.back().basis.xform(local_dir);
}

float LevelBuilder::gyaw(float local_yaw) const {
	Vector3 f = gdir(yaw_forward(local_yaw));
	return std::atan2(-f.x, -f.z);
}

Transform3D LevelBuilder::gxf(const Transform3D &local) const {
	return transforms.back() * local;
}

StaticBody3D *LevelBuilder::box(const Vector3 &center, const Vector3 &size, const char *material, bool collide) {
	return box_rot(center, Basis(), size, material, collide);
}

StaticBody3D *LevelBuilder::box_rot(const Vector3 &center, const Basis &basis, const Vector3 &size, const char *material, bool collide) {
	if (!collide) {
		visual_box(center, size, material, basis);
		return nullptr;
	}
	StaticBody3D *body = memnew(StaticBody3D);
	body->set_collision_layer(layer::WORLD);
	body->set_collision_mask(0);
	body->set_transform(Transform3D(basis, center));
	current()->add_child(body);

	Ref<BoxShape3D> shape;
	shape.instantiate();
	shape->set_size(size);
	CollisionShape3D *cs = memnew(CollisionShape3D);
	cs->set_shape(shape);
	body->add_child(cs);

	Ref<BoxMesh> mesh;
	mesh.instantiate();
	mesh->set_size(size);
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(mat(material));
	body->add_child(mi);
	return body;
}

MeshInstance3D *LevelBuilder::visual_box(const Vector3 &center, const Vector3 &size, const char *material, const Basis &basis, bool shadows) {
	Ref<BoxMesh> mesh;
	mesh.instantiate();
	mesh->set_size(size);
	return visual_mesh(mesh, Transform3D(basis, center), material, shadows);
}

MeshInstance3D *LevelBuilder::visual_mesh(const Ref<Mesh> &mesh, const Transform3D &xf, const char *material, bool shadows) {
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(mat(material));
	mi->set_transform(xf);
	if (!shadows) {
		mi->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	}
	current()->add_child(mi);
	return mi;
}

StaticBody3D *LevelBuilder::collider_box(const Vector3 &center, const Basis &basis, const Vector3 &size) {
	StaticBody3D *body = memnew(StaticBody3D);
	body->set_collision_layer(layer::WORLD);
	body->set_collision_mask(0);
	body->set_transform(Transform3D(basis, center));
	current()->add_child(body);
	Ref<BoxShape3D> shape;
	shape.instantiate();
	shape->set_size(size);
	CollisionShape3D *cs = memnew(CollisionShape3D);
	cs->set_shape(shape);
	body->add_child(cs);
	return body;
}

MeshInstance3D *LevelBuilder::cylinder(const Vector3 &base, float radius, float height, const char *material, bool collide, int segments) {
	Ref<CylinderMesh> mesh;
	mesh.instantiate();
	mesh->set_top_radius(radius);
	mesh->set_bottom_radius(radius);
	mesh->set_height(height);
	mesh->set_radial_segments(segments);
	mesh->set_rings(1);
	Vector3 c = base + Vector3(0, height * 0.5f, 0);
	if (collide) {
		StaticBody3D *body = memnew(StaticBody3D);
		body->set_collision_layer(layer::WORLD);
		body->set_collision_mask(0);
		body->set_position(c);
		current()->add_child(body);
		Ref<CylinderShape3D> shape;
		shape.instantiate();
		shape->set_radius(radius);
		shape->set_height(height);
		CollisionShape3D *cs = memnew(CollisionShape3D);
		cs->set_shape(shape);
		body->add_child(cs);
		MeshInstance3D *mi = memnew(MeshInstance3D);
		mi->set_mesh(mesh);
		mi->set_material_override(mat(material));
		body->add_child(mi);
		return mi;
	}
	return visual_mesh(mesh, Transform3D(Basis(), c), material);
}

MeshInstance3D *LevelBuilder::sphere(const Vector3 &center, float radius, const char *material) {
	Ref<SphereMesh> mesh;
	mesh.instantiate();
	mesh->set_radius(radius);
	mesh->set_height(radius * 2.0f);
	mesh->set_radial_segments(12);
	mesh->set_rings(6);
	return visual_mesh(mesh, Transform3D(Basis(), center), material);
}

void LevelBuilder::wall(const Vector3 &a, const Vector3 &b, float y0, float height, float thickness, const char *material, std::vector<Opening> openings, bool collide) {
	Vector3 d(b.x - a.x, 0.0f, b.z - a.z);
	float len = d.length();
	if (len < 0.01f) {
		return;
	}
	Vector3 dir = d / len;
	float angle = std::atan2(-dir.z, dir.x);
	Basis basis(Vector3(0, 1, 0), angle);
	std::sort(openings.begin(), openings.end(), [](const Opening &l, const Opening &r) { return l.offset < r.offset; });

	auto segment = [&](float s0, float s1, float yb, float yt) {
		if (s1 - s0 < 0.02f || yt - yb < 0.02f) {
			return;
		}
		float mid = (s0 + s1) * 0.5f;
		Vector3 c = Vector3(a.x, 0.0f, a.z) + dir * mid;
		c.y = (yb + yt) * 0.5f;
		box_rot(c, basis, Vector3(s1 - s0, yt - yb, thickness), material, collide);
	};

	float cursor = 0.0f;
	for (const Opening &o : openings) {
		float s0 = clampf(o.offset - o.width * 0.5f, 0.0f, len);
		float s1 = clampf(o.offset + o.width * 0.5f, 0.0f, len);
		if (s0 < cursor) {
			s0 = cursor;
		}
		segment(cursor, s0, y0, y0 + height);
		segment(s0, s1, y0, y0 + std::min(o.bottom, height));
		segment(s0, s1, y0 + std::min(o.top, height), y0 + height);
		cursor = std::max(cursor, s1);
	}
	segment(cursor, len, y0, y0 + height);
}

void LevelBuilder::slab(float x0, float z0, float x1, float z1, float top_y, float thickness, const char *material, const std::vector<Rect2> &holes) {
	std::vector<float> xs = { x0, x1 };
	std::vector<float> zs = { z0, z1 };
	for (const Rect2 &h : holes) {
		float hx0 = clampf(h.position.x, x0, x1);
		float hx1 = clampf(h.position.x + h.size.x, x0, x1);
		float hz0 = clampf(h.position.y, z0, z1);
		float hz1 = clampf(h.position.y + h.size.y, z0, z1);
		xs.push_back(hx0);
		xs.push_back(hx1);
		zs.push_back(hz0);
		zs.push_back(hz1);
	}
	std::sort(xs.begin(), xs.end());
	std::sort(zs.begin(), zs.end());
	xs.erase(std::unique(xs.begin(), xs.end(), [](float l, float r) { return std::fabs(l - r) < 0.001f; }), xs.end());
	zs.erase(std::unique(zs.begin(), zs.end(), [](float l, float r) { return std::fabs(l - r) < 0.001f; }), zs.end());

	auto solid = [&](float cx, float cz) {
		for (const Rect2 &h : holes) {
			if (cx > h.position.x && cx < h.position.x + h.size.x && cz > h.position.y && cz < h.position.y + h.size.y) {
				return false;
			}
		}
		return true;
	};

	for (size_t zi = 0; zi + 1 < zs.size(); zi++) {
		float za = zs[zi];
		float zb = zs[zi + 1];
		float cz = (za + zb) * 0.5f;
		size_t xi = 0;
		while (xi + 1 < xs.size()) {
			if (!solid((xs[xi] + xs[xi + 1]) * 0.5f, cz)) {
				xi++;
				continue;
			}
			size_t end = xi + 1;
			while (end + 1 < xs.size() && solid((xs[end] + xs[end + 1]) * 0.5f, cz)) {
				end++;
			}
			float xa = xs[xi];
			float xb = xs[end];
			box(Vector3((xa + xb) * 0.5f, top_y - thickness * 0.5f, cz), Vector3(xb - xa, thickness, zb - za), material);
			xi = end;
		}
	}
}

void LevelBuilder::hex_slab(const Vector3 &center, float radius, float top_y, float thickness, const char *material) {
	StaticBody3D *body = memnew(StaticBody3D);
	body->set_collision_layer(layer::WORLD);
	body->set_collision_mask(0);
	body->set_position(Vector3(center.x, top_y - thickness * 0.5f, center.z));
	current()->add_child(body);
	PackedVector3Array points;
	for (int i = 0; i < 6; i++) {
		float a = float(i) * PI / 3.0f;
		points.push_back(Vector3(std::sin(a) * radius, thickness * 0.5f, std::cos(a) * radius));
		points.push_back(Vector3(std::sin(a) * radius, -thickness * 0.5f, std::cos(a) * radius));
	}
	Ref<ConvexPolygonShape3D> shape;
	shape.instantiate();
	shape->set_points(points);
	CollisionShape3D *cs = memnew(CollisionShape3D);
	cs->set_shape(shape);
	body->add_child(cs);
	Ref<CylinderMesh> mesh;
	mesh.instantiate();
	mesh->set_top_radius(radius);
	mesh->set_bottom_radius(radius);
	mesh->set_height(thickness);
	mesh->set_radial_segments(6);
	mesh->set_rings(1);
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(mat(material));
	body->add_child(mi);
}

void LevelBuilder::ramp(const Vector3 &bottom, const Vector3 &top, float width, const char *material, bool rails) {
	Vector3 d = top - bottom;
	Vector3 horizontal(d.x, 0.0f, d.z);
	float run = horizontal.length();
	float rise = d.y;
	if (run < 0.1f) {
		return;
	}
	Vector3 fwd = horizontal / run;
	Vector3 right = fwd.cross(Vector3(0, 1, 0)).normalized();

	int steps = std::max(1, int(std::round(rise / 0.17f)));
	float step_rise = rise / float(steps);
	float step_run = run / float(steps);

	Vector3 z = d.normalized();
	Vector3 x = Vector3(0, 1, 0).cross(z).normalized();
	Vector3 y = z.cross(x).normalized();
	Basis ramp_basis(x, y, z);
	float thick = 0.25f;
	float lift = step_rise * 0.35f;
	float extend = 0.4f;
	Vector3 start = bottom - z * extend;
	float len = d.length() + extend;
	Vector3 center = (start + top) * 0.5f - y * (thick * 0.5f) + Vector3(0, lift, 0);
	collider_box(center, ramp_basis, Vector3(width, thick, len));

	Basis yaw_basis = basis_looking(fwd);
	for (int i = 0; i < steps; i++) {
		float s_mid = (float(i) + 0.5f) * step_run;
		float top_y = bottom.y + float(i + 1) * step_rise;
		float h = step_rise + 0.12f;
		Vector3 c = bottom + fwd * s_mid;
		c.y = top_y - h * 0.5f;
		visual_box(c, Vector3(width, h, step_run + 0.01f), material, yaw_basis);
	}
	Vector3 under_center = (bottom + top) * 0.5f - y * 0.22f;
	visual_box(under_center, Vector3(width, 0.18f, d.length()), material, ramp_basis);

	if (rails) {
		Vector3 side = right * (width * 0.5f - 0.04f);
		for (float sgn : { -1.0f, 1.0f }) {
			Vector3 a = bottom + side * sgn + Vector3(0, 0.95f, 0);
			Vector3 b = top + side * sgn + Vector3(0, 0.95f, 0);
			pipe(a, b, 0.025f, "steel");
		}
	}
}

void LevelBuilder::pipe(const Vector3 &a, const Vector3 &b, float radius, const char *material) {
	Vector3 d = b - a;
	float len = d.length();
	if (len < 0.01f) {
		return;
	}
	Ref<CylinderMesh> mesh;
	mesh.instantiate();
	mesh->set_top_radius(radius);
	mesh->set_bottom_radius(radius);
	mesh->set_height(len);
	mesh->set_radial_segments(8);
	mesh->set_rings(1);
	Vector3 yv = d / len;
	Vector3 ref = std::fabs(yv.y) > 0.9f ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
	Vector3 xv = ref.cross(yv).normalized();
	Vector3 zv = xv.cross(yv).normalized();
	visual_mesh(mesh, Transform3D(Basis(xv, yv, zv), (a + b) * 0.5f), material);
}

void LevelBuilder::railing(const Vector3 &a, const Vector3 &b, float y, float height, const char *material) {
	Vector3 d(b.x - a.x, 0.0f, b.z - a.z);
	float len = d.length();
	if (len < 0.05f) {
		return;
	}
	Vector3 dir = d / len;
	int posts = std::max(2, int(len / 1.5f) + 1);
	for (int i = 0; i < posts; i++) {
		Vector3 p = Vector3(a.x, y, a.z) + dir * (len * float(i) / float(posts - 1));
		pipe(p, p + Vector3(0, height, 0), 0.025f, material);
	}
	pipe(Vector3(a.x, y + height, a.z), Vector3(b.x, y + height, b.z), 0.03f, material);
	pipe(Vector3(a.x, y + height * 0.5f, a.z), Vector3(b.x, y + height * 0.5f, b.z), 0.02f, material);
	float angle = std::atan2(-dir.z, dir.x);
	Vector3 c = Vector3((a.x + b.x) * 0.5f, y + height * 0.5f, (a.z + b.z) * 0.5f);
	collider_box(c, Basis(Vector3(0, 1, 0), angle), Vector3(len, height, 0.08f));
}

void LevelBuilder::rubble(const Vector3 &center, float radius, int count, const char *material) {
	for (int i = 0; i < count; i++) {
		float ang = rng.range(0.0f, 2.0f * PI);
		float r = radius * std::sqrt(rng.randf());
		Vector3 p = center + Vector3(std::cos(ang) * r, 0.0f, std::sin(ang) * r);
		float s = rng.range(0.08f, 0.35f);
		Vector3 size(s * rng.range(0.7f, 1.6f), s * rng.range(0.3f, 0.8f), s * rng.range(0.7f, 1.6f));
		Basis b = Basis(Vector3(0, 1, 0), rng.range(0.0f, 2.0f * PI)) * Basis(Vector3(1, 0, 0), rng.range(-0.3f, 0.3f));
		p.y += size.y * 0.35f;
		visual_box(p, size, material, b, false);
	}
}

void LevelBuilder::tree(const Vector3 &base, float height) {
	cylinder(base, 0.18f, height * 0.55f, "tree_bark", true, 6);
	Ref<SphereMesh> crown;
	crown.instantiate();
	float r = height * 0.28f;
	crown->set_radius(r);
	crown->set_height(r * 2.2f);
	crown->set_radial_segments(8);
	crown->set_rings(5);
	visual_mesh(crown, Transform3D(Basis(), base + Vector3(0, height * 0.68f, 0)), "tree_crown");
	visual_mesh(crown, Transform3D(Basis().scaled(Vector3(0.7f, 0.7f, 0.7f)), base + Vector3(r * 0.5f, height * 0.88f, -r * 0.3f)), "tree_crown");
}

void LevelBuilder::block_building(const Vector3 &center, const Vector3 &size, bool roof_details) {
	box(center + Vector3(0, size.y * 0.5f, 0), size, "facade");
	box(center + Vector3(0, size.y + 0.15f, 0), Vector3(size.x + 0.2f, 0.3f, size.z + 0.2f), "roof", false);
	if (roof_details) {
		visual_box(center + Vector3(size.x * 0.2f, size.y + 1.2f, 0), Vector3(3.0f, 2.2f, 3.0f), "concrete");
		pipe(center + Vector3(-size.x * 0.3f, size.y, 0), center + Vector3(-size.x * 0.3f, size.y + 4.0f, 0), 0.04f, "steel");
	}
}

void LevelBuilder::fence(const Vector3 &a, const Vector3 &b, float height, const char *material, std::vector<Opening> gaps, bool barbed) {
	wall(a, b, a.y, height, 0.06f, material, gaps);
	Vector3 d(b.x - a.x, 0.0f, b.z - a.z);
	float len = d.length();
	if (len < 0.1f) {
		return;
	}
	Vector3 dir = d / len;
	int posts = int(len / 3.0f) + 1;
	for (int i = 0; i <= posts; i++) {
		float s = len * float(i) / float(posts);
		bool in_gap = false;
		for (const Opening &o : gaps) {
			if (s > o.offset - o.width * 0.5f - 0.1f && s < o.offset + o.width * 0.5f + 0.1f) {
				in_gap = true;
			}
		}
		if (in_gap) {
			continue;
		}
		Vector3 p = Vector3(a.x, a.y, a.z) + dir * s;
		visual_box(p + Vector3(0, height * 0.5f + 0.1f, 0), Vector3(0.08f, height + 0.2f, 0.08f), "metal_gray");
	}
	if (barbed) {
		for (float off : { 0.05f, 0.18f }) {
			pipe(Vector3(a.x, a.y + height + off, a.z), Vector3(b.x, b.y + height + off, b.z), 0.012f, "steel");
		}
	}
}

void LevelBuilder::bunk_bed(const Vector3 &center, float yaw, float length) {
	Basis bs(Vector3(0, 1, 0), yaw);
	Vector3 fwd = bs.xform(Vector3(1, 0, 0));
	Vector3 side = bs.xform(Vector3(0, 0, 1));
	for (float level_y : { 0.45f, 1.35f }) {
		visual_box(center + Vector3(0, level_y, 0), Vector3(length, 0.05f, 0.75f), "wood", bs);
	}
	for (float sx : { -0.5f, 0.5f }) {
		for (float sz : { -0.34f, 0.34f }) {
			Vector3 p = center + fwd * (sx * (length - 0.1f)) + side * sz;
			visual_box(p + Vector3(0, 0.9f, 0), Vector3(0.07f, 1.8f, 0.07f), "wood", bs);
		}
	}
	collider_box(center + Vector3(0, 0.4f, 0), bs, Vector3(length, 0.8f, 0.75f));
}

void LevelBuilder::bed_frame(const Vector3 &center, float yaw) {
	Basis bs(Vector3(0, 1, 0), yaw);
	Vector3 fwd = bs.xform(Vector3(1, 0, 0));
	visual_box(center + Vector3(0, 0.45f, 0), Vector3(1.9f, 0.04f, 0.85f), "rust", bs);
	for (float sx : { -0.92f, 0.92f }) {
		Vector3 p = center + fwd * sx;
		visual_box(p + Vector3(0, 0.45f, 0), Vector3(0.04f, 0.9f, 0.85f), "rust", bs);
	}
	collider_box(center + Vector3(0, 0.3f, 0), bs, Vector3(1.9f, 0.6f, 0.85f));
}

void LevelBuilder::lamp_fixture(const Vector3 &pos, bool lit) {
	visual_box(pos, Vector3(0.25f, 0.12f, 0.25f), lit ? "lamp_warm" : "plastic_white", Basis(), false);
}

void LevelBuilder::poster(const Vector3 &center, const Vector3 &normal, float w, float h, const String &title, const String &body, const Color &accent) {
	Vector3 n = normal.normalized();
	Basis bs = basis_looking(-n);
	visual_box(center, Vector3(w, h, 0.01f), "poster_paper", bs, false);
	visual_box(center + Vector3(0, h * 0.5f - 0.06f, 0) + n * 0.006f, Vector3(w, 0.1f, 0.005f), "red_paint", bs, false);
	Label3D *t = text(center + Vector3(0, h * 0.3f, 0) + n * 0.012f, n, title, h * 0.09f, Color(0.55f, 0.06f, 0.04f), true);
	t->set_width(w * 0.95f / t->get_pixel_size());
	t->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	Label3D *b = text(center - Vector3(0, h * 0.12f, 0) + n * 0.012f, n, body, h * 0.045f, Color(0.12f, 0.1f, 0.08f), true);
	b->set_width(w * 0.9f / b->get_pixel_size());
	b->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	(void)accent;
}

Label3D *LevelBuilder::text(const Vector3 &pos, const Vector3 &normal, const String &txt, float height, const Color &color, bool shaded, bool dynamic) {
	Label3D *label = memnew(Label3D);
	label->set_text(txt);
	int font_size = 64;
	label->set_font_size(font_size);
	label->set_pixel_size(height / float(font_size));
	label->set_modulate(color);
	label->set_outline_size(0);
	label->set_draw_flag(Label3D::FLAG_SHADED, shaded);
	label->set_draw_flag(Label3D::FLAG_DOUBLE_SIDED, false);
	label->set_alpha_cut_mode(Label3D::ALPHA_CUT_DISCARD);
	Vector3 n = normal.normalized();
	label->set_transform(Transform3D(Basis(Vector3(0, 1, 0), std::atan2(n.x, n.z)), pos));
	if (dynamic) {
		dynamic_root->add_child(label);
		label->set_global_transform(gxf(label->get_transform()));
	} else {
		current()->add_child(label);
	}
	return label;
}

Label3D *LevelBuilder::graffiti(const Vector3 &pos, const Vector3 &normal, const String &txt, float height) {
	static const Color palette[] = {
		Color(0.85f, 0.12f, 0.1f),
		Color(0.15f, 0.55f, 0.85f),
		Color(0.95f, 0.85f, 0.2f),
		Color(0.2f, 0.75f, 0.35f),
		Color(0.9f, 0.9f, 0.9f),
		Color(0.85f, 0.35f, 0.75f),
		Color(0.08f, 0.08f, 0.08f),
	};
	Color c = palette[rng.rangei(0, 6)];
	Label3D *l = text(pos, normal, txt, height, c, true);
	l->set_outline_size(10);
	l->set_outline_modulate(Color(c.r * 0.3f, c.g * 0.3f, c.b * 0.3f, 0.9f));
	Basis b = l->get_transform().basis * Basis(Vector3(0, 0, 1), rng.range(-0.12f, 0.12f));
	l->set_transform(Transform3D(b, l->get_transform().origin));
	return l;
}

OmniLight3D *LevelBuilder::omni(const Vector3 &pos, const Color &color, float range, float energy, bool shadow) {
	OmniLight3D *light = memnew(OmniLight3D);
	light->set_color(color);
	light->set_param(Light3D::PARAM_RANGE, range);
	light->set_param(Light3D::PARAM_ENERGY, energy);
	light->set_param(Light3D::PARAM_ATTENUATION, 1.2f);
	light->set_shadow(shadow);
	light->set_position(pos);
	current()->add_child(light);
	return light;
}

SpotLight3D *LevelBuilder::spot(const Vector3 &pos, const Vector3 &target, const Color &color, float range, float energy, float angle_deg, bool shadow) {
	SpotLight3D *light = memnew(SpotLight3D);
	light->set_color(color);
	light->set_param(Light3D::PARAM_RANGE, range);
	light->set_param(Light3D::PARAM_ENERGY, energy);
	light->set_param(Light3D::PARAM_SPOT_ANGLE, angle_deg);
	light->set_param(Light3D::PARAM_SPOT_ATTENUATION, 0.8f);
	light->set_shadow(shadow);
	light->set_transform(Transform3D(basis_looking(target - pos), pos));
	current()->add_child(light);
	return light;
}

} // namespace urbex
