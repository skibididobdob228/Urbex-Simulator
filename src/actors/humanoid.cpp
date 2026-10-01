#include "actors/humanoid.h"

#include "core/common.h"
#include "core/materials.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/capsule_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/torus_mesh.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace urbex {

namespace {

struct Parts {
	const MaterialLibrary &m;

	MeshInstance3D *add(Node3D *parent, const Ref<Mesh> &mesh, const Vector3 &pos, const char *mat, const Vector3 &rot = Vector3(), const Vector3 &scl = Vector3(1, 1, 1)) {
		MeshInstance3D *mi = memnew(MeshInstance3D);
		mi->set_mesh(mesh);
		mi->set_material_override(m.get(mat));
		Transform3D t(Basis::from_euler(rot).scaled_local(scl), pos);
		mi->set_transform(t);
		parent->add_child(mi);
		return mi;
	}

	MeshInstance3D *box(Node3D *parent, const Vector3 &size, const Vector3 &pos, const char *mat, const Vector3 &rot = Vector3()) {
		Ref<BoxMesh> mesh;
		mesh.instantiate();
		mesh->set_size(size);
		return add(parent, mesh, pos, mat, rot);
	}

	MeshInstance3D *capsule(Node3D *parent, float r, float h, const Vector3 &pos, const char *mat, const Vector3 &rot = Vector3(), const Vector3 &scl = Vector3(1, 1, 1)) {
		Ref<CapsuleMesh> mesh;
		mesh.instantiate();
		mesh->set_radius(r);
		mesh->set_height(std::max(h, r * 2.0f));
		mesh->set_radial_segments(12);
		mesh->set_rings(4);
		return add(parent, mesh, pos, mat, rot, scl);
	}

	MeshInstance3D *sphere(Node3D *parent, float r, const Vector3 &pos, const char *mat, const Vector3 &scl = Vector3(1, 1, 1)) {
		Ref<SphereMesh> mesh;
		mesh.instantiate();
		mesh->set_radius(r);
		mesh->set_height(r * 2.0f);
		mesh->set_radial_segments(14);
		mesh->set_rings(7);
		return add(parent, mesh, pos, mat, Vector3(), scl);
	}

	MeshInstance3D *cyl(Node3D *parent, float r_bottom, float r_top, float h, const Vector3 &pos, const char *mat, const Vector3 &rot = Vector3()) {
		Ref<CylinderMesh> mesh;
		mesh.instantiate();
		mesh->set_bottom_radius(r_bottom);
		mesh->set_top_radius(r_top);
		mesh->set_height(h);
		mesh->set_radial_segments(12);
		mesh->set_rings(1);
		return add(parent, mesh, pos, mat, rot);
	}

	MeshInstance3D *torus(Node3D *parent, float inner, float outer, const Vector3 &pos, const char *mat, const Vector3 &rot = Vector3()) {
		Ref<TorusMesh> mesh;
		mesh.instantiate();
		mesh->set_inner_radius(inner);
		mesh->set_outer_radius(outer);
		mesh->set_rings(14);
		mesh->set_ring_segments(6);
		return add(parent, mesh, pos, mat, rot);
	}
};

Node3D *pivot(Node3D *parent, const Vector3 &pos) {
	Node3D *n = memnew(Node3D);
	n->set_position(pos);
	parent->add_child(n);
	return n;
}

}

