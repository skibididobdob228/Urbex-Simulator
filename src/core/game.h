#pragma once

#include "core/level_data.h"
#include "core/materials.h"
#include "core/sound_bank.h"

#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/navigation_region3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>

#include <memory>
#include <vector>

namespace urbex {

class UrbexPlayer;
class UrbexHud;
class MainMenu;
class Guard;
class SelfTest;

struct RunStats {
	float time = 0.0f;
	int photos = 0;
	int objective_photos = 0;
	int artifacts = 0;
	int alarms = 0;
	int spotted = 0;
	int chases = 0;
	bool gbr_arrived = false;
};

class UrbexGame : public godot::Node {
	GDCLASS(UrbexGame, godot::Node)

public:
	enum Mode {
		MODE_MENU,
		MODE_PLAYING,
		MODE_PAUSED,
		MODE_RESULT,
	};

	enum Outcome {
		OUTCOME_ESCAPED,
		OUTCOME_CAUGHT,
		OUTCOME_DIED,
	};

private:
	friend class SelfTest;
	static UrbexGame *singleton;

	MaterialLibrary material_library;
	SoundBank sound_bank;
	Mode mode = MODE_MENU;
	int level_index = -1;
	LevelData level;
	RunStats stats;

	bool audio_muted = false;
	int quit_countdown = -1;
	int quit_code = 0;
	uint64_t quit_after_msec = 0;
	godot::Node3D *level_root = nullptr;
	godot::NavigationRegion3D *nav_region = nullptr;
	UrbexPlayer *player = nullptr;
	UrbexHud *hud = nullptr;
	MainMenu *menu = nullptr;
	godot::AudioStreamPlayer *ambience = nullptr;
	godot::AudioStreamPlayer *ambience2 = nullptr;
	godot::AudioStreamPlayer *heartbeat = nullptr;
	godot::AudioStreamPlayer3D *alarm_player = nullptr;

	struct PoweredLight {
		godot::Light3D *light = nullptr;
		godot::String group;
		float energy = 1.0f;
	};
	std::vector<PoweredLight> powered_lights;
	std::vector<godot::String> unpowered_groups;

	bool alarm_active = false;
	float alarm_timer = 0.0f;
	godot::Vector3 alarm_position;
	godot::String alarm_source;
	bool gbr_spawned = false;
	bool chop_called = false;

	struct Transition {
		bool active = false;
		float timer = 0.0f;
		float duration = 0.0f;
		bool moved = false;
		godot::Vector3 target;
		float yaw = 0.0f;
		godot::String message;
	};
	Transition transition_state;

	float end_timer = -1.0f;
	Outcome pending_outcome = OUTCOME_ESCAPED;
	godot::String pending_reason;
	float drip_timer = 3.0f;
	float note_cooldown = 0.0f;
	float detection_display = 0.0f;
	bool exit_announced = false;

	godot::PackedStringArray user_args;
	godot::String save_path = "user://urbex_save.cfg";
	godot::PackedStringArray selftest_levels;
	bool selftest_requested = false;
	std::unique_ptr<SelfTest> selftest;
	godot::String autotest_level;
	godot::String screenshot_path;
	int automation_frames = 0;
	godot::Vector3 shot_position;
	float shot_yaw = 0.0f;
	float shot_pitch = 0.0f;
	bool shot_override = false;
	bool shot_flashlight = false;
	bool shot_bright = false;
	bool walk_start = false;
	godot::Vector3 walk_position;
	float walk_seconds = 0.0f;
	bool walk_run = false;
	bool auto_snap = false;
	int auto_use = 0;
	godot::PackedStringArray auto_items;
	godot::Ref<godot::Environment> environment;
	godot::Node3D *celestial_light = nullptr;
	float sensitivity = 1.0f;

	void setup_input();
	void setup_automation();
	void run_automation();
	void clear_level();
	void build_environment();
	void update_playing(double delta);
	void update_objectives();
	void update_hud(double delta);
	void update_alarm(double delta);
	void update_transition(double delta);
	void spawn_gbr();
	void finish(Outcome outcome, const godot::String &reason);
	void show_results();
	godot::String objectives_text() const;
	bool required_done() const;
	void save_result(int score);
	int load_best(const godot::String &level_id) const;
	void set_mouse_captured(bool captured);

protected:
	static void _bind_methods();

public:
	UrbexGame();
	~UrbexGame();

	static UrbexGame *get_singleton() { return singleton; }

	void _ready() override;
	void _exit_tree() override;
	void _notification(int p_what);
	void silence_audio();
	void request_quit(int code);
	void _process(double delta) override;
	void _unhandled_input(const godot::Ref<godot::InputEvent> &event) override;

	void start_level(int index);
	void restart_level();
	void return_to_menu();
	void resume();
	void quit_game();
	void set_sensitivity(float value);
	float get_sensitivity() const { return sensitivity; }
	int get_best_score(int index) const;

	const MaterialLibrary &get_materials() const { return material_library; }
	const SoundBank &get_sounds() const { return sound_bank; }
	UrbexPlayer *get_player() const { return player; }
	LevelData &get_level() { return level; }
	Mode get_mode() const { return mode; }
	bool is_playing() const { return mode == MODE_PLAYING; }
	godot::Node3D *get_level_root() const { return level_root; }
	godot::PhysicsDirectSpaceState3D *get_space() const;

	void emit_noise(const godot::Vector3 &position, float radius, bool from_player);
	void raise_alarm(const godot::Vector3 &position, const godot::String &source);
	bool is_alarm_active() const { return alarm_active; }
	void on_guard_spotted(Guard *guard, const godot::Vector3 &where);
	void on_guard_lost(Guard *guard);
	void player_caught(const godot::String &who);
	void player_died(const godot::String &reason);
	void notify(const godot::String &text, const godot::Color &color = godot::Color(0.92f, 0.92f, 0.88f));
	void say(godot::Node3D *speaker, const godot::String &who, const godot::String &line);
	void show_note(const godot::String &title, const godot::String &body);
	void on_photo(godot::Camera3D *camera);
	godot::String photo_hint(godot::Camera3D *camera) const;
	void on_item(const godot::String &id, const godot::String &name, bool artifact);
	void complete_objective(const godot::String &id);
	void set_power(const godot::String &group, bool on, const godot::Vector3 &where);
	bool is_powered(const godot::String &group) const;
	void register_light(godot::Light3D *light, const godot::String &group);
	void transition(const godot::Vector3 &target, float yaw, float duration, const godot::String &message = godot::String());
	bool is_transitioning() const { return transition_state.active; }
	void play_sound(const godot::String &name, const godot::Vector3 &position, float volume_db = 0.0f, float pitch = 1.0f, float max_distance = 40.0f);
	void play_ui_sound(const godot::String &name, float volume_db = 0.0f, float pitch = 1.0f);
	float exposure_for(const godot::Vector3 &observer, const godot::Vector3 &observer_forward, bool observer_light) const;
	bool is_noisy_floor(const godot::Vector3 &position) const;
	bool line_of_sight(const godot::Vector3 &from, const godot::Vector3 &to, uint32_t mask) const;
	float now() const;
};

}
