#include "levels/kit.h"

#include "core/game.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>

using namespace godot;

namespace urbex {

namespace {

MeshInstance3D *mesh_box(Node3D *parent, const Vector3 &size, const Vector3 &pos, const Ref<Material> &m, const Vector3 &rot = Vector3()) {
	Ref<BoxMesh> mesh;
	mesh.instantiate();
	mesh->set_size(size);
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(m);
	mi->set_position(pos);
	mi->set_rotation(rot);
	parent->add_child(mi);
	return mi;
}

MeshInstance3D *mesh_cyl(Node3D *parent, float radius, float height, const Vector3 &pos, const Ref<Material> &m, const Vector3 &rot = Vector3()) {
	Ref<CylinderMesh> mesh;
	mesh.instantiate();
	mesh->set_top_radius(radius);
	mesh->set_bottom_radius(radius);
	mesh->set_height(height);
	mesh->set_radial_segments(10);
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(m);
	mi->set_position(pos);
	mi->set_rotation(rot);
	parent->add_child(mi);
	return mi;
}

MeshInstance3D *mesh_sphere(Node3D *parent, float radius, const Vector3 &pos, const Ref<Material> &m) {
	Ref<SphereMesh> mesh;
	mesh.instantiate();
	mesh->set_radius(radius);
	mesh->set_height(radius * 2.0f);
	mesh->set_radial_segments(10);
	mesh->set_rings(5);
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(m);
	mi->set_position(pos);
	parent->add_child(mi);
	return mi;
}

} // namespace

Door *Kit::door(const Vector3 &local_pos, float local_yaw, Door::Style style, float width, float height, const String &name) {
	const MaterialLibrary &m = g.get_materials();
	const char *panel = "door_wood";
	const char *detail = "steel";
	switch (style) {
		case Door::STYLE_WOOD:
			panel = "door_wood";
			break;
		case Door::STYLE_METAL:
			panel = "door_metal";
			break;
		case Door::STYLE_BLAST:
			panel = "door_gray";
			detail = "metal";
			break;
		case Door::STYLE_HATCH:
			panel = "door_metal";
			break;
		case Door::STYLE_GATE:
			panel = "fence";
			break;
	}
	Door *d = memnew(Door);
	d->setup(style, width, height, name, m.get(panel), m.get(detail));
	b.add_dynamic(d, local_pos, local_yaw);
	return d;
}

Pickup *Kit::pickup(const Vector3 &local_pos, const String &id, const String &name, const String &description, bool artifact, const char *model) {
	const MaterialLibrary &m = g.get_materials();
	Pickup *p = memnew(Pickup);
	p->setup(id, name, description, artifact);
	std::string kind = model;
	if (kind == "batteries") {
		mesh_box(p, Vector3(0.09f, 0.05f, 0.035f), Vector3(0, 0.025f, 0), m.get("yellow_paint"));
		mesh_box(p, Vector3(0.09f, 0.012f, 0.036f), Vector3(0, 0.05f, 0), m.get("black"));
	} else if (kind == "paper") {
		mesh_box(p, Vector3(0.22f, 0.012f, 0.3f), Vector3(0, 0.006f, 0), m.get("paper"), Vector3(0, 0.3f, 0));
		mesh_box(p, Vector3(0.18f, 0.002f, 0.04f), Vector3(0, 0.013f, -0.08f), m.get("red_paint"), Vector3(0, 0.3f, 0));
	} else if (kind == "gasmask") {
		mesh_sphere(p, 0.11f, Vector3(0, 0.1f, 0), m.get("rubber"));
		mesh_box(p, Vector3(0.08f, 0.04f, 0.05f), Vector3(0, 0.06f, -0.1f), m.get("plastic_dark"));
		mesh_cyl(p, 0.055f, 0.1f, Vector3(0.0f, 0.05f, -0.2f), m.get("metal"), Vector3(1.5708f, 0, 0));
		mesh_box(p, Vector3(0.28f, 0.2f, 0.12f), Vector3(0.25f, 0.1f, 0.05f), m.get("canvas"));
	} else if (kind == "key") {
		mesh_box(p, Vector3(0.07f, 0.008f, 0.02f), Vector3(0, 0.004f, 0), m.get("steel"));
		mesh_cyl(p, 0.02f, 0.006f, Vector3(-0.045f, 0.004f, 0), m.get("steel"));
	} else if (kind == "helmet") {
		mesh_sphere(p, 0.15f, Vector3(0, 0.05f, 0), m.get("yellow_paint"));
	} else {
		mesh_box(p, Vector3(0.25f, 0.18f, 0.2f), Vector3(0, 0.09f, 0), m.get("canvas"));
	}
	p->add_hitbox(Vector3(0.6f, 0.4f, 0.6f), Vector3(0, 0.15f, 0));
	b.add_dynamic(p, local_pos);
	return p;
}

NoteBoard *Kit::board(const Vector3 &local_pos, float local_yaw, const String &title, const String &body, bool on_post) {
	const MaterialLibrary &m = g.get_materials();
	NoteBoard *n = memnew(NoteBoard);
	n->setup(title, body);
	if (on_post) {
		mesh_cyl(n, 0.04f, 1.6f, Vector3(-0.4f, 0.8f, 0), m.get("metal_gray"));
		mesh_cyl(n, 0.04f, 1.6f, Vector3(0.4f, 0.8f, 0), m.get("metal_gray"));
		mesh_box(n, Vector3(1.0f, 0.7f, 0.04f), Vector3(0, 1.45f, 0), m.get("metal"));
		mesh_box(n, Vector3(0.9f, 0.6f, 0.01f), Vector3(0, 1.45f, -0.025f), m.get("poster_paper"));
		n->add_hitbox(Vector3(1.1f, 0.9f, 0.4f), Vector3(0, 1.4f, 0));
	} else {
		mesh_box(n, Vector3(0.7f, 0.5f, 0.02f), Vector3(0, 0, 0), m.get("poster_paper"));
		mesh_box(n, Vector3(0.7f, 0.06f, 0.022f), Vector3(0, 0.22f, 0), m.get("red_paint"));
		n->add_hitbox(Vector3(0.8f, 0.6f, 0.3f), Vector3(0, 0, -0.1f));
	}
	Label3D *l = memnew(Label3D);
	l->set_text(title);
	l->set_font_size(48);
	l->set_pixel_size(0.0016f);
	l->set_modulate(Color(0.15f, 0.12f, 0.1f));
	l->set_outline_size(0);
	l->set_width(500.0f);
	l->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	l->set_draw_flag(Label3D::FLAG_SHADED, true);
	l->set_position(on_post ? Vector3(0, 1.5f, -0.032f) : Vector3(0, 0.05f, -0.012f));
	l->set_rotation(Vector3(0, PI, 0));
	n->add_child(l);
	b.add_dynamic(n, local_pos, local_yaw);
	return n;
}

PhotoSpot *Kit::spot(const Vector3 &local_pos, const String &id, const String &title, float distance) {
	PhotoSpot *s = memnew(PhotoSpot);
	s->setup(id, title, distance);
	b.add_dynamic(s, local_pos);
	return s;
}

Guard *Kit::guard(Guard::Kind kind, const String &name, const Vector3 &global_pos, bool flashlight) {
	Guard *gu = memnew(Guard);
	gu->setup(kind, name, g.get_materials(), flashlight);
	b.dynamic_root->add_child(gu);
	gu->set_global_position(global_pos);
	return gu;
}

MotionSensor *Kit::pir(const Vector3 &local_pos, const Vector3 &local_target, float range, const String &group, bool dead, const String &name) {
	MotionSensor *s = memnew(MotionSensor);
	s->setup(g.get_materials(), range, 90.0f);
	s->set_power_group(group);
	s->set_dead(dead);
	s->set_device_name(name);
	b.dynamic_root->add_child(s);
	Vector3 p = b.g(local_pos);
	s->set_global_transform(Transform3D(basis_looking(b.g(local_target) - p), p));
	return s;
}

LaserBarrier *Kit::beam(const Vector3 &local_a, const Vector3 &local_b, float local_floor, const String &group, const String &name) {
	LaserBarrier *l = memnew(LaserBarrier);
	b.dynamic_root->add_child(l);
	l->setup(g.get_materials(), b.g(local_a), b.g(local_b), b.g(Vector3(local_a.x, local_floor, local_a.z)).y);
	l->set_power_group(group);
	l->set_device_name(name);
	return l;
}

SecurityCamera *Kit::camera(const Vector3 &local_pos, float local_yaw, float sweep_deg, float pitch_deg, bool fake, const String &group, const String &name) {
	SecurityCamera *c = memnew(SecurityCamera);
	c->setup(g.get_materials(), b.gyaw(local_yaw), sweep_deg, pitch_deg, fake);
	c->set_power_group(group);
	c->set_device_name(name);
	b.add_dynamic(c, local_pos, local_yaw);
	return c;
}

ClimbPoint *Kit::climb(const Vector3 &local_pos, const Vector3 &hitbox, const String &label, const Vector3 &global_target, float global_yaw, float duration) {
	ClimbPoint *c = memnew(ClimbPoint);
	c->setup(label, global_target, global_yaw, duration);
	c->add_hitbox(hitbox, Vector3());
	b.add_dynamic(c, local_pos);
	return c;
}

PowerSwitch *Kit::power_box(const Vector3 &local_pos, float local_yaw, const String &group, const String &name) {
	const MaterialLibrary &m = g.get_materials();
	PowerSwitch *s = memnew(PowerSwitch);
	mesh_box(s, Vector3(0.5f, 0.7f, 0.18f), Vector3(0, 0, 0), m.get("metal_gray"));
	mesh_box(s, Vector3(0.12f, 0.12f, 0.01f), Vector3(0.12f, 0.22f, -0.095f), m.get("yellow_paint"));
	Node3D *lever = memnew(Node3D);
	lever->set_position(Vector3(-0.1f, -0.05f, -0.1f));
	lever->set_rotation(Vector3(-0.6f, 0, 0));
	s->add_child(lever);
	mesh_box(lever, Vector3(0.04f, 0.2f, 0.04f), Vector3(0, 0.1f, 0), m.get("black"));
	mesh_box(lever, Vector3(0.1f, 0.04f, 0.04f), Vector3(0, 0.2f, 0), m.get("red_paint"));
	s->setup(group, name, lever);
	s->add_hitbox(Vector3(0.6f, 0.8f, 0.4f), Vector3(0, 0, -0.1f));
	b.add_dynamic(s, local_pos, local_yaw);
	return s;
}

ActionPoint *Kit::action(const Vector3 &local_pos, const Vector3 &hitbox, const String &prompt, std::function<void(UrbexPlayer *)> fn, bool one_shot) {
	ActionPoint *a = memnew(ActionPoint);
	a->setup(prompt, std::move(fn), one_shot);
	a->add_hitbox(hitbox, Vector3());
	b.add_dynamic(a, local_pos);
	return a;
}

void Kit::street_lamp(const Vector3 &local_pos, float local_yaw, const String &group) {
	b.cylinder(local_pos, 0.08f, 6.5f, "metal_gray", true, 8);
	Vector3 arm_dir = yaw_forward(local_yaw);
	Vector3 head = local_pos + Vector3(0, 6.4f, 0) + arm_dir * 1.2f;
	b.pipe(local_pos + Vector3(0, 6.3f, 0), head, 0.05f, "metal_gray");
	b.visual_box(head, Vector3(0.5f, 0.12f, 0.3f), group.is_empty() || g.is_powered(group) ? "lamp_warm" : "plastic_dark", basis_looking(arm_dir), false);
	OmniLight3D *l = b.omni(head - Vector3(0, 0.3f, 0), Color(1.0f, 0.72f, 0.38f), 15.0f, 1.8f, false);
	if (!group.is_empty()) {
		g.register_light(l, group);
	}
	probe(head - Vector3(0, 5.0f, 0), 9.0f, 0.75f, group);
}

void Kit::ceiling_lamp(const Vector3 &local_pos, const Color &color, float range, float energy, const String &group, bool shadow) {
	b.visual_box(local_pos + Vector3(0, 0.06f, 0), Vector3(0.3f, 0.1f, 0.3f), "lamp_warm", Basis(), false);
	OmniLight3D *l = b.omni(local_pos - Vector3(0, 0.2f, 0), color, range, energy, shadow);
	if (!group.is_empty()) {
		g.register_light(l, group);
	}
	probe(local_pos - Vector3(0, 1.4f, 0), range * 0.6f, 0.7f, group);
}

void Kit::probe(const Vector3 &local_pos, float radius, float strength, const String &group) {
	LightProbe lp;
	lp.position = b.g(local_pos);
	lp.radius = radius;
	lp.strength = strength;
	lp.power_group = group;
	d.lights.push_back(lp);
}

void Kit::shadow_zone(const Vector3 &local_center, const Vector3 &size) {
	Zone z;
	z.xf = b.gxf(Transform3D(Basis(), local_center));
	z.half = size * 0.5f;
	d.shadow_zones.push_back(z);
}

AABB Kit::aabb(const Vector3 &local_center, const Vector3 &size) const {
	Vector3 h = size * 0.5f;
	AABB out;
	bool first = true;
	for (int i = 0; i < 8; i++) {
		Vector3 c = local_center + Vector3((i & 1) ? h.x : -h.x, (i & 2) ? h.y : -h.y, (i & 4) ? h.z : -h.z);
		Vector3 gp = b.g(c);
		if (first) {
			out = AABB(gp, Vector3());
			first = false;
		} else {
			out.expand_to(gp);
		}
	}
	return out;
}

void Kit::noisy(const Vector3 &local_center, const Vector3 &size, bool visual) {
	d.noisy_floors.push_back(aabb(local_center, size));
	if (!visual) {
		return;
	}
	int count = int(size.x * size.z * 6.0f);
	for (int i = 0; i < count; i++) {
		Vector3 p = local_center + Vector3(b.rng.range(-size.x * 0.5f, size.x * 0.5f), -size.y * 0.5f + 0.01f, b.rng.range(-size.z * 0.5f, size.z * 0.5f));
		float s = b.rng.range(0.04f, 0.14f);
		b.visual_box(p, Vector3(s, 0.006f, s * b.rng.range(0.4f, 1.0f)), "glass", Basis(Vector3(0, 1, 0), b.rng.range(0.0f, 6.28f)), false);
	}
}

void Kit::hint(const Vector3 &local_center, const Vector3 &size, const String &text) {
	HintZone h;
	h.zone = aabb(local_center, size);
	h.text = text;
	d.hints.push_back(h);
}

void Kit::objective_photo(const String &id, const String &title, bool optional) {
	Objective o;
	o.type = ObjectiveType::Photo;
	o.id = id;
	o.title = title;
	o.optional = optional;
	d.objectives.push_back(o);
}

void Kit::objective_item(const String &id, const String &title, bool optional) {
	Objective o;
	o.type = ObjectiveType::Item;
	o.id = id;
	o.title = title;
	o.optional = optional;
	d.objectives.push_back(o);
}

void Kit::objective_action(const String &id, const String &title, bool optional) {
	Objective o;
	o.type = ObjectiveType::Action;
	o.id = id;
	o.title = title;
	o.optional = optional;
	d.objectives.push_back(o);
}

void Kit::objective_reach(const String &id, const String &title, const AABB &zone, bool optional) {
	Objective o;
	o.type = ObjectiveType::Reach;
	o.id = id;
	o.title = title;
	o.zone = zone;
	o.optional = optional;
	d.objectives.push_back(o);
}

} // namespace urbex
