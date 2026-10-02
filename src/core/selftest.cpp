#include "core/selftest.h"

#include "actors/guard.h"
#include "actors/player.h"
#include "core/common.h"
#include "core/game.h"
#include "levels/levels.h"
#include "security/devices.h"
#include "ui/hud.h"
#include "world/door.h"
#include "world/interactable.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_server3d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace urbex {

namespace {

const char *const IGNORED[] = {
	"audio driver",
	"V-Sync",
	"Volumetric fog is only available",
};

bool is_ignored(const String &message) {
	for (const char *s : IGNORED) {
		if (message.find(s) >= 0) {
			return true;
		}
	}
	return false;
}

template <typename T>
std::vector<T *> collect(Node *root, const char *type) {
	std::vector<T *> out;
	if (!root) {
		return out;
	}
	TypedArray<Node> nodes = root->find_children("*", type, true, false);
	for (int i = 0; i < nodes.size(); i++) {
		if (T *t = Object::cast_to<T>(nodes[i])) {
			if (!t->is_queued_for_deletion()) {
				out.push_back(t);
			}
		}
	}
	return out;
}

template <typename T>
std::vector<T *> group(SceneTree *tree, const char *name) {
	std::vector<T *> out;
	TypedArray<Node> nodes = tree->get_nodes_in_group(name);
	for (int i = 0; i < nodes.size(); i++) {
		if (T *t = Object::cast_to<T>(nodes[i])) {
			out.push_back(t);
		}
	}
	return out;
}

String where(const Vector3 &v) {
	return String("(") + String::num(v.x, 1) + ", " + String::num(v.y, 1) + ", " + String::num(v.z, 1) + ")";
}

String label_of(Interactable *it, UrbexPlayer *player) {
	String prompt = it->get_prompt(player);
	if (prompt.is_empty()) {
		prompt = it->get_class();
	}
	return prompt + " " + where(it->get_global_position());
}

template <typename T>
T *alive(uint64_t id) {
	return Object::cast_to<T>(ObjectDB::get_instance(id));
}

}

void SelfTestLogger::_log_error(const String &p_function, const String &p_file, int32_t p_line, const String &p_code, const String &p_rationale, bool, int32_t p_error_type, const TypedArray<Ref<ScriptBacktrace>> &) {
	String kind = p_error_type == ERROR_TYPE_WARNING ? String("WARNING") : String("ERROR");
	String detail = p_rationale.is_empty() ? p_code : p_rationale + String(" [") + p_code + String("]");
	String text = kind + String(": ") + detail + String(" in ") + p_function + String(" ") + p_file + String(":") + String::num_int64(p_line);
	std::lock_guard<std::mutex> guard(lock);
	pending.push_back(text);
}

void SelfTestLogger::_log_message(const String &p_message, bool p_error) {
	if (!p_error) {
		return;
	}
	std::lock_guard<std::mutex> guard(lock);
	pending.push_back(String("STDERR: ") + p_message.strip_edges());
}

std::vector<String> SelfTestLogger::take() {
	std::lock_guard<std::mutex> guard(lock);
	std::vector<String> out;
	out.swap(pending);
	return out;
}

SelfTest::SelfTest(UrbexGame &game, const std::vector<int> &levels) :
		g(game) {
	logger.instantiate();
	OS::get_singleton()->add_logger(logger);
	for (int level : levels) {
		add_level(level);
	}
	add_cycles();
	UtilityFunctions::print("[selftest] steps=", int64_t(steps.size()));
}

SelfTest::~SelfTest() {
	if (logger.is_valid()) {
		OS::get_singleton()->remove_logger(logger);
	}
}

void SelfTest::add(const String &name, std::function<bool(int)> run) {
	steps.push_back({ name, std::move(run) });
}

