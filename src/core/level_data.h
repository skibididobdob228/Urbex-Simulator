#pragma once

#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <functional>
#include <vector>

namespace urbex {

enum class ObjectiveType {
	Photo,
	Item,
	Reach,
	Action,
};

struct Objective {
	ObjectiveType type = ObjectiveType::Photo;
	godot::String id;
	godot::String title;
	godot::AABB zone;
	bool optional = false;
	bool done = false;
};

struct LightProbe {
	godot::Vector3 position;
	float radius = 6.0f;
	float strength = 0.8f;
	godot::String power_group;
	bool enabled = true;
};

struct Zone {
	godot::Transform3D xf;
	godot::Vector3 half;

	bool contains(const godot::Vector3 &p) const {
		godot::Vector3 l = xf.affine_inverse().xform(p);
		return std::fabs(l.x) <= half.x && std::fabs(l.y) <= half.y && std::fabs(l.z) <= half.z;
	}
};

struct HintZone {
	godot::AABB zone;
	godot::String text;
	bool shown = false;
};

enum class Ambience {
	NightOutdoor,
	Underground,
	Rooftop,
};

struct LevelData {
	godot::String id;
	godot::String title;
	godot::String subtitle;
	godot::String briefing;
	godot::Vector3 spawn_position;
	float spawn_yaw = 0.0f;
	godot::AABB exit_zone;
	godot::String exit_hint;
	std::vector<Objective> objectives;
	std::vector<LightProbe> lights;
	std::vector<godot::AABB> noisy_floors;
	std::vector<HintZone> hints;
	godot::PackedStringArray start_items;
	float ambient_exposure = 0.18f;
	float indoor_exposure = 0.07f;
	std::vector<Zone> shadow_zones;
	float kill_height = -40.0f;
	float gbr_delay = 60.0f;
	godot::Vector3 gbr_spawn;
	std::vector<godot::Vector3> gbr_search_points;
	godot::String legal_note;
	Ambience ambience = Ambience::NightOutdoor;
	godot::AABB nav_bounds;
	std::function<void(double)> tick;
};

} // namespace urbex
