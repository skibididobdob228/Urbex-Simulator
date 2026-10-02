#pragma once

#include <godot_cpp/classes/logger.hpp>
#include <godot_cpp/classes/script_backtrace.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

namespace urbex {

class UrbexGame;
class Interactable;
class PhotoSpot;

class SelfTestLogger : public godot::Logger {
	GDCLASS(SelfTestLogger, godot::Logger)

	std::mutex lock;
	std::vector<godot::String> pending;

protected:
	static void _bind_methods() {}

public:
	void _log_error(const godot::String &p_function, const godot::String &p_file, int32_t p_line, const godot::String &p_code, const godot::String &p_rationale, bool p_editor_notify, int32_t p_error_type, const godot::TypedArray<godot::Ref<godot::ScriptBacktrace>> &p_script_backtraces) override;
	void _log_message(const godot::String &p_message, bool p_error) override;
	std::vector<godot::String> take();
};

class SelfTest {
public:
	SelfTest(UrbexGame &game, const std::vector<int> &levels);
	~SelfTest();

	bool tick();
	int get_failures() const { return failures; }

private:
	struct Step {
		godot::String name;
		std::function<bool(int)> run;
	};

	struct Stand {
		godot::Vector3 feet;
		float yaw = 0.0f;
		float pitch = 0.0f;
		bool crouch = false;
	};

	UrbexGame &g;
	godot::Ref<SelfTestLogger> logger;
	std::vector<Step> steps;
	size_t index = 0;
	int frame = 0;
	int checks = 0;
	int failures = 0;
	int ignored_errors = 0;
	godot::String scope;
	std::map<uint64_t, Stand> stands;
	std::vector<uint64_t> watched;
	std::vector<uint64_t> deferred;
	std::vector<godot::Vector3> watched_start;

	void add(const godot::String &name, std::function<bool(int)> run);
	void add_level(int level);
	void add_cycles();
	void check(bool ok, const godot::String &what);
	void flush_errors();

	godot::Vector3 closest(const godot::Vector3 &p) const;
	bool floor_below(const godot::Vector3 &p, godot::Vector3 &out, bool &crouch) const;
	bool reachable(const godot::Vector3 &from, const godot::Vector3 &to, float tolerance) const;
	void freeze(bool value);
	void calm();
	void close_note();
	void place(const Stand &s);
	Stand aim(const godot::Vector3 &feet, const godot::Vector3 &target, bool crouch = false) const;
	bool find_stand(Interactable *it, const godot::Vector3 &target, Stand &out);
	bool frame_photo(PhotoSpot *spot, Stand &out);
	godot::Vector3 target_of(Interactable *it) const;
	void give_everything();

	bool nav_ready() const;
	bool step_load(int level, int f);
	bool step_reload(int f);
	bool step_navigation(int f);
	bool step_targets(int f);
	bool step_pickups(int f);
	bool step_actions(int f);
	bool step_power(int f);
	bool step_doors(int f);
	bool step_reeds(int f);
	bool step_climbs(int f);
	bool step_photos(int f);
	bool step_escape(int level, int f);
	bool step_alarm(int f);
	bool step_capture(int f);
	bool step_fall(int f);
	bool step_kill_height(int f);
	bool step_pause(int f);
	bool step_menu(int f);
};

}