void SelfTest::add_level(int level) {
	const std::vector<LevelInfo> &levels = level_catalog();
	if (level < 0 || level >= int(levels.size())) {
		return;
	}
	String id = levels[size_t(level)].id;
	add(id + "/load", [this, level](int f) { return step_load(level, f); });
	add(id + "/navigation", [this](int f) { return step_navigation(f); });
	add(id + "/targets", [this](int f) { return step_targets(f); });
	add(id + "/pickups", [this](int f) { return step_pickups(f); });
	add(id + "/actions", [this](int f) { return step_actions(f); });
	add(id + "/power", [this](int f) { return step_power(f); });
	add(id + "/doors", [this](int f) { return step_doors(f); });
	add(id + "/reeds", [this](int f) { return step_reeds(f); });
	add(id + "/climbs", [this](int f) { return step_climbs(f); });
	add(id + "/photos", [this](int f) { return step_photos(f); });
	add(id + "/escape", [this, level](int f) { return step_escape(level, f); });
	add(id + "/restart", [this](int f) { return step_reload(f); });
	add(id + "/alarm", [this](int f) { return step_alarm(f); });
	add(id + "/capture", [this](int f) { return step_capture(f); });
	add(id + "/restart", [this](int f) { return step_reload(f); });
	add(id + "/fall", [this](int f) { return step_fall(f); });
	add(id + "/restart", [this](int f) { return step_reload(f); });
	add(id + "/kill_height", [this](int f) { return step_kill_height(f); });
	add(id + "/restart", [this](int f) { return step_reload(f); });
	add(id + "/pause", [this](int f) { return step_pause(f); });
	add(id + "/menu", [this](int f) { return step_menu(f); });
}

void SelfTest::add_cycles() {
	int count = int(level_catalog().size());
	for (int k = 0; k < count * 2; k++) {
		add(String("cycle/") + String::num_int64(k), [this, k, count](int f) {
			if (f == 0) {
				g.start_level(k % count);
			}
			if (f == 2 && k % 2 == 0) {
				g.restart_level();
			}
			if (f == 4) {
				check(g.mode == UrbexGame::MODE_PLAYING && g.player != nullptr, "level is playing after quick start");
				g.return_to_menu();
			}
			if (f == 6) {
				check(g.mode == UrbexGame::MODE_MENU && g.level_root == nullptr, "menu after quick return");
				return true;
			}
			return false;
		});
	}
}

bool SelfTest::tick() {
	if (index >= steps.size()) {
		flush_errors();
		UtilityFunctions::print("[selftest] checks=", checks, " failures=", failures, " ignored_engine_messages=", ignored_errors);
		UtilityFunctions::print(failures == 0 ? "[selftest] PASS" : "[selftest] FAIL");
		return true;
	}
	Step &s = steps[index];
	scope = s.name;
	if (frame == 0) {
		UtilityFunctions::print("[selftest] ", s.name);
	}
	bool done = s.run(frame);
	frame++;
	if (!done && frame > 5400) {
		check(false, "step timed out");
		done = true;
	}
	if (done) {
		flush_errors();
		index++;
		frame = 0;
	}
	return false;
}

void SelfTest::check(bool ok, const String &what) {
	checks++;
	if (!ok) {
		failures++;
		UtilityFunctions::print("[selftest] FAIL ", scope, ": ", what);
	}
}

void SelfTest::flush_errors() {
	for (const String &e : logger->take()) {
		if (is_ignored(e)) {
			ignored_errors++;
			continue;
		}
		failures++;
		UtilityFunctions::print("[selftest] ENGINE ", scope, ": ", e);
	}
}

bool SelfTest::floor_below(const Vector3 &p, Vector3 &out, bool &crouch) const {
	PhysicsDirectSpaceState3D *space = g.get_space();
	if (!space) {
		return false;
	}
	Ref<PhysicsRayQueryParameters3D> down = PhysicsRayQueryParameters3D::create(p, p - Vector3(0.0f, 4.0f, 0.0f), layer::WORLD);
	Dictionary hit = space->intersect_ray(down);
	if (hit.is_empty() || Vector3(hit["normal"]).y < 0.7f) {
		return false;
	}
	Vector3 feet = Vector3(hit["position"]) + Vector3(0.0f, 0.05f, 0.0f);
	Ref<PhysicsRayQueryParameters3D> up = PhysicsRayQueryParameters3D::create(feet + Vector3(0.0f, 0.1f, 0.0f), feet + Vector3(0.0f, 1.75f, 0.0f), layer::WORLD);
	Dictionary roof = space->intersect_ray(up);
	if (roof.is_empty()) {
		crouch = false;
	} else if (Vector3(roof["position"]).y - feet.y >= 1.05f) {
		crouch = true;
	} else {
		return false;
	}
	out = feet;
	return true;
}

Vector3 SelfTest::closest(const Vector3 &p) const {
	RID map = g.level_root->get_world_3d()->get_navigation_map();
	return NavigationServer3D::get_singleton()->map_get_closest_point(map, p);
}

bool SelfTest::reachable(const Vector3 &from, const Vector3 &to, float tolerance) const {
	RID map = g.level_root->get_world_3d()->get_navigation_map();
	NavigationServer3D *ns = NavigationServer3D::get_singleton();
	Vector3 a = ns->map_get_closest_point(map, from);
	Vector3 b = ns->map_get_closest_point(map, to);
	PackedVector3Array path = ns->map_get_path(map, a, b, true);
	return !path.is_empty() && (path[path.size() - 1] - b).length() <= tolerance;
}

