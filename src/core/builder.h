#pragma once

#include "core/common.h"
#include "core/materials.h"

#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <vector>

namespace urbex {

struct Opening {
	float offset = 0.0f;
	float width = 1.0f;
	float bottom = 0.0f;
	float top = 2.1f;
};

inline Opening door_gap(float offset, float width = 1.0f, float height = 2.15f) {
	return Opening{ offset, width, 0.0f, height };
}

inline Opening window_gap(float offset, float width = 1.4f, float sill = 0.9f, float top = 2.3f) {
	return Opening{ offset, width, sill, top };
}

class LevelBuilder {
public:
	LevelBuilder(godot::Node3D *dynamic_root, godot::Node3D *static_root, const MaterialLibrary *materials, uint64_t seed);

	godot::Node3D *dynamic_root = nullptr;
	godot::Node3D *static_root = nullptr;
	const MaterialLibrary *materials = nullptr;
	Rng rng;

	godot::Ref<godot::StandardMaterial3D> mat(const char *name) const;

	void push(const godot::Transform3D &local);
	void push_yaw(const godot::Vector3 &origin, float yaw);
	void pop();
	godot::Vector3 g(const godot::Vector3 &local) const;
	godot::Vector3 gdir(const godot::Vector3 &local_dir) const;
	float gyaw(float local_yaw) const;
	godot::Transform3D gxf(const godot::Transform3D &local) const;

	godot::StaticBody3D *box(const godot::Vector3 &center, const godot::Vector3 &size, const char *material, bool collide = true);
	godot::StaticBody3D *box_rot(const godot::Vector3 &center, const godot::Basis &basis, const godot::Vector3 &size, const char *material, bool collide = true);
	godot::MeshInstance3D *visual_box(const godot::Vector3 &center, const godot::Vector3 &size, const char *material, const godot::Basis &basis = godot::Basis(), bool shadows = true);
	godot::MeshInstance3D *visual_mesh(const godot::Ref<godot::Mesh> &mesh, const godot::Transform3D &xf, const char *material, bool shadows = true);
	godot::StaticBody3D *collider_box(const godot::Vector3 &center, const godot::Basis &basis, const godot::Vector3 &size);
	godot::MeshInstance3D *cylinder(const godot::Vector3 &base, float radius, float height, const char *material, bool collide = false, int segments = 12);
	godot::MeshInstance3D *sphere(const godot::Vector3 &center, float radius, const char *material);

	void wall(const godot::Vector3 &a, const godot::Vector3 &b, float y0, float height, float thickness, const char *material, std::vector<Opening> openings = {}, bool collide = true);
	void hex_slab(const godot::Vector3 &center, float radius, float top_y, float thickness, const char *material);
	void slab(float x0, float z0, float x1, float z1, float top_y, float thickness, const char *material, const std::vector<godot::Rect2> &holes = {}, float tile = 6.0f);
	void tiled_box(const godot::Vector3 &center, const godot::Basis &basis, const godot::Vector3 &size, const char *material, float tile, bool collide);
	void ramp(const godot::Vector3 &bottom, const godot::Vector3 &top, float width, const char *material, bool rails = false);
	void railing(const godot::Vector3 &a, const godot::Vector3 &b, float y, float height, const char *material);
	void rubble(const godot::Vector3 &center, float radius, int count, const char *material);
	void tree(const godot::Vector3 &base, float height);
	void block_building(const godot::Vector3 &center, const godot::Vector3 &size, bool roof_details = true);
	void fence(const godot::Vector3 &a, const godot::Vector3 &b, float height, const char *material, std::vector<Opening> gaps = {}, bool barbed = true);
	void bunk_bed(const godot::Vector3 &center, float yaw, float length);
	void bed_frame(const godot::Vector3 &center, float yaw);
	void pipe(const godot::Vector3 &a, const godot::Vector3 &b, float radius, const char *material);
	void lamp_fixture(const godot::Vector3 &pos, bool lit);
	void poster(const godot::Vector3 &center, const godot::Vector3 &normal, float w, float h, const godot::String &title, const godot::String &body, const godot::Color &accent);

	godot::Label3D *text(const godot::Vector3 &pos, const godot::Vector3 &normal, const godot::String &txt, float height, const godot::Color &color, bool shaded = true, bool dynamic = false);
	godot::Label3D *graffiti(const godot::Vector3 &pos, const godot::Vector3 &normal, const godot::String &txt, float height = 0.6f);
	godot::OmniLight3D *omni(const godot::Vector3 &pos, const godot::Color &color, float range, float energy, bool shadow = false);
	godot::SpotLight3D *spot(const godot::Vector3 &pos, const godot::Vector3 &target, const godot::Color &color, float range, float energy, float angle_deg, bool shadow = true);

	template <typename T>
	T *add_dynamic(T *node, const godot::Vector3 &local_pos, float local_yaw = 0.0f) {
		dynamic_root->add_child(node);
		node->set_global_transform(godot::Transform3D(godot::Basis(godot::Vector3(0, 1, 0), gyaw(local_yaw)), g(local_pos)));
		return node;
	}

private:
	godot::Node3D *current() const;

	std::vector<godot::Node3D *> parents;
	std::vector<godot::Transform3D> transforms;
};

godot::Basis basis_looking(const godot::Vector3 &dir);

}
