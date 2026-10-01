#pragma once

#include "actors/humanoid.h"
#include "core/common.h"

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/label3d.hpp>
#include <godot_cpp/classes/navigation_agent3d.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>

#include <vector>

namespace urbex {

class MaterialLibrary;
class Door;

class Guard : public godot::CharacterBody3D {
	GDCLASS(Guard, godot::CharacterBody3D)

public:
	enum State {
		STATE_PATROL,
		STATE_SUSPICIOUS,
		STATE_INVESTIGATE,
		STATE_CHASE,
		STATE_SEARCH,
		STATE_RETURN,
	};

	enum Kind {
		KIND_CHOP,
		KIND_WATCHMAN,
		KIND_GBR,
		KIND_CONCIERGE,
	};

private:
	Kind kind = KIND_CHOP;
	State state = STATE_PATROL;
	godot::String display_name = "ЧОП"_u;
	godot::NavigationAgent3D *agent = nullptr;
	godot::Node3D *visual = nullptr;
	godot::Node3D *look_pivot = nullptr;
	godot::SpotLight3D *flashlight = nullptr;
	godot::Label3D *indicator = nullptr;
	HumanoidRig rig;

	struct RoutePoint {
		godot::Vector3 position;
		float wait = 0.0f;
	};
	std::vector<RoutePoint> route;
	int route_index = 0;
	bool stationary = false;
	godot::Vector3 post_position;
	std::vector<float> post_yaws;
	int post_index = 0;
	float post_timer = 0.0f;
	float post_interval = 6.0f;
	bool sit_at_post = false;
	bool sitting = false;
	std::vector<godot::Vector3> search_pool;

	float yaw = 0.0f;
	float look_offset = 0.0f;
	float look_clock = 0.0f;
	float wait_timer = 0.0f;
	float state_timer = 0.0f;
	float detection = 0.0f;
	float vision_range = 16.0f;
	float half_fov = 55.0f * 0.0174533f;
	float walk_speed = 1.5f;
	float run_speed = 4.5f;
	bool running = false;
	float anim_phase = 0.0f;

	bool sees_player = false;
	float seen_distance = 0.0f;
	bool seen_peripheral = false;
	float seen_exposure = 0.0f;
	float seen_range_value = 1.0f;
	float vision_tick = 0.0f;
	float lost_timer = 0.0f;
	float chase_time = 0.0f;
	bool reported_chase = false;
	godot::Vector3 last_known;
	godot::Vector3 investigate_target;
	godot::Vector3 suspicious_point;
	int search_left = 0;
	float bark_cooldown = 0.0f;
	int bark_index = 0;
	float alarm_called_cooldown = 0.0f;

	godot::Vector3 nav_target;
	bool has_nav_target = false;
	godot::Vector3 stuck_origin;
	float stuck_timer = 0.0f;
	std::vector<Door *> doors;
	bool doors_cached = false;

	void update_vision(float dt);
	void update_detection(float dt);
	void update_state(float dt);
	bool move_to(const godot::Vector3 &target, float speed, float dt);
	void stand_still(float dt);
	void face(const godot::Vector3 &point, float dt, float speed = 4.0f);
	void open_doors_ahead();
	void set_state(State next);
	void bark(const char *const *lines, int count, bool radio = false);
	godot::Vector3 random_search_point(const godot::Vector3 &around, float radius);
	godot::Vector3 forward() const;
	godot::Vector3 eye_position() const;
	bool arrived(const godot::Vector3 &target) const;

protected:
	static void _bind_methods() {}

public:
	void setup(Kind p_kind, const godot::String &name, const MaterialLibrary &materials, bool with_flashlight);
	void add_route_point(const godot::Vector3 &position, float wait = 0.0f);
	void set_post(const godot::Vector3 &position, const std::vector<float> &yaws, float interval, bool sit);
	void set_vision(float range, float fov_deg);
	void set_speeds(float walk, float run);
	void set_search_pool(const std::vector<godot::Vector3> &points) { search_pool = points; }
	void set_initial_yaw(float value);

	void hear(const godot::Vector3 &position, float radius);
	void alert(const godot::Vector3 &position, bool urgent);
	void start_chase_from_alarm(const godot::Vector3 &position);

	float get_detection() const { return detection; }
	State get_state() const { return state; }
	Kind get_kind() const { return kind; }
	bool is_chasing() const { return state == STATE_CHASE; }
	godot::String get_display_name() const { return display_name; }

	void _physics_process(double delta) override;
	void _process(double delta) override;
};

}