void SelfTest::freeze(bool value) {
	Node::ProcessMode mode = value ? Node::PROCESS_MODE_DISABLED : Node::PROCESS_MODE_INHERIT;
	for (Guard *guard : group<Guard>(g.get_tree(), "urbex_guards")) {
		guard->set_process_mode(mode);
	}
	for (SecurityDevice *d : group<SecurityDevice>(g.get_tree(), "urbex_devices")) {
		d->set_process_mode(mode);
	}
}

void SelfTest::calm() {
	g.alarm_active = false;
	g.alarm_timer = 0.0f;
	if (g.alarm_player) {
		g.alarm_player->stop();
		g.alarm_player->queue_free();
		g.alarm_player = nullptr;
	}
	g.hud->set_alarm(false, String());
}

void SelfTest::close_note() {
	g.hud->hide_note();
	g.note_cooldown = 0.0f;
	if (g.player) {
		g.player->set_controls_enabled(true);
	}
}

void SelfTest::place(const Stand &s) {
	g.player->set_crouch(s.crouch);
	g.player->place(s.feet, s.yaw);
	g.player->set_look(s.yaw, s.pitch);
}

SelfTest::Stand SelfTest::aim(const Vector3 &feet, const Vector3 &target, bool crouch) const {
	Stand s;
	s.feet = feet;
	s.crouch = crouch;
	g.player->set_crouch(crouch);
	g.player->place(feet, 0.0f);
	for (int i = 0; i < 2; i++) {
		Vector3 eye = g.player->get_eye_position();
		Vector3 d = target - eye;
		s.yaw = std::atan2(-d.x, -d.z);
		s.pitch = std::atan2(d.y, Vector2(d.x, d.z).length());
		g.player->set_look(s.yaw, s.pitch);
	}
	return s;
}

Vector3 SelfTest::target_of(Interactable *it) const {
	if (Door *d = Object::cast_to<Door>(it)) {
		return d->get_center();
	}
	for (int i = 0; i < it->get_child_count(); i++) {
		StaticBody3D *body = Object::cast_to<StaticBody3D>(it->get_child(i));
		if (body && (body->get_collision_layer() & layer::INTERACT)) {
			return body->get_global_position();
		}
	}
	return it->get_global_position();
}

bool SelfTest::find_stand(Interactable *it, const Vector3 &target, Stand &out) {
	std::vector<Vector3> candidates;
	std::vector<bool> crouched;
	for (float r : { 0.0f, 0.8f, 1.3f, 1.8f, 2.2f }) {
		int n = r == 0.0f ? 1 : 12;
		for (int k = 0; k < n; k++) {
			float a = float(k) * PI * 2.0f / float(n);
			for (float dy : { 0.0f, -1.2f, -2.4f, 1.2f }) {
				Vector3 sample = target + Vector3(std::cos(a) * r, dy, std::sin(a) * r);
				Vector3 options[2] = { closest(sample), Vector3() };
				bool low[2] = { false, false };
				int count = floor_below(sample + Vector3(0.0f, 0.5f, 0.0f), options[1], low[1]) ? 2 : 1;
				for (int o = 0; o < count; o++) {
					const Vector3 &p = options[o];
					if ((p - target).length() > 3.3f) {
						continue;
					}
					bool duplicate = false;
					for (const Vector3 &c : candidates) {
						if ((c - p).length() < 0.25f) {
							duplicate = true;
							break;
						}
					}
					if (!duplicate) {
						candidates.push_back(p);
						crouched.push_back(low[o]);
					}
				}
			}
		}
	}
	for (size_t i = 0; i < candidates.size(); i++) {
		Stand s = aim(candidates[i], target, crouched[i]);
		place(s);
		if ((g.player->get_eye_position() - target).length() > 2.55f) {
			continue;
		}
		if (g.player->probe_focus() == it) {
			out = s;
			return true;
		}
	}
	return false;
}

