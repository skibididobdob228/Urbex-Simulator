#include "actors/humanoid.h"

#include "core/materials.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>

#include <cmath>

using namespace godot;

namespace urbex {

namespace {

MeshInstance3D *part(Node3D *parent, const Ref<Mesh> &mesh, const Vector3 &pos, const Ref<Material> &material) {
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(material);
	mi->set_position(pos);
	parent->add_child(mi);
	return mi;
}

Ref<BoxMesh> box(const Vector3 &size) {
	Ref<BoxMesh> m;
	m.instantiate();
	m->set_size(size);
	return m;
}

Node3D *pivot(Node3D *parent, const Vector3 &pos) {
	Node3D *n = memnew(Node3D);
	n->set_position(pos);
	parent->add_child(n);
	return n;
}

} // namespace

void HumanoidRig::build(Node3D *parent, const MaterialLibrary &materials, const HumanoidLook &look) {
	root = memnew(Node3D);
	root->set_scale(Vector3(look.scale, look.scale, look.scale));
	parent->add_child(root);

	Ref<Material> torso = materials.get(look.torso);
	Ref<Material> legs = materials.get(look.legs);
	Ref<Material> skin = materials.get(look.skin);

	hip_l = pivot(root, Vector3(-0.11f, 0.9f, 0.0f));
	hip_r = pivot(root, Vector3(0.11f, 0.9f, 0.0f));
	part(hip_l, box(Vector3(0.15f, 0.88f, 0.17f)), Vector3(0.0f, -0.44f, 0.0f), legs);
	part(hip_r, box(Vector3(0.15f, 0.88f, 0.17f)), Vector3(0.0f, -0.44f, 0.0f), legs);
	part(hip_l, box(Vector3(0.16f, 0.08f, 0.26f)), Vector3(0.0f, -0.86f, -0.04f), materials.get("black"));
	part(hip_r, box(Vector3(0.16f, 0.08f, 0.26f)), Vector3(0.0f, -0.86f, -0.04f), materials.get("black"));

	part(root, box(Vector3(0.46f, 0.64f, 0.27f)), Vector3(0.0f, 1.22f, 0.0f), torso);

	shoulder_l = pivot(root, Vector3(-0.3f, 1.5f, 0.0f));
	shoulder_r = pivot(root, Vector3(0.3f, 1.5f, 0.0f));
	part(shoulder_l, box(Vector3(0.12f, 0.6f, 0.14f)), Vector3(0.0f, -0.29f, 0.0f), torso);
	part(shoulder_r, box(Vector3(0.12f, 0.6f, 0.14f)), Vector3(0.0f, -0.29f, 0.0f), torso);
	part(shoulder_l, box(Vector3(0.1f, 0.1f, 0.1f)), Vector3(0.0f, -0.62f, 0.0f), skin);
	part(shoulder_r, box(Vector3(0.1f, 0.1f, 0.1f)), Vector3(0.0f, -0.62f, 0.0f), skin);
	hand_r = pivot(shoulder_r, Vector3(0.0f, -0.62f, -0.05f));

	head = pivot(root, Vector3(0.0f, 1.62f, 0.0f));
	Ref<SphereMesh> skull;
	skull.instantiate();
	skull->set_radius(0.12f);
	skull->set_height(0.26f);
	skull->set_radial_segments(12);
	skull->set_rings(6);
	part(head, skull, Vector3(0.0f, 0.08f, 0.0f), skin);
	part(head, box(Vector3(0.1f, 0.08f, 0.1f)), Vector3(0.0f, -0.04f, 0.0f), skin);

	if (look.cap) {
		Ref<CylinderMesh> cap;
		cap.instantiate();
		cap->set_top_radius(0.125f);
		cap->set_bottom_radius(0.13f);
		cap->set_height(0.08f);
		cap->set_radial_segments(12);
		part(head, cap, Vector3(0.0f, 0.18f, 0.0f), materials.get(look.hat));
		part(head, box(Vector3(0.18f, 0.02f, 0.1f)), Vector3(0.0f, 0.15f, -0.15f), materials.get(look.hat));
	}
	if (look.hair) {
		Ref<SphereMesh> hair;
		hair.instantiate();
		hair->set_radius(0.135f);
		hair->set_height(0.2f);
		hair->set_radial_segments(12);
		hair->set_rings(6);
		part(head, hair, Vector3(0.0f, 0.15f, 0.03f), materials.get("hair_gray"));
	}

	if (!look.back_text.is_empty()) {
		Label3D *label = memnew(Label3D);
		label->set_text(look.back_text);
		label->set_font_size(48);
		label->set_pixel_size(0.0035f);
		label->set_modulate(look.text_color);
		label->set_outline_size(0);
		label->set_draw_flag(Label3D::FLAG_SHADED, true);
		label->set_draw_flag(Label3D::FLAG_DOUBLE_SIDED, false);
		label->set_position(Vector3(0.0f, 1.32f, 0.14f));
		root->add_child(label);
	}
}

void HumanoidRig::animate(float phase, float amount, float arm_raise) {
	if (!root) {
		return;
	}
	float swing = std::sin(phase) * 0.6f * amount;
	hip_l->set_rotation(Vector3(swing, 0.0f, 0.0f));
	hip_r->set_rotation(Vector3(-swing, 0.0f, 0.0f));
	shoulder_l->set_rotation(Vector3(-swing * 0.7f, 0.0f, 0.0f));
	shoulder_r->set_rotation(Vector3(swing * 0.7f - arm_raise, 0.0f, 0.0f));
	float bob = std::fabs(std::cos(phase)) * 0.04f * amount;
	root->set_position(Vector3(0.0f, bob, 0.0f));
}

void HumanoidRig::sit(bool seated) {
	if (!root) {
		return;
	}
	if (seated) {
		hip_l->set_rotation(Vector3(1.45f, 0.0f, 0.0f));
		hip_r->set_rotation(Vector3(1.45f, 0.0f, 0.0f));
		root->set_position(Vector3(0.0f, -0.42f, 0.0f));
	} else {
		root->set_position(Vector3());
	}
}

} // namespace urbex
