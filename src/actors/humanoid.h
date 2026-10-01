#pragma once

#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

namespace urbex {

class MaterialLibrary;

struct HumanoidLook {
	const char *torso = "uniform";
	const char *legs = "pants";
	const char *skin = "skin";
	const char *hat = "black";
	bool cap = true;
	bool hair = false;
	godot::String back_text;
	godot::Color text_color = godot::Color(0.95f, 0.8f, 0.2f);
	float scale = 1.0f;
};

struct HumanoidRig {
	godot::Node3D *root = nullptr;
	godot::Node3D *hip_l = nullptr;
	godot::Node3D *hip_r = nullptr;
	godot::Node3D *shoulder_l = nullptr;
	godot::Node3D *shoulder_r = nullptr;
	godot::Node3D *head = nullptr;
	godot::Node3D *hand_r = nullptr;

	void build(godot::Node3D *parent, const MaterialLibrary &materials, const HumanoidLook &look);
	void animate(float phase, float amount, float arm_raise = 0.0f);
	void sit(bool seated);
};

} // namespace urbex