bool SelfTest::frame_photo(PhotoSpot *spot, Stand &out) {
	Vector3 t = spot->get_global_position();
	std::vector<Vector3> feet;
	if (spot->has_zone_box()) {
		AABB z = spot->get_zone();
		Rng r(77);
		for (int i = 0; i < 80; i++) {
			feet.push_back(closest(Vector3(z.position.x + r.randf() * z.size.x, z.position.y + 0.4f, z.position.z + r.randf() * z.size.z)));
		}
	} else {
		float reach = std::min(spot->get_max_distance(), 14.0f) * 0.95f;
		for (float r : { 1.5f, 3.0f, 5.0f, 7.5f, 10.0f, 13.0f }) {
			if (r > reach) {
				continue;
			}
			for (int k = 0; k < 16; k++) {
				float a = float(k) * PI / 8.0f;
				for (float dy : { 0.0f, -1.7f, -3.4f, 1.7f, -6.0f }) {
					feet.push_back(closest(t + Vector3(std::cos(a) * r, dy, std::sin(a) * r)));
				}
			}
		}
	}
	Camera3D *cam = g.player->get_camera();
	for (const Vector3 &p : feet) {
		Stand s = aim(p, t);
		place(s);
		Vector3 fwd = -cam->get_global_transform().basis.get_column(2);
		if (spot->in_frame(cam, fwd)) {
			out = s;
			return true;
		}
	}
	return false;
}

void SelfTest::give_everything() {
	for (const char *id : { "magnet", "boltcutter", "key_attic", "key_booth", "key_shelter", "key_roof", "key_intercom" }) {
		if (!g.player->has_item(id)) {
			g.player->give_item(id, item_display_name(id));
		}
	}
}

bool SelfTest::step_load(int level, int f) {
	if (f == 0) {
		stands.clear();
		deferred.clear();
		g.start_level(level);
		check(g.mode == UrbexGame::MODE_PLAYING, "mode is PLAYING after start_level");
		check(g.level_root != nullptr && g.player != nullptr && g.nav_region != nullptr, "level root, player and navigation exist");
		return false;
	}
	if (f < 30) {
		return false;
	}
	bool ready = nav_ready();
	if (!ready && f < 1200) {
		return false;
	}
	check(ready, "navigation map synced");
	close_note();
	check(!g.hud->is_note_open(), "briefing note closes");
	check(!g.transition_state.active, "start fade finished");
	freeze(true);
	return true;
}

bool SelfTest::nav_ready() const {
	RID map = g.level_root->get_world_3d()->get_navigation_map();
	RID region = g.nav_region->get_rid();
	NavigationServer3D *ns = NavigationServer3D::get_singleton();
	Vector3 probe = ns->map_get_closest_point(map, g.level.gbr_spawn);
	return ns->region_get_iteration_id(region) > 0 && ns->region_owns_point(region, probe) && (probe - g.level.gbr_spawn).length() < 3.0f;
}

bool SelfTest::step_reload(int f) {
	if (f == 0) {
		g.restart_level();
		return false;
	}
	if (f < 30) {
		return false;
	}
	bool ready = nav_ready();
	if (!ready && f < 1200) {
		return false;
	}
	check(ready, "navigation map synced after restart");
	check(g.mode == UrbexGame::MODE_PLAYING && !g.get_tree()->is_paused(), "restart resumes play");
	close_note();
	return true;
}

bool SelfTest::step_navigation(int f) {
	(void)f;
	Ref<NavigationMesh> nm = g.nav_region->get_navigation_mesh();
	int polygons = nm.is_valid() ? int(nm->get_polygon_count()) : 0;
	check(polygons > 100, String("navmesh has polygons: ") + String::num_int64(polygons));
	for (Guard *guard : group<Guard>(g.get_tree(), "urbex_guards")) {
		Vector3 p = guard->get_global_position();
		check((closest(p) - p).length() < 1.5f, String("guard on navmesh: ") + guard->get_display_name() + " " + where(p));
		for (const Vector3 &r : guard->get_route_positions()) {
			check(reachable(p, r, 1.5f), String("guard route point reachable: ") + guard->get_display_name() + " -> " + where(r));
		}
	}
	for (const Vector3 &p : g.level.gbr_search_points) {
		check((closest(p) - p).length() < 2.0f, String("GBR search point on navmesh ") + where(p));
		check(reachable(g.level.gbr_spawn, p, 1.5f), String("GBR search point reachable ") + where(p));
	}
	std::vector<PhotoSpot *> spots = group<PhotoSpot>(g.get_tree(), "urbex_photo_spots");
	for (const Objective &o : g.level.objectives) {
		if (o.type != ObjectiveType::Photo) {
			continue;
		}
		bool found = false;
		for (PhotoSpot *s : spots) {
			found = found || s->get_spot_id() == o.id;
		}
		check(found, String("photo spot exists for objective ") + o.id);
	}
	check(g.level.exit_zone.has_volume(), "exit zone defined");
	return true;
}

