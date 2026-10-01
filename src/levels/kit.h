#pragma once

#include "actors/guard.h"
#include "core/builder.h"
#include "core/level_data.h"
#include "security/devices.h"
#include "world/door.h"
#include "world/interactable.h"

#include <functional>
#include <vector>

namespace urbex {

class UrbexGame;

struct Kit {
	LevelBuilder &b;
	LevelData &d;
	UrbexGame &g;

	Kit(LevelBuilder &builder, LevelData &data, UrbexGame &game) :
			b(builder), d(data), g(game) {}

	Door *door(const godot::Vector3 &local_pos, float local_yaw, Door::Style style, float width, float height, const godot::String &name);
	Pickup *pickup(const godot::Vector3 &local_pos, const godot::String &id, const godot::String &name, const godot::String &description, bool artifact, const char *model);
	NoteBoard *board(const godot::Vector3 &local_pos, float local_yaw, const godot::String &title, const godot::String &body, bool on_post);
	PhotoSpot *spot(const godot::Vector3 &local_pos, const godot::String &id, const godot::String &title, float distance);
	Guard *guard(Guard::Kind kind, const godot::String &name, const godot::Vector3 &global_pos, bool flashlight);
	MotionSensor *pir(const godot::Vector3 &local_pos, const godot::Vector3 &local_target, float range, const godot::String &group, bool dead, const godot::String &name);
	LaserBarrier *beam(const godot::Vector3 &local_a, const godot::Vector3 &local_b, float local_floor, const godot::String &group, const godot::String &name);
	SecurityCamera *camera(const godot::Vector3 &local_pos, float local_yaw, float sweep_deg, float pitch_deg, bool fake, const godot::String &group, const godot::String &name);
	ClimbPoint *climb(const godot::Vector3 &local_pos, const godot::Vector3 &hitbox, const godot::String &label, const godot::Vector3 &global_target, float global_yaw, float duration = 0.8f);
	PowerSwitch *power_box(const godot::Vector3 &local_pos, float local_yaw, const godot::String &group, const godot::String &name);
	ActionPoint *action(const godot::Vector3 &local_pos, const godot::Vector3 &hitbox, const godot::String &prompt, std::function<void(UrbexPlayer *)> fn, bool one_shot = true);
	void street_lamp(const godot::Vector3 &local_pos, float local_yaw, const godot::String &group = godot::String());
	void ceiling_lamp(const godot::Vector3 &local_pos, const godot::Color &color, float range, float energy, const godot::String &group, bool shadow = false);
	void probe(const godot::Vector3 &local_pos, float radius, float strength, const godot::String &group = godot::String());
	void shadow_zone(const godot::Vector3 &local_center, const godot::Vector3 &size);
	void noisy(const godot::Vector3 &local_center, const godot::Vector3 &size, bool visual = true);
	void hint(const godot::Vector3 &local_center, const godot::Vector3 &size, const godot::String &text);
	godot::AABB aabb(const godot::Vector3 &local_center, const godot::Vector3 &size) const;
	void objective_photo(const godot::String &id, const godot::String &title, bool optional = false);
	void objective_item(const godot::String &id, const godot::String &title, bool optional = false);
	void objective_action(const godot::String &id, const godot::String &title, bool optional = false);
	void objective_reach(const godot::String &id, const godot::String &title, const godot::AABB &zone, bool optional = false);
};

} // namespace urbex
