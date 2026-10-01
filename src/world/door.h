#pragma once

#include "world/interactable.h"

#include <godot_cpp/classes/animatable_body3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

namespace urbex {

class Door : public Interactable {
	GDCLASS(Door, Interactable)

public:
	enum Style {
		STYLE_WOOD,
		STYLE_METAL,
		STYLE_BLAST,
		STYLE_HATCH,
		STYLE_GATE,
	};

private:
	Style style = STYLE_WOOD;
	float width = 1.0f;
	float height = 2.1f;
	float thickness = 0.06f;
	godot::AnimatableBody3D *panel = nullptr;
	godot::Node3D *panel_visual = nullptr;
	godot::MeshInstance3D *padlock_mesh = nullptr;
	godot::MeshInstance3D *reed_mesh = nullptr;
	float angle = 0.0f;
	float target_angle = 0.0f;
	bool open = false;
	bool moving = false;
	godot::String key_id;
	bool padlock = false;
	bool reed = false;
	bool reed_bypassed = false;
	godot::String power_group;
	bool powered = true;
	bool jammed = false;
	godot::String jammed_text;
	godot::String door_name;
	bool fast_swing = false;
	bool exit_button = false;

	bool opens_from_inside(UrbexPlayer *player) const;

	void update_panel();
	float swing_speed() const;
	void start_open(const godot::Vector3 &from, bool by_guard);
	godot::String locked_reason(UrbexPlayer *player) const;

protected:
	static void _bind_methods() {}

public:
	void setup(Style p_style, float p_width, float p_height, const godot::String &name, const godot::Ref<godot::Material> &panel_material, const godot::Ref<godot::Material> &detail_material);
	void set_key(const godot::String &id) { key_id = id; }
	void set_padlock(bool value);
	void set_reed(bool value, const godot::String &group, const godot::Ref<godot::Material> &material);
	void set_jammed(bool value, const godot::String &text, bool block_navigation = true);
	void set_exit_button(bool value) { exit_button = value; }
	void close_door();
	void set_powered(bool value) { powered = value; }
	void bypass_reed();
	void open_instantly();

	bool is_open() const { return open; }
	bool has_reed() const { return reed; }
	bool is_reed_armed() const { return reed && !reed_bypassed && powered; }
	bool is_reed_bypassed() const { return reed_bypassed; }
	bool is_locked() const { return !key_id.is_empty() || padlock || jammed; }
	godot::String get_power_group() const { return power_group; }
	godot::String get_door_name() const { return door_name; }
	godot::Vector3 get_center() const;
	Style get_style() const { return style; }

	void open_by_guard(const godot::Vector3 &from);

	godot::String get_prompt(UrbexPlayer *player) const override;
	void interact(UrbexPlayer *player) override;

	void _physics_process(double delta) override;
};

} // namespace urbex