bool SelfTest::step_targets(int f) {
	(void)f;
	std::vector<Interactable *> items = collect<Interactable>(g.level_root, "Interactable");
	check(!items.empty(), "level has interactables");
	for (Interactable *it : items) {
		Stand s;
		bool ok = find_stand(it, target_of(it), s);
		if (ok) {
			stands[it->get_instance_id()] = s;
		} else {
			deferred.push_back(it->get_instance_id());
		}
		check(!it->get_prompt(g.player).is_empty() || Object::cast_to<Door>(it) != nullptr, String("prompt is not empty for ") + it->get_class() + " " + where(it->get_global_position()));
	}
	g.player->set_crouch(false);
	g.player->place(g.level.spawn_position, g.level.spawn_yaw);
	return true;
}

bool SelfTest::step_pickups(int f) {
	if (f == 0) {
		watched.clear();
		for (Pickup *p : collect<Pickup>(g.level_root, "Pickup")) {
			auto st = stands.find(p->get_instance_id());
			if (st != stands.end()) {
				place(st->second);
			}
			watched.push_back(p->get_instance_id());
			String id = p->get_item_id();
			bool artifact = p->is_artifact();
			int before = g.stats.artifacts;
			p->interact(g.player);
			if (artifact) {
				check(g.stats.artifacts == before + 1, String("artifact counted: ") + id);
			} else {
				check(g.player->has_item(id), String("item in inventory: ") + id);
			}
			for (const Objective &o : g.level.objectives) {
				if (o.type == ObjectiveType::Item && o.id == id) {
					check(o.done, String("item objective done: ") + id);
				}
			}
		}
		return false;
	}
	if (f < 3) {
		return false;
	}
	for (uint64_t id : watched) {
		check(alive<Pickup>(id) == nullptr, "picked item removed from the world");
	}
	return true;
}

bool SelfTest::step_actions(int f) {
	(void)f;
	give_everything();
	for (ActionPoint *a : collect<ActionPoint>(g.level_root, "ActionPoint")) {
		a->interact(g.player);
		check(a->is_used(), String("action point used: ") + label_of(a, g.player));
	}
	for (const Objective &o : g.level.objectives) {
		if (o.type == ObjectiveType::Action) {
			check(o.done, String("action objective done: ") + o.id);
		}
	}
	return true;
}

bool SelfTest::step_power(int f) {
	(void)f;
	for (PowerSwitch *ps : collect<PowerSwitch>(g.level_root, "PowerSwitch")) {
		String grp = ps->get_group();
		check(g.is_powered(grp), String("group powered at start: ") + grp);
		ps->interact(g.player);
		check(!ps->is_on() && !g.is_powered(grp), String("switch cuts power: ") + grp);
		for (SecurityDevice *d : group<SecurityDevice>(g.get_tree(), "urbex_devices")) {
			if (d->get_power_group() == grp) {
				check(!d->is_active(), String("device off without power: ") + d->get_device_name());
			}
		}
		for (const UrbexGame::PoweredLight &pl : g.powered_lights) {
			if (pl.group == grp) {
				check(!pl.light->is_visible(), String("light off without power: ") + grp);
			}
		}
		for (Door *d : group<Door>(g.get_tree(), "urbex_doors")) {
			if (d->get_power_group() == grp && d->has_reed()) {
				check(!d->is_reed_armed(), String("reed disarmed without power: ") + d->get_door_name());
			}
		}
		ps->interact(g.player);
		check(ps->is_on() && g.is_powered(grp), String("switch restores power: ") + grp);
		for (const UrbexGame::PoweredLight &pl : g.powered_lights) {
			if (pl.group == grp) {
				check(pl.light->is_visible(), String("light back on: ") + grp);
			}
		}
	}
	calm();
	return true;
}

