#pragma once

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>

namespace urbex {

class Interactable;

class UrbexPlayer : public godot::CharacterBody3D {
	GDCLASS(UrbexPlayer, godot::CharacterBody3D)

public:
	enum Gait {
		GAIT_IDLE,
		GAIT_SNEAK,
		GAIT_CROUCH,
		GAIT_WALK,
		GAIT_RUN,
	};

private:
	godot::Node3D *head = nullptr;
	godot::Camera3D *camera = nullptr;
	godot::SpotLight3D *flashlight = nullptr;
	godot::SpotLight3D *flashlight_spill = nullptr;
	godot::Node3D *viewmodel = nullptr;
	godot::Node3D *torch_arm = nullptr;
	godot::Node3D *phone_arm = nullptr;
	float torch_blend = 0.0f;
	float phone_blend = 1.0f;
	godot::Vector2 sway;
	godot::Vector2 sway_target;
	godot::CollisionShape3D *collider = nullptr;
	godot::Ref<godot::CapsuleShape3D> capsule;

	float yaw = 0.0f;
	float pitch = 0.0f;
	float mouse_sensitivity = 0.0022f;
	bool crouch_toggled = false;
	bool crouching = false;
	float crouch_blend = 0.0f;
	bool flashlight_on = false;
	float battery = 1.0f;
	bool aiming = false;
	float aim_blend = 0.0f;
	float stamina = 1.0f;
	bool exhausted = false;
	float step_distance = 0.0f;
	int step_index = 0;
	float bob_phase = 0.0f;
	float bob_amount = 0.0f;
	float noise_radius = 0.0f;
	float exposure = 0.0f;
	bool controls_enabled = true;
	bool dead = false;
	float shake = 0.0f;
	float photo_cooldown = 0.0f;
	float flicker_timer = 0.0f;
	float last_vertical_speed = 0.0f;
	bool was_on_floor = true;
	Gait gait = GAIT_IDLE;
	Interactable *focus = nullptr;
	godot::Dictionary inventory;
	int photos_taken = 0;
	bool photo_flash = false;

	void build();
	void build_viewmodel();
	void update_focus();
	void update_crouch(float dt);
	void update_flashlight(float dt);
	void emit_step(float radius);
	void take_photo();
	bool can_stand() const;

protected:
	static void _bind_methods() {}

public:
	void _ready() override;
	void _physics_process(double delta) override;
	void _process(double delta) override;
	void _unhandled_input(const godot::Ref<godot::InputEvent> &event) override;

	void place(const godot::Vector3 &position, float p_yaw);
	void set_look(float p_yaw, float p_pitch);
	void set_flashlight(bool on);
	void set_controls_enabled(bool enabled);
	bool are_controls_enabled() const { return controls_enabled; }
	void set_dead(bool value) { dead = value; }
	void set_mouse_sensitivity(float value) { mouse_sensitivity = value; }

	godot::Camera3D *get_camera() const { return camera; }
	godot::Vector3 get_eye_position() const;
	godot::Vector3 get_chest_position() const;
	godot::Vector3 get_look_direction() const;
	float get_height() const;
	float get_yaw() const { return yaw; }
	bool is_crouching() const { return crouching; }
	bool is_flashlight_on() const { return flashlight_on && battery > 0.0f; }
	bool is_aiming() const { return aiming; }
	bool is_photo_flash() const { return photo_flash; }
	float get_stamina() const { return stamina; }
	float get_battery() const { return battery; }
	float get_noise_radius() const { return noise_radius; }
	float get_exposure() const { return exposure; }
	void set_exposure(float value) { exposure = value; }
	Gait get_gait() const { return gait; }
	godot::String get_gait_name() const;
	int get_photos_taken() const { return photos_taken; }
	Interactable *get_focus() const { return focus; }

	bool has_item(const godot::String &id) const;
	void give_item(const godot::String &id, const godot::String &name);
	void remove_item(const godot::String &id);
	godot::Dictionary get_inventory() const { return inventory; }
	void recharge_battery() { battery = 1.0f; }
	void add_shake(float amount) { shake = std::max(shake, amount); }
	void make_noise(float radius);
};

}
