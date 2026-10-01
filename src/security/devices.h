#pragma once

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/aabb.hpp>

namespace urbex {

class MaterialLibrary;

class SecurityDevice : public godot::Node3D {
	GDCLASS(SecurityDevice, godot::Node3D)

protected:
	godot::String device_name;
	godot::String power_group;
	bool powered = true;
	bool dead = false;
	godot::MeshInstance3D *led = nullptr;
	godot::Ref<godot::StandardMaterial3D> led_material;
	float led_clock = 0.0f;
	float rearm_timer = 0.0f;

	static void _bind_methods() {}
	void make_led(godot::Node3D *parent, const godot::Vector3 &position, float radius);
	void set_led(const godot::Color &color, bool lit);
	void trip(const godot::Vector3 &where);

public:
	void set_device_name(const godot::String &name) { device_name = name; }
	godot::String get_device_name() const { return device_name; }
	void set_power_group(const godot::String &group) { power_group = group; }
	godot::String get_power_group() const { return power_group; }
	void set_powered(bool value) { powered = value; }
	void set_dead(bool value) { dead = value; }
	bool is_active() const { return powered && !dead; }
};

class MotionSensor : public SecurityDevice {
	GDCLASS(MotionSensor, SecurityDevice)

	float range = 8.0f;
	float half_angle = 0.8f;
	float signal = 0.0f;

protected:
	static void _bind_methods() {}

public:
	void setup(const MaterialLibrary &materials, float p_range, float fov_deg);
	float get_signal() const { return signal; }
	void _physics_process(double delta) override;
};

class LaserBarrier : public SecurityDevice {
	GDCLASS(LaserBarrier, SecurityDevice)

	godot::Vector3 a;
	godot::Vector3 b;
	godot::MeshInstance3D *beam = nullptr;
	godot::Ref<godot::StandardMaterial3D> beam_material;

protected:
	static void _bind_methods() {}

public:
	void setup(const MaterialLibrary &materials, const godot::Vector3 &global_a, const godot::Vector3 &global_b, float floor_y);
	void _physics_process(double delta) override;
};

class SecurityCamera : public SecurityDevice {
	GDCLASS(SecurityCamera, SecurityDevice)

	godot::Node3D *pivot = nullptr;
	float base_yaw = 0.0f;
	float sweep = 0.6f;
	float yaw = 0.0f;
	float direction = 1.0f;
	float pause = 0.0f;
	float pitch = -0.35f;
	float range = 16.0f;
	float half_fov = 0.42f;
	float detection = 0.0f;
	bool fake = false;
	bool tracking = false;
	float vision_tick = 0.0f;
	bool sees_player = false;

protected:
	static void _bind_methods() {}

public:
	void setup(const MaterialLibrary &materials, float p_base_yaw, float sweep_deg, float pitch_deg, bool is_fake);
	float get_detection() const { return detection; }
	bool is_fake() const { return fake; }
	void _physics_process(double delta) override;
};

class PhotoSpot : public godot::Node3D {
	GDCLASS(PhotoSpot, godot::Node3D)

	godot::String spot_id;
	godot::String title;
	float max_distance = 14.0f;
	float half_angle = 0.33f;
	bool captured = false;
	bool has_zone = false;
	godot::AABB zone;

protected:
	static void _bind_methods() {}

public:
	void setup(const godot::String &id, const godot::String &p_title, float distance);
	void set_zone(const godot::AABB &aabb);
	godot::String get_spot_id() const { return spot_id; }
	godot::String get_title() const { return title; }
	bool is_captured() const { return captured; }
	void set_captured(bool value) { captured = value; }
	bool in_frame(godot::Node3D *camera, godot::Vector3 forward) const;
};

} // namespace urbex