bool SelfTest::step_doors(int f) {
	std::vector<Door *> doors = collect<Door>(g.level_root, "Door");
	if (f == 0) {
		give_everything();
		watched.clear();
		for (Door *d : doors) {
			if (d->is_reed_armed()) {
				continue;
			}
			Transform3D xf = d->get_global_transform();
			Vector3 front = d->get_style() == Door::STYLE_HATCH ? xf.origin - Vector3(0.0f, 1.6f, 0.0f) : xf.xform(Vector3(0.0f, 0.05f, 1.3f));
			g.player->place(front, 0.0f);
			if (d->is_jammed()) {
				d->interact(g.player);
				check(!d->is_open(), String("jammed door stays closed: ") + d->get_door_name());
				check(!d->get_prompt(g.player).is_empty(), String("jammed door explains why: ") + d->get_door_name());
				continue;
			}
			for (int i = 0; i < 3 && !d->is_open(); i++) {
				d->interact(g.player);
			}
			check(d->is_open(), String("door opens with all keys: ") + d->get_door_name());
			watched.push_back(d->get_instance_id());
		}
		return false;
	}
	if (f == 240) {
		for (uint64_t id : watched) {
			if (Door *d = alive<Door>(id)) {
				check(d->is_open() && !d->is_moving(), String("door finished opening: ") + d->get_door_name());
				d->interact(g.player);
			}
		}
		return false;
	}
	if (f == 480) {
		for (uint64_t id : watched) {
			if (Door *d = alive<Door>(id)) {
				check(!d->is_open() && !d->is_moving(), String("door closes again: ") + d->get_door_name());
			}
		}
		check(!g.alarm_active, "plain doors do not raise the alarm");
		calm();
		return true;
	}
	return false;
}

bool SelfTest::step_reeds(int f) {
	std::vector<Door *> doors;
	for (Door *d : collect<Door>(g.level_root, "Door")) {
		if (d->has_reed()) {
			doors.push_back(d);
		}
	}
	if (doors.empty()) {
		return true;
	}
	if (f == 0) {
		give_everything();
		for (Door *d : doors) {
			calm();
			g.player->place(d->get_global_transform().xform(Vector3(0.0f, 0.05f, 1.3f)), 0.0f);
			check(d->is_reed_armed(), String("reed armed: ") + d->get_door_name());
			for (int i = 0; i < 3 && !d->is_open(); i++) {
				d->interact(g.player);
			}
			check(d->is_open(), String("reed door opens: ") + d->get_door_name());
			check(g.alarm_active, String("reed raises the alarm: ") + d->get_door_name());
		}
		calm();
		return false;
	}
	if (f == 240) {
		for (Door *d : doors) {
			d->close_door();
		}
		return false;
	}
	if (f == 480) {
		for (ReedSwitch *rs : collect<ReedSwitch>(g.level_root, "ReedSwitch")) {
			rs->interact(g.player);
		}
		for (Door *d : doors) {
			check(!d->is_open(), String("reed door closed: ") + d->get_door_name());
			check(d->is_reed_bypassed(), String("magnet bypasses reed: ") + d->get_door_name());
			g.player->place(d->get_global_transform().xform(Vector3(0.0f, 0.05f, 1.3f)), 0.0f);
			d->interact(g.player);
			check(d->is_open(), String("bypassed door opens: ") + d->get_door_name());
		}
		check(!g.alarm_active, "bypassed reed stays silent");
		calm();
		return true;
	}
	return false;
}

bool SelfTest::step_climbs(int f) {
	std::vector<ClimbPoint *> climbs = collect<ClimbPoint>(g.level_root, "ClimbPoint");
	if (f == 0) {
		give_everything();
		for (Door *d : collect<Door>(g.level_root, "Door")) {
			if (!d->is_jammed() && !d->is_open()) {
				d->open_instantly();
			}
		}
		watched.clear();
		watched_start.clear();
		for (uint64_t id : deferred) {
			Interactable *it = alive<Interactable>(id);
			if (!it) {
				continue;
			}
			Stand s;
			bool ok = find_stand(it, target_of(it), s);
			check(ok, String("player can aim at ") + label_of(it, g.player) + " (doors open)");
			if (ok) {
				stands[id] = s;
			}
		}
		deferred.clear();
	}
	int slot = f / 330;
	int local = f % 330;
	if (slot >= int(climbs.size())) {
		calm();
		return true;
	}
	ClimbPoint *c = climbs[size_t(slot)];
	if (local == 0) {
		calm();
		auto st = stands.find(c->get_instance_id());
		if (st != stands.end()) {
			place(st->second);
		}
		check(!c->get_prompt(g.player).begins_with("Сначала"_u), String("climb is usable: ") + label_of(c, g.player));
		c->interact(g.player);
		check(g.transition_state.active, String("climb starts a transition: ") + label_of(c, g.player));
	}
	if (local == 329) {
		Vector3 p = g.player->get_global_position();
		Vector3 t = c->get_target();
		check(!g.transition_state.active, String("climb transition ends: ") + label_of(c, g.player));
		check((Vector2(p.x, p.z) - Vector2(t.x, t.z)).length() < 1.2f && std::fabs(p.y - t.y) < 1.5f, String("climb lands at target ") + where(t) + " got " + where(p));
		check(!g.player->is_dead(), String("player survives climb: ") + label_of(c, g.player));
		if (c->has_vibration_sensor()) {
			check(g.alarm_active, String("vibration sensor raises alarm: ") + label_of(c, g.player));
		}
	}
	return false;
}