void HumanoidRig::build(Node3D *parent, const MaterialLibrary &materials, const HumanoidLook &look) {
	Parts p{ materials };
	root = memnew(Node3D);
	root->set_scale(Vector3(look.scale, look.scale, look.scale));
	parent->add_child(root);

	const char *jacket = look.torso;
	const char *legs = look.legs;
	const char *skin = look.skin;
	bool gbr = look.outfit == HumanoidLook::OUTFIT_GBR;
	bool concierge = look.outfit == HumanoidLook::OUTFIT_CONCIERGE;
	bool guard = look.outfit == HumanoidLook::OUTFIT_GUARD;

	hip_l = pivot(root, Vector3(-0.1f, 0.92f, 0.0f));
	hip_r = pivot(root, Vector3(0.1f, 0.92f, 0.0f));
	for (Node3D *hip : { hip_l, hip_r }) {
		p.capsule(hip, 0.078f, 0.5f, Vector3(0.0f, -0.23f, 0.0f), legs);
		Node3D *knee = pivot(hip, Vector3(0.0f, -0.46f, 0.0f));
		p.capsule(knee, 0.062f, 0.46f, Vector3(0.0f, -0.22f, 0.0f), legs);
		p.box(knee, Vector3(0.11f, 0.08f, 0.27f), Vector3(0.0f, -0.43f, -0.05f), "black");
		p.box(knee, Vector3(0.112f, 0.025f, 0.272f), Vector3(0.0f, -0.465f, -0.05f), "rubber");
		if (hip == hip_l) {
			knee_l = knee;
		} else {
			knee_r = knee;
		}
	}
	p.box(root, Vector3(0.34f, 0.2f, 0.22f), Vector3(0.0f, 0.97f, 0.0f), legs);
	if (concierge) {
		p.cyl(root, 0.27f, 0.2f, 0.5f, Vector3(0.0f, 0.72f, 0.0f), "skirt");
	}

	torso = pivot(root, Vector3(0.0f, 1.0f, 0.0f));
	p.capsule(torso, 0.19f, 0.66f, Vector3(0.0f, 0.27f, 0.0f), jacket, Vector3(), Vector3(1.18f, 1.0f, 0.7f));
	p.box(torso, Vector3(0.3f, 0.07f, 0.17f), Vector3(0.0f, 0.55f, 0.0f), jacket);
	if (guard || look.outfit == HumanoidLook::OUTFIT_CIVIL) {
		p.box(torso, Vector3(0.012f, 0.52f, 0.006f), Vector3(0.0f, 0.27f, -0.135f), "steel");
		p.box(torso, Vector3(0.1f, 0.1f, 0.012f), Vector3(-0.1f, 0.38f, -0.13f), guard ? "uniform_detail" : jacket);
		p.box(torso, Vector3(0.1f, 0.1f, 0.012f), Vector3(0.1f, 0.38f, -0.13f), guard ? "uniform_detail" : jacket);
	}
	if (guard || gbr) {
		p.box(torso, Vector3(0.4f, 0.05f, 0.26f), Vector3(0.0f, -0.02f, 0.0f), "black");
		p.box(torso, Vector3(0.05f, 0.04f, 0.01f), Vector3(0.0f, -0.02f, -0.133f), "steel");
		p.box(torso, Vector3(0.06f, 0.11f, 0.04f), Vector3(-0.2f, -0.03f, -0.06f), "plastic_dark");
		p.cyl(torso, 0.006f, 0.006f, 0.12f, Vector3(-0.2f, 0.08f, -0.06f), "black");
		p.cyl(torso, 0.017f, 0.017f, 0.45f, Vector3(0.21f, -0.2f, 0.02f), "black", Vector3(0.12f, 0.0f, 0.0f));
	}
	if (gbr) {
		p.box(torso, Vector3(0.44f, 0.46f, 0.31f), Vector3(0.0f, 0.3f, 0.0f), "armor");
		for (float x : { -0.12f, 0.0f, 0.12f }) {
			p.box(torso, Vector3(0.1f, 0.12f, 0.05f), Vector3(x, 0.17f, -0.17f), "armor");
		}
		p.box(torso, Vector3(0.06f, 0.11f, 0.04f), Vector3(0.14f, 0.43f, -0.17f), "plastic_dark");
	}
	if (concierge) {
		p.torus(torso, 0.06f, 0.11f, Vector3(0.0f, 0.57f, 0.0f), "red_paint", Vector3(0.1f, 0.0f, 0.0f));
		for (int i = 0; i < 4; i++) {
			p.sphere(torso, 0.012f, Vector3(0.0f, 0.15f + float(i) * 0.09f, -0.135f), "plastic_white");
		}
	}

	shoulder_l = pivot(root, Vector3(-0.25f, 1.47f, 0.0f));
	shoulder_r = pivot(root, Vector3(0.25f, 1.47f, 0.0f));
	for (Node3D *sh : { shoulder_l, shoulder_r }) {
		p.capsule(sh, 0.058f, 0.34f, Vector3(0.0f, -0.15f, 0.0f), jacket);
		if (guard) {
			p.box(sh, Vector3(0.012f, 0.08f, 0.06f), Vector3(sh == shoulder_l ? -0.055f : 0.055f, -0.08f, 0.0f), "yellow_paint");
		}
		Node3D *elbow = pivot(sh, Vector3(0.0f, -0.3f, 0.0f));
		p.capsule(elbow, 0.05f, 0.3f, Vector3(0.0f, -0.13f, 0.0f), jacket);
		p.sphere(elbow, 0.045f, Vector3(0.0f, -0.3f, -0.01f), skin, Vector3(0.8f, 1.2f, 0.7f));
		if (sh == shoulder_l) {
			elbow_l = elbow;
		} else {
			elbow_r = elbow;
			hand_r = pivot(elbow, Vector3(0.0f, -0.3f, -0.02f));
		}
	}
	if (look.flashlight && hand_r) {
		p.cyl(hand_r, 0.022f, 0.026f, 0.2f, Vector3(0.0f, -0.05f, -0.03f), "black", Vector3(PI * 0.5f, 0.0f, 0.0f));
		p.cyl(hand_r, 0.024f, 0.024f, 0.01f, Vector3(0.0f, -0.05f, -0.135f), "lamp_cold", Vector3(PI * 0.5f, 0.0f, 0.0f));
	}
	if (look.bag && elbow_l) {
		p.box(elbow_l, Vector3(0.08f, 0.26f, 0.34f), Vector3(-0.02f, -0.45f, 0.0f), "black");
		p.torus(elbow_l, 0.05f, 0.06f, Vector3(0.0f, -0.3f, 0.0f), "black", Vector3(0.0f, 0.0f, PI * 0.5f));
	}

	p.cyl(root, 0.05f, 0.05f, 0.1f, Vector3(0.0f, 1.58f, 0.0f), skin);
	head = pivot(root, Vector3(0.0f, 1.6f, 0.0f));
	const char *face = gbr ? "black" : skin;
	p.sphere(head, 0.105f, Vector3(0.0f, 0.1f, 0.0f), face, Vector3(0.92f, 1.1f, 1.0f));
	p.box(head, Vector3(0.022f, 0.04f, 0.03f), Vector3(0.0f, 0.085f, -0.105f), face);
	for (float sx : { -0.1f, 0.1f }) {
		p.box(head, Vector3(0.015f, 0.05f, 0.03f), Vector3(sx, 0.1f, 0.0f), face);
	}
	if (gbr) {
		p.box(head, Vector3(0.13f, 0.035f, 0.02f), Vector3(0.0f, 0.125f, -0.097f), skin);
		p.sphere(head, 0.13f, Vector3(0.0f, 0.17f, 0.005f), "armor", Vector3(1.0f, 0.8f, 1.06f));
	}
	for (float sx : { -0.035f, 0.035f }) {
		p.sphere(head, 0.011f, Vector3(sx, 0.125f, -0.097f), "black");
		if (!gbr) {
			p.box(head, Vector3(0.035f, 0.007f, 0.01f), Vector3(sx, 0.148f, -0.1f), look.hair && concierge ? "hair_gray" : "hair_dark");
		}
	}
	if (!gbr) {
		p.box(head, Vector3(0.045f, 0.008f, 0.01f), Vector3(0.0f, 0.045f, -0.1f), "rubber");
	}
	if (look.cap && !gbr) {
		p.cyl(head, 0.112f, 0.122f, 0.085f, Vector3(0.0f, 0.205f, 0.0f), look.hat);
		p.box(head, Vector3(0.17f, 0.012f, 0.09f), Vector3(0.0f, 0.17f, -0.12f), look.hat, Vector3(0.15f, 0.0f, 0.0f));
		p.box(head, Vector3(0.03f, 0.03f, 0.006f), Vector3(0.0f, 0.215f, -0.118f), "yellow_paint");
	} else if (!gbr) {
		const char *hair = concierge ? "hair_gray" : "hair_dark";
		p.sphere(head, 0.108f, Vector3(0.0f, 0.145f, 0.012f), hair, Vector3(0.96f, 0.78f, 1.0f));
		if (concierge) {
			p.sphere(head, 0.05f, Vector3(0.0f, 0.18f, 0.1f), hair);
			for (float sx : { -0.04f, 0.04f }) {
				p.torus(head, 0.019f, 0.025f, Vector3(sx, 0.122f, -0.105f), "black", Vector3(PI * 0.5f, 0.0f, 0.0f));
			}
			p.box(head, Vector3(0.03f, 0.005f, 0.005f), Vector3(0.0f, 0.124f, -0.108f), "black");
		}
	}

	if (!look.back_text.is_empty()) {
		Label3D *label = memnew(Label3D);
		label->set_text(look.back_text);
		label->set_font_size(48);
		label->set_pixel_size(0.0032f);
		label->set_modulate(look.text_color);
		label->set_outline_size(0);
		label->set_draw_flag(Label3D::FLAG_SHADED, true);
		label->set_draw_flag(Label3D::FLAG_DOUBLE_SIDED, false);
		label->set_position(Vector3(0.0f, 0.36f, gbr ? 0.165f : 0.14f));
		torso->add_child(label);
	}
}

