#pragma once

#include "core/common.h"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <functional>

namespace urbex {

class UrbexPlayer;
class Door;

class Interactable : public godot::Node3D {
	GDCLASS(Interactable, godot::Node3D)

protected:
	static void _bind_methods() {}

public:
	virtual godot::String get_prompt(UrbexPlayer *player) const { return godot::String(); }
	virtual void interact(UrbexPlayer *player) {}

	godot::StaticBody3D *add_hitbox(const godot::Vector3 &size, const godot::Vector3 &offset);
	static Interactable *find_from(godot::Object *collider);
};

class Pickup : public Interactable {
	GDCLASS(Pickup, Interactable)

	godot::String item_id;
	godot::String item_name;
	godot::String description;
	bool artifact = false;

protected:
	static void _bind_methods() {}

public:
	void setup(const godot::String &id, const godot::String &name, const godot::String &desc, bool is_artifact);
	godot::String get_item_id() const { return item_id; }
	godot::String get_item_name() const { return item_name; }
	godot::String get_description() const { return description; }
	bool is_artifact() const { return artifact; }

	godot::String get_prompt(UrbexPlayer *player) const override;
	void interact(UrbexPlayer *player) override;
};

class ActionPoint : public Interactable {
	GDCLASS(ActionPoint, Interactable)

	godot::String prompt;
	godot::String done_prompt;
	godot::String required_item;
	godot::String missing_text;
	bool one_shot = true;
	bool used = false;
	std::function<void(UrbexPlayer *)> action;

protected:
	static void _bind_methods() {}

public:
	void setup(const godot::String &p_prompt, std::function<void(UrbexPlayer *)> p_action, bool p_one_shot = true);
	void set_requirement(const godot::String &item, const godot::String &missing);
	void set_done_prompt(const godot::String &text) { done_prompt = text; }
	bool is_used() const { return used; }

	godot::String get_prompt(UrbexPlayer *player) const override;
	void interact(UrbexPlayer *player) override;
};

class NoteBoard : public Interactable {
	GDCLASS(NoteBoard, Interactable)

	godot::String title;
	godot::String body;
	godot::String verb;

protected:
	static void _bind_methods() {}

public:
	void setup(const godot::String &p_title, const godot::String &p_body, const godot::String &p_verb = "Читать"_u);
	godot::String get_prompt(UrbexPlayer *player) const override;
	void interact(UrbexPlayer *player) override;
};

class ClimbPoint : public Interactable {
	GDCLASS(ClimbPoint, Interactable)

	godot::String label;
	godot::Vector3 target;
	float target_yaw = 0.0f;
	float duration = 0.8f;
	godot::String required_item;
	godot::String missing_text;
	Door *required_door = nullptr;
	godot::String door_closed_text;
	godot::String vibration_sensor;
	float noise_radius = 0.0f;
	godot::String sound;
	godot::String arrival_message;

protected:
	static void _bind_methods() {}

public:
	void setup(const godot::String &p_label, const godot::Vector3 &p_target, float p_target_yaw, float p_duration = 0.8f);
	void set_requirement(const godot::String &item, const godot::String &missing);
	void set_required_door(Door *door, const godot::String &closed_text);
	void set_vibration_sensor(const godot::String &name) { vibration_sensor = name; }
	void set_noise(float radius, const godot::String &snd);
	void set_arrival_message(const godot::String &text) { arrival_message = text; }

	godot::String get_prompt(UrbexPlayer *player) const override;
	void interact(UrbexPlayer *player) override;
};

class PowerSwitch : public Interactable {
	GDCLASS(PowerSwitch, Interactable)

	godot::String group;
	godot::String switch_name;
	bool on = true;
	godot::Node3D *lever = nullptr;

protected:
	static void _bind_methods() {}

public:
	void setup(const godot::String &p_group, const godot::String &name, godot::Node3D *lever_node);
	bool is_on() const { return on; }
	godot::String get_prompt(UrbexPlayer *player) const override;
	void interact(UrbexPlayer *player) override;
};

class ReedSwitch : public Interactable {
	GDCLASS(ReedSwitch, Interactable)

	Door *door = nullptr;

protected:
	static void _bind_methods() {}

public:
	void setup(Door *p_door);
	godot::String get_prompt(UrbexPlayer *player) const override;
	void interact(UrbexPlayer *player) override;
};

} // namespace urbex