bool SelfTest::step_photos(int f) {
	(void)f;
	std::vector<String> done_ids;
	for (PhotoSpot *spot : group<PhotoSpot>(g.get_tree(), "urbex_photo_spots")) {
		if (std::find(done_ids.begin(), done_ids.end(), spot->get_spot_id()) != done_ids.end()) {
			continue;
		}
		Stand s;
		bool ok = frame_photo(spot, s);
		check(ok, String("photo spot can be framed: ") + spot->get_spot_id() + " " + where(spot->get_global_position()));
		if (!ok) {
			continue;
		}
		place(s);
		g.on_photo(g.player->get_camera());
		check(spot->is_captured(), String("photo captured: ") + spot->get_spot_id());
		done_ids.push_back(spot->get_spot_id());
	}
	for (const Objective &o : g.level.objectives) {
		if (o.type == ObjectiveType::Photo) {
			check(o.done, String("photo objective done: ") + o.id);
		}
	}
	return true;
}

bool SelfTest::step_escape(int level, int f) {
	if (f == 0) {
		calm();
		for (const Objective &o : g.level.objectives) {
			if (!o.optional) {
				check(o.done, String("required objective done before escape: ") + o.id);
			}
		}
		check(g.required_done(), "required objectives complete");
		Vector3 c = g.level.exit_zone.get_center();
		g.player->place(Vector3(c.x, g.level.exit_zone.position.y + 1.3f, c.z), 0.0f);
		g.player->set_physics_process(true);
		return false;
	}
	if (g.mode == UrbexGame::MODE_RESULT || f > 400) {
		check(g.mode == UrbexGame::MODE_RESULT, "escape ends the run");
		check(g.pending_outcome == UrbexGame::OUTCOME_ESCAPED, "outcome is ESCAPED");
		check(g.load_best(level_catalog()[size_t(level)].id) > 0, "best score saved");
		return true;
	}
	return false;
}

bool SelfTest::step_alarm(int f) {
	if (f == 0) {
		check(!g.alarm_active && !g.gbr_spawned && g.stats.alarms == 0, "restart resets the alarm");
		for (const Objective &o : g.level.objectives) {
			check(!o.done, String("restart resets objective ") + o.id);
		}
		g.player->set_physics_process(false);
		g.player->place(Vector3(600.0f, 0.1f, 600.0f), 0.0f);
		Vector3 target = g.level.spawn_position;
		float far = -1.0f;
		for (const Vector3 &p : g.level.gbr_search_points) {
			float d = (p - g.level.gbr_spawn).length();
			if (d > far) {
				far = d;
				target = p;
			}
		}
		g.raise_alarm(target, "selftest");
		check(g.alarm_active && g.alarm_player != nullptr, "alarm raised");
		check(g.stats.alarms == 1, "alarm counted");
		g.alarm_timer = 0.05f;
		return false;
	}
	if (f == 10) {
		check(g.gbr_spawned, "GBR arrives when the timer ends");
		watched.clear();
		watched_start.clear();
		for (Guard *guard : group<Guard>(g.get_tree(), "urbex_guards")) {
			if (guard->get_kind() == Guard::KIND_GBR) {
				Vector3 p = guard->get_global_position();
				check((closest(p) - p).length() < 1.5f, String("GBR spawned on navmesh ") + where(p));
				watched.push_back(guard->get_instance_id());
				watched_start.push_back(p);
			}
		}
		check(watched.size() == 2, String("two GBR guards: ") + String::num_int64(int64_t(watched.size())));
		return false;
	}
	if (f == 390) {
		for (size_t i = 0; i < watched.size(); i++) {
			if (Guard *guard = alive<Guard>(watched[i])) {
				float moved = (guard->get_global_position() - watched_start[i]).length();
				check(moved > 1.5f, String("GBR guard walks after spawn, moved ") + String::num(moved, 1));
			}
		}
		bool alerted = false;
		for (Guard *guard : group<Guard>(g.get_tree(), "urbex_guards")) {
			if (guard->get_kind() != Guard::KIND_GBR && guard->get_state() != Guard::STATE_PATROL) {
				alerted = true;
			}
		}
		check(alerted || group<Guard>(g.get_tree(), "urbex_guards").size() <= 2, "alarm alerts the local guards");
		return true;
	}
	return false;
}