void HumanoidRig::animate(float phase, float amount, float arm_raise) {
	if (!root || seated) {
		return;
	}
	float s = std::sin(phase);
	hip_l->set_rotation(Vector3(s * 0.55f * amount, 0.0f, 0.0f));
	hip_r->set_rotation(Vector3(-s * 0.55f * amount, 0.0f, 0.0f));
	knee_l->set_rotation(Vector3(-std::max(0.0f, std::sin(phase + 1.3f)) * 0.95f * amount, 0.0f, 0.0f));
	knee_r->set_rotation(Vector3(-std::max(0.0f, std::sin(phase + PI + 1.3f)) * 0.95f * amount, 0.0f, 0.0f));
	shoulder_l->set_rotation(Vector3(-s * 0.45f * amount, 0.0f, -0.06f));
	shoulder_r->set_rotation(Vector3(s * 0.45f * amount * (1.0f - std::min(1.0f, arm_raise)) + arm_raise, 0.0f, 0.06f));
	elbow_l->set_rotation(Vector3(0.25f + 0.2f * amount, 0.0f, 0.0f));
	elbow_r->set_rotation(Vector3(arm_raise > 0.0f ? 0.35f : 0.25f + 0.2f * amount, 0.0f, 0.0f));
	torso->set_rotation(Vector3(-0.04f * amount, s * 0.08f * amount, 0.0f));
	float bob = std::fabs(std::cos(phase)) * 0.035f * amount;
	root->set_position(Vector3(0.0f, bob, 0.0f));
}

void HumanoidRig::sit(bool value) {
	if (!root) {
		return;
	}
	seated = value;
	if (value) {
		hip_l->set_rotation(Vector3(1.45f, 0.0f, 0.04f));
		hip_r->set_rotation(Vector3(1.45f, 0.0f, -0.04f));
		knee_l->set_rotation(Vector3(-1.45f, 0.0f, 0.0f));
		knee_r->set_rotation(Vector3(-1.45f, 0.0f, 0.0f));
		shoulder_l->set_rotation(Vector3(0.35f, 0.0f, -0.05f));
		shoulder_r->set_rotation(Vector3(0.35f, 0.0f, 0.05f));
		elbow_l->set_rotation(Vector3(0.9f, 0.0f, 0.0f));
		elbow_r->set_rotation(Vector3(0.9f, 0.0f, 0.0f));
		torso->set_rotation(Vector3(0.05f, 0.0f, 0.0f));
		root->set_position(Vector3(0.0f, -0.45f, 0.0f));
	} else {
		root->set_position(Vector3());
	}
}

void HumanoidRig::look(float yaw, float pitch) {
	if (head) {
		head->set_rotation(Vector3(pitch, yaw, 0.0f));
	}
}

}