bool SelfTest::step_capture(int f) {
	if (f == 0) {
		Guard *hunter = nullptr;
		for (Guard *guard : group<Guard>(g.get_tree(), "urbex_guards")) {
			if (guard->get_kind() == Guard::KIND_GBR) {
				hunter = guard;
				break;
			}
		}
		check(hunter != nullptr, "a GBR guard to get caught by");
		if (!hunter) {
			return true;
		}
		watched.assign(1, hunter->get_instance_id());
		Vector3 hp = hunter->get_global_position();
		Vector3 eye = hp + Vector3(0.0f, 1.6f, 0.0f);
		Vector3 spot = closest(hp);
		bool found = false;
		for (float r : { 1.6f, 1.2f, 2.2f }) {
			for (int k = 0; k < 8 && !found; k++) {
				float a = float(k) * PI * 0.25f;
				Vector3 p = closest(hp + Vector3(std::cos(a) * r, 0.0f, std::sin(a) * r));
				if (std::fabs(p.y - hp.y) < 0.4f && (p - hp).length() > 0.8f && g.line_of_sight(eye, p + Vector3(0.0f, 1.2f, 0.0f), layer::WORLD | layer::DOORS)) {
					spot = p;
					found = true;
				}
			}
		}
		check(found, String("free spot next to the guard ") + where(hp));
		Stand s = aim(spot, eye);
		g.player->set_physics_process(true);
		place(s);
		g.player->set_flashlight(true);
		return false;
	}
	if (g.end_timer >= 0.0f || g.mode == UrbexGame::MODE_RESULT || f > 1800) {
		bool caught = g.end_timer >= 0.0f || g.mode == UrbexGame::MODE_RESULT;
		Guard *hunter = watched.empty() ? nullptr : alive<Guard>(watched.front());
		String detail;
		if (hunter) {
			detail = String(" guard at ") + where(hunter->get_global_position()) + " state " + String::num_int64(int(hunter->get_state())) + " detection " + String::num(hunter->get_detection(), 2) + " player at " + where(g.player->get_global_position());
		}
		check(caught, String("guard catches a player standing in front of him") + (caught ? String() : detail));
		check(g.pending_outcome == UrbexGame::OUTCOME_CAUGHT, "outcome is CAUGHT");
		return true;
	}
	return false;
}

bool SelfTest::step_fall(int f) {
	if (f == 0) {
		g.player->set_physics_process(true);
		g.player->place(g.level.spawn_position + Vector3(0.0f, 30.0f, 0.0f), g.level.spawn_yaw);
		return false;
	}
	if (g.mode == UrbexGame::MODE_RESULT || f > 600) {
		check(g.mode == UrbexGame::MODE_RESULT, "fall from 30 m ends the run");
		check(g.pending_outcome == UrbexGame::OUTCOME_DIED, "outcome is DIED after a fall");
		return true;
	}
	return false;
}

bool SelfTest::step_kill_height(int f) {
	if (f == 0) {
		Vector3 s = g.level.spawn_position;
		g.player->place(Vector3(s.x, g.level.kill_height - 3.0f, s.z), g.level.spawn_yaw);
		return false;
	}
	if (g.mode == UrbexGame::MODE_RESULT || f > 300) {
		check(g.mode == UrbexGame::MODE_RESULT, "falling below kill height ends the run");
		check(g.pending_outcome == UrbexGame::OUTCOME_DIED, "outcome is DIED below kill height");
		return true;
	}
	return false;
}

bool SelfTest::step_pause(int f) {
	if (f == 0) {
		Ref<InputEventAction> ev;
		ev.instantiate();
		ev->set_action("pause");
		ev->set_pressed(true);
		g._unhandled_input(ev);
		check(g.mode == UrbexGame::MODE_PAUSED && g.get_tree()->is_paused(), "pause key pauses the game");
		return false;
	}
	if (f == 10) {
		Ref<InputEventAction> ev;
		ev.instantiate();
		ev->set_action("pause");
		ev->set_pressed(true);
		g._unhandled_input(ev);
		check(g.mode == UrbexGame::MODE_PLAYING && !g.get_tree()->is_paused(), "pause key resumes the game");
		return true;
	}
	return false;
}

bool SelfTest::step_menu(int f) {
	if (f == 0) {
		g.return_to_menu();
		return false;
	}
	if (f == 5) {
		check(g.mode == UrbexGame::MODE_MENU, "back in menu");
		check(g.level_root == nullptr && g.player == nullptr, "level freed");
		check(!g.get_tree()->is_paused(), "tree not paused in menu");
		return true;
	}
	return false;
}

}
