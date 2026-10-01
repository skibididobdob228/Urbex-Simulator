#include "core/game.h"

#include "actors/guard.h"
#include "actors/player.h"
#include "core/builder.h"
#include "core/common.h"
#include "levels/levels.h"
#include "security/devices.h"
#include "ui/hud.h"
#include "world/door.h"
#include "world/interactable.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/navigation_mesh.hpp>
#include <godot_cpp/classes/navigation_server3d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/procedural_sky_material.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sky.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace urbex {

UrbexGame *UrbexGame::singleton = nullptr;

namespace {

const char *SAVE_PATH = "user://urbex_save.cfg";

String format_time(float seconds) {
	int s = int(seconds);
	int m = s / 60;
	s %= 60;
	return String::num_int64(m) + ":" + (s < 10 ? "0" : "") + String::num_int64(s);
}

} // namespace

void UrbexGame::_bind_methods() {
}

UrbexGame::UrbexGame() {
	if (!singleton) {
		singleton = this;
	}
}

UrbexGame::~UrbexGame() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

void UrbexGame::setup_input() {
	InputMap *map = InputMap::get_singleton();
	auto key = [map](const char *action, Key k) {
		if (!map->has_action(action)) {
			map->add_action(action);
		}
		Ref<InputEventKey> e;
		e.instantiate();
		e->set_physical_keycode(k);
		map->action_add_event(action, e);
	};
	auto mouse = [map](const char *action, MouseButton b) {
		if (!map->has_action(action)) {
			map->add_action(action);
		}
		Ref<InputEventMouseButton> e;
		e.instantiate();
		e->set_button_index(b);
		map->action_add_event(action, e);
	};
	key("move_forward", KEY_W);
	key("move_forward", KEY_UP);
	key("move_back", KEY_S);
	key("move_back", KEY_DOWN);
	key("move_left", KEY_A);
	key("move_left", KEY_LEFT);
	key("move_right", KEY_D);
	key("move_right", KEY_RIGHT);
	key("jump", KEY_SPACE);
	key("run", KEY_SHIFT);
	key("crouch", KEY_C);
	key("crouch_hold", KEY_CTRL);
	key("sneak", KEY_ALT);
	key("sneak", KEY_Z);
	key("flashlight", KEY_F);
	key("interact", KEY_E);
	key("pause", KEY_ESCAPE);
	key("pause", KEY_P);
	mouse("aim", MOUSE_BUTTON_RIGHT);
	key("aim", KEY_Q);
	mouse("photo", MOUSE_BUTTON_LEFT);
}

void UrbexGame::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	singleton = this;
	set_process_mode(PROCESS_MODE_ALWAYS);
	setup_input();

	Ref<ConfigFile> cfg;
	cfg.instantiate();
	if (cfg->load(SAVE_PATH) == OK) {
		sensitivity = float(double(cfg->get_value("settings", "sensitivity", 1.0)));
	}

	material_library.build();
	sound_bank.build();

	ambience = memnew(AudioStreamPlayer);
	add_child(ambience);
	ambience2 = memnew(AudioStreamPlayer);
	add_child(ambience2);
	heartbeat = memnew(AudioStreamPlayer);
	heartbeat->set_stream(sound_bank.get("heartbeat"));
	heartbeat->set_volume_db(-80.0f);
	add_child(heartbeat);

	hud = memnew(UrbexHud);
	add_child(hud);
	hud->build();
	hud->set_gameplay_visible(false);
	hud->pause_resume->connect("pressed", callable_mp(this, &UrbexGame::resume));
	hud->pause_restart->connect("pressed", callable_mp(this, &UrbexGame::restart_level));
	hud->pause_menu_button->connect("pressed", callable_mp(this, &UrbexGame::return_to_menu));
	hud->get_result_retry()->connect("pressed", callable_mp(this, &UrbexGame::restart_level));
	hud->result_menu->connect("pressed", callable_mp(this, &UrbexGame::return_to_menu));

	menu = memnew(MainMenu);
	add_child(menu);
	menu->build();

	setup_automation();
	if (!autotest_level.is_empty()) {
		const std::vector<LevelInfo> &levels = level_catalog();
		for (size_t i = 0; i < levels.size(); i++) {
			if (autotest_level == levels[i].id) {
				start_level(int(i));
				return;
			}
		}
		UtilityFunctions::printerr("Unknown level id for autotest: ", autotest_level);
		get_tree()->quit(2);
		return;
	}
	mode = MODE_MENU;
	set_mouse_captured(false);
	menu->refresh();
}

void UrbexGame::_exit_tree() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	for (AudioStreamPlayer *p : { ambience, ambience2, heartbeat }) {
		if (p) {
			p->stop();
			p->set_stream(Ref<AudioStream>());
		}
	}
	if (alarm_player) {
		alarm_player->stop();
	}
}

void UrbexGame::setup_automation() {
	user_args = OS::get_singleton()->get_cmdline_user_args();
	for (int i = 0; i < user_args.size(); i++) {
		String a = user_args[i];
		if (a.begins_with("--autotest=")) {
			autotest_level = a.get_slice("=", 1);
		} else if (a.begins_with("--shot=")) {
			screenshot_path = a.get_slice("=", 1);
		} else if (a.begins_with("--pos=")) {
			PackedFloat64Array v = a.get_slice("=", 1).split_floats(",");
			if (v.size() == 3) {
				shot_position = Vector3(float(v[0]), float(v[1]), float(v[2]));
				shot_override = true;
			}
		} else if (a.begins_with("--yaw=")) {
			shot_yaw = float(a.get_slice("=", 1).to_float()) * DEG;
		} else if (a.begins_with("--pitch=")) {
			shot_pitch = float(a.get_slice("=", 1).to_float()) * DEG;
		} else if (a == "--light") {
			shot_flashlight = true;
		} else if (a.begins_with("--start=")) {
			PackedFloat64Array v = a.get_slice("=", 1).split_floats(",");
			if (v.size() == 3) {
				walk_position = Vector3(float(v[0]), float(v[1]), float(v[2]));
				walk_start = true;
			}
		} else if (a.begins_with("--walk=")) {
			walk_seconds = float(a.get_slice("=", 1).to_float());
		} else if (a.begins_with("--give=")) {
			auto_items.push_back(a.get_slice("=", 1));
		} else if (a == "--snap") {
			auto_snap = true;
		} else if (a.begins_with("--use=")) {
			auto_use = int(a.get_slice("=", 1).to_int());
		} else if (a == "--run") {
			walk_run = true;
		} else if (a == "--bright") {
			shot_bright = true;
		} else if (a.begins_with("--frames=")) {
			automation_frames = int(a.get_slice("=", 1).to_int());
		}
	}
	if (automation_frames <= 0) {
		automation_frames = screenshot_path.is_empty() ? 480 : 90;
	}
}

void UrbexGame::run_automation() {
	static int frame = 0;
	if (autotest_level.is_empty() && !screenshot_path.is_empty()) {
		frame++;
		if (frame == automation_frames) {
			Ref<Image> img = get_viewport()->get_texture()->get_image();
			img->save_png(screenshot_path);
			get_tree()->quit(0);
		}
		return;
	}
	if (autotest_level.is_empty() || mode == MODE_MENU) {
		return;
	}
	frame++;
	if (frame == 2) {
		hud->hide_note();
		hud->show_center(String(), 0.0f);
		if (player) {
			player->set_controls_enabled(true);
			if (shot_override) {
				player->place(shot_position, shot_yaw);
				player->set_look(shot_yaw, shot_pitch);
				player->set_physics_process(false);
			}
			player->set_flashlight(shot_flashlight);
		}
		for (int i = 0; player && i < auto_items.size(); i++) {
			player->give_item(auto_items[i], item_display_name(auto_items[i]));
		}
		if (walk_start && player) {
			player->place(walk_position, shot_yaw);
			player->set_look(shot_yaw, shot_pitch);
		}
		if (walk_seconds > 0.0f) {
			Input::get_singleton()->action_press("move_forward");
			if (walk_run) {
				Input::get_singleton()->action_press("run");
			}
		}
		if (shot_bright && environment.is_valid()) {
			environment->set_ambient_light_energy(1.2f);
			environment->set_fog_enabled(false);
			if (DirectionalLight3D *dl = Object::cast_to<DirectionalLight3D>(celestial_light)) {
				dl->set_param(Light3D::PARAM_ENERGY, 1.5f);
			}
		}
	}
	if (frame == 20) {
		RID map = level_root->get_world_3d()->get_navigation_map();
		NavigationServer3D *ns = NavigationServer3D::get_singleton();
		Ref<NavigationMesh> nm = nav_region->get_navigation_mesh();
		UtilityFunctions::print("[autotest] level=", level.id, " navmesh polygons=", nm.is_valid() ? nm->get_polygon_count() : 0);
		TypedArray<Node> guards = get_tree()->get_nodes_in_group("urbex_guards");
		TypedArray<Node> devices = get_tree()->get_nodes_in_group("urbex_devices");
		TypedArray<Node> doors = get_tree()->get_nodes_in_group("urbex_doors");
		TypedArray<Node> spots = get_tree()->get_nodes_in_group("urbex_photo_spots");
		UtilityFunctions::print("[autotest] guards=", guards.size(), " devices=", devices.size(), " doors=", doors.size(), " photo_spots=", spots.size());
		int bad_paths = 0;
		for (int i = 0; i < guards.size(); i++) {
			Guard *g = Object::cast_to<Guard>(guards[i]);
			Vector3 from = g->get_global_position();
			Vector3 snapped = ns->map_get_closest_point(map, from);
			if ((snapped - from).length() > 1.5f) {
				UtilityFunctions::print("[autotest] WARN guard off navmesh: ", g->get_display_name(), " at ", from, " closest ", snapped);
				bad_paths++;
			}
		}
		for (const Objective &o : level.objectives) {
			if (o.type == ObjectiveType::Photo) {
				bool found = false;
				for (int i = 0; i < spots.size(); i++) {
					PhotoSpot *s = Object::cast_to<PhotoSpot>(spots[i]);
					if (s && s->get_spot_id() == o.id) {
						found = true;
					}
				}
				if (!found) {
					UtilityFunctions::print("[autotest] WARN missing photo spot for objective ", o.id);
					bad_paths++;
				}
			}
		}
		Vector3 exit_center = level.exit_zone.get_center();
		PackedVector3Array path = ns->map_get_path(map, ns->map_get_closest_point(map, level.spawn_position), ns->map_get_closest_point(map, exit_center), true);
		UtilityFunctions::print("[autotest] spawn->exit path points=", path.size());
		for (const Vector3 &p : level.gbr_search_points) {
			Vector3 c = ns->map_get_closest_point(map, p);
			if ((c - p).length() > 2.0f) {
				UtilityFunctions::print("[autotest] WARN gbr search point off navmesh ", p, " -> ", c);
				bad_paths++;
			}
		}
		UtilityFunctions::print("[autotest] warnings=", bad_paths);
	}
	if (frame >= 30 && frame < 30 + auto_use * 20 && (frame - 30) % 20 == 0 && player) {
		Interactable *f = player->get_focus();
		UtilityFunctions::print("[use] focus=", f ? f->get_prompt(player) : String("none"));
		if (f) {
			f->interact(player);
		}
	}
	if (frame == 35 && auto_snap && player) {
		on_photo(player->get_camera());
		UtilityFunctions::print("[snap] hint=", photo_hint(player->get_camera()));
	}
	if ((frame == 36 && auto_snap) || (auto_use > 0 && frame == 30 + auto_use * 20)) {
		for (const Objective &o : level.objectives) {
			UtilityFunctions::print("[objective] ", o.id, " done=", o.done);
		}
		Dictionary inv = player->get_inventory();
		UtilityFunctions::print("[inventory] ", inv);
	}
	if (walk_seconds > 0.0f && stats.time > walk_seconds) {
		Input::get_singleton()->action_release("move_forward");
		Input::get_singleton()->action_release("run");
		walk_seconds = -1.0f;
	}
	if (walk_start && frame % 30 == 0 && player) {
		UtilityFunctions::print("[walk] t=", stats.time, " pos=", player->get_global_position(), " on_floor=", player->is_on_floor());
	}
	if (frame % 120 == 0 && screenshot_path.is_empty() && !walk_start) {
		TypedArray<Node> guards = get_tree()->get_nodes_in_group("urbex_guards");
		for (int i = 0; i < guards.size(); i++) {
			Guard *g = Object::cast_to<Guard>(guards[i]);
			UtilityFunctions::print("[autotest] t=", stats.time, " ", g->get_display_name(), " pos=", g->get_global_position(), " state=", int(g->get_state()), " det=", g->get_detection());
		}
	}
	if (frame == automation_frames) {
		if (!screenshot_path.is_empty()) {
			Ref<Image> img = get_viewport()->get_texture()->get_image();
			img->save_png(screenshot_path);
			UtilityFunctions::print("[autotest] screenshot saved ", screenshot_path);
		}
		UtilityFunctions::print("[autotest] done mode=", int(mode), " time=", stats.time, " alarms=", stats.alarms, " spotted=", stats.spotted);
		get_tree()->quit(0);
	}
}

void UrbexGame::set_mouse_captured(bool captured) {
	Input::get_singleton()->set_mouse_mode(captured ? Input::MOUSE_MODE_CAPTURED : Input::MOUSE_MODE_VISIBLE);
}

void UrbexGame::set_sensitivity(float value) {
	sensitivity = value;
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	cfg->load(SAVE_PATH);
	cfg->set_value("settings", "sensitivity", value);
	cfg->save(SAVE_PATH);
}

int UrbexGame::load_best(const String &level_id) const {
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	if (cfg->load(SAVE_PATH) != OK) {
		return 0;
	}
	return int(cfg->get_value("best", level_id, 0));
}

int UrbexGame::get_best_score(int index) const {
	const std::vector<LevelInfo> &levels = level_catalog();
	if (index < 0 || index >= int(levels.size())) {
		return 0;
	}
	return load_best(levels[size_t(index)].id);
}

void UrbexGame::save_result(int score) {
	if (score <= load_best(level.id)) {
		return;
	}
	Ref<ConfigFile> cfg;
	cfg.instantiate();
	cfg->load(SAVE_PATH);
	cfg->set_value("best", level.id, score);
	cfg->save(SAVE_PATH);
}

void UrbexGame::clear_level() {
	if (level_root) {
		remove_child(level_root);
		level_root->queue_free();
	}
	level_root = nullptr;
	nav_region = nullptr;
	player = nullptr;
	alarm_player = nullptr;
	powered_lights.clear();
	unpowered_groups.clear();
	ambience->stop();
	ambience2->stop();
	heartbeat->stop();
}

void UrbexGame::build_environment() {
	WorldEnvironment *we = memnew(WorldEnvironment);
	Ref<Environment> env;
	env.instantiate();
	Ref<ProceduralSkyMaterial> skymat;
	skymat.instantiate();
	Ref<Sky> sky;
	sky.instantiate();
	sky->set_material(skymat);
	env->set_background(Environment::BG_SKY);
	env->set_sky(sky);
	env->set_ambient_source(Environment::AMBIENT_SOURCE_COLOR);
	env->set_reflection_source(Environment::REFLECTION_SOURCE_SKY);
	env->set_tonemapper(Environment::TONE_MAPPER_FILMIC);
	env->set_glow_enabled(true);
	env->set_glow_intensity(0.7f);
	env->set_glow_bloom(0.05f);
	env->set_ssao_enabled(true);
	env->set_ssao_intensity(1.6f);
	env->set_fog_enabled(true);
	env->set_volumetric_fog_enabled(true);
	env->set_volumetric_fog_albedo(Color(0.8f, 0.8f, 0.85f));
	env->set_volumetric_fog_anisotropy(0.4f);
	env->set_volumetric_fog_length(48.0f);

	DirectionalLight3D *celestial = memnew(DirectionalLight3D);
	celestial->set_shadow(true);
	celestial->set_param(Light3D::PARAM_SHADOW_MAX_DISTANCE, 90.0f);

	switch (level.ambience) {
		case Ambience::NightOutdoor:
		case Ambience::Underground:
			skymat->set_sky_top_color(Color(0.006f, 0.009f, 0.02f));
			skymat->set_sky_horizon_color(Color(0.11f, 0.075f, 0.06f));
			skymat->set_ground_horizon_color(Color(0.07f, 0.05f, 0.04f));
			skymat->set_ground_bottom_color(Color(0.01f, 0.01f, 0.012f));
			skymat->set_sky_curve(0.12f);
			skymat->set_sun_angle_max(4.0f);
			skymat->set_sky_energy_multiplier(1.0f);
			env->set_ambient_light_color(Color(0.42f, 0.46f, 0.6f));
			env->set_ambient_light_energy(level.ambience == Ambience::Underground ? 0.05f : 0.09f);
			env->set_tonemap_exposure(1.15f);
			env->set_fog_light_color(Color(0.05f, 0.05f, 0.07f));
			env->set_fog_density(level.ambience == Ambience::Underground ? 0.02f : 0.0022f);
			env->set_fog_sky_affect(0.3f);
			env->set_volumetric_fog_density(level.ambience == Ambience::Underground ? 0.035f : 0.012f);
			celestial->set_color(Color(0.62f, 0.72f, 1.0f));
			celestial->set_param(Light3D::PARAM_ENERGY, 0.16f);
			celestial->set_rotation(Vector3(-42.0f * DEG, 35.0f * DEG, 0.0f));
			break;
		case Ambience::Rooftop:
			skymat->set_sky_top_color(Color(0.05f, 0.07f, 0.17f));
			skymat->set_sky_horizon_color(Color(0.85f, 0.42f, 0.24f));
			skymat->set_ground_horizon_color(Color(0.3f, 0.17f, 0.12f));
			skymat->set_ground_bottom_color(Color(0.02f, 0.02f, 0.025f));
			skymat->set_sky_curve(0.08f);
			skymat->set_sun_angle_max(10.0f);
			skymat->set_sky_energy_multiplier(1.0f);
			env->set_ambient_light_color(Color(0.55f, 0.5f, 0.6f));
			env->set_ambient_light_energy(0.18f);
			env->set_tonemap_exposure(1.0f);
			env->set_fog_light_color(Color(0.35f, 0.25f, 0.25f));
			env->set_fog_density(0.0025f);
			env->set_fog_sky_affect(0.1f);
			env->set_volumetric_fog_density(0.004f);
			celestial->set_color(Color(1.0f, 0.6f, 0.42f));
			celestial->set_param(Light3D::PARAM_ENERGY, 0.45f);
			celestial->set_rotation(Vector3(-7.0f * DEG, 90.0f * DEG, 0.0f));
			break;
	}
	we->set_environment(env);
	level_root->add_child(we);
	level_root->add_child(celestial);
	environment = env;
	celestial_light = celestial;
}

void UrbexGame::start_level(int index) {
	const std::vector<LevelInfo> &levels = level_catalog();
	if (index < 0 || index >= int(levels.size())) {
		return;
	}
	get_tree()->set_pause(false);
	clear_level();
	level = LevelData();
	stats = RunStats();
	alarm_active = false;
	alarm_timer = 0.0f;
	gbr_spawned = false;
	chop_called = false;
	transition_state = Transition();
	end_timer = -1.0f;
	exit_announced = false;
	detection_display = 0.0f;
	level_index = index;

	level_root = memnew(Node3D);
	level_root->set_name("Level");
	level_root->set_process_mode(PROCESS_MODE_PAUSABLE);
	add_child(level_root);

	nav_region = memnew(NavigationRegion3D);
	nav_region->set_name("Static");
	level_root->add_child(nav_region);
	Ref<NavigationMesh> navmesh;
	navmesh.instantiate();
	navmesh->set_parsed_geometry_type(NavigationMesh::PARSED_GEOMETRY_STATIC_COLLIDERS);
	navmesh->set_collision_mask(layer::WORLD);
	navmesh->set_agent_radius(0.3f);
	navmesh->set_agent_height(1.8f);
	navmesh->set_agent_max_climb(0.3f);
	navmesh->set_agent_max_slope(46.0f);
	navmesh->set_cell_size(0.1f);
	navmesh->set_cell_height(0.1f);
	nav_region->set_navigation_mesh(navmesh);

	player = memnew(UrbexPlayer);
	player->set_name("Player");
	level_root->add_child(player);

	LevelBuilder builder(level_root, nav_region, &material_library, 1337u + uint64_t(index) * 7919u);
	levels[size_t(index)].build(builder, level, *this);
	build_environment();

	if (level.nav_bounds.has_volume()) {
		navmesh->set_filter_baking_aabb(level.nav_bounds);
	}
	nav_region->bake_navigation_mesh(false);

	player->place(level.spawn_position, level.spawn_yaw);
	player->set_controls_enabled(true);
	player->set_dead(false);
	for (int i = 0; i < level.start_items.size(); i++) {
		String id = level.start_items[i];
		player->give_item(id, item_display_name(id));
	}

	switch (level.ambience) {
		case Ambience::NightOutdoor:
			ambience->set_stream(sound_bank.get("wind"));
			ambience->set_volume_db(-17.0f);
			ambience->play();
			break;
		case Ambience::Underground:
			ambience->set_stream(sound_bank.get("wind"));
			ambience->set_volume_db(-24.0f);
			ambience->play();
			ambience2->set_stream(sound_bank.get("hum"));
			ambience2->set_volume_db(-30.0f);
			ambience2->play();
			break;
		case Ambience::Rooftop:
			ambience->set_stream(sound_bank.get("wind"));
			ambience->set_volume_db(-12.0f);
			ambience->play();
			break;
	}
	heartbeat->set_volume_db(-80.0f);
	heartbeat->play();

	menu->set_visible(false);
	hud->set_gameplay_visible(true);
	hud->hide_result();
	hud->show_pause(false);
	hud->set_alarm(false, String());
	hud->set_level_title(level.title);
	hud->set_fade(1.0f);
	transition_state.active = true;
	transition_state.timer = 0.3f;
	transition_state.duration = 0.0f;
	transition_state.moved = true;
	mode = MODE_PLAYING;
	set_mouse_captured(true);
	update_hud(0.0);
	show_note(level.title, level.briefing);
}

void UrbexGame::restart_level() {
	if (level_index >= 0) {
		start_level(level_index);
	}
}

void UrbexGame::return_to_menu() {
	get_tree()->set_pause(false);
	clear_level();
	hud->set_gameplay_visible(false);
	hud->hide_result();
	hud->show_pause(false);
	hud->hide_note();
	menu->set_visible(true);
	menu->refresh();
	mode = MODE_MENU;
	set_mouse_captured(false);
}

void UrbexGame::resume() {
	if (mode != MODE_PAUSED) {
		return;
	}
	mode = MODE_PLAYING;
	get_tree()->set_pause(false);
	hud->show_pause(false);
	set_mouse_captured(true);
}

void UrbexGame::quit_game() {
	get_tree()->quit();
}

void UrbexGame::_unhandled_input(const Ref<InputEvent> &event) {
	if (Engine::get_singleton()->is_editor_hint() || event.is_null()) {
		return;
	}
	if (mode == MODE_PLAYING) {
		if (hud->is_note_open()) {
			if (event->is_action_pressed("pause") || event->is_action_pressed("interact")) {
				hud->hide_note();
				note_cooldown = 0.2f;
				get_viewport()->set_input_as_handled();
			}
			return;
		}
		if (event->is_action_pressed("pause")) {
			mode = MODE_PAUSED;
			get_tree()->set_pause(true);
			hud->show_pause(true);
			hud->set_viewfinder(false, String());
			set_mouse_captured(false);
			get_viewport()->set_input_as_handled();
		}
	} else if (mode == MODE_PAUSED) {
		if (event->is_action_pressed("pause")) {
			resume();
			get_viewport()->set_input_as_handled();
		}
	}
}

void UrbexGame::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !hud) {
		return;
	}
	hud->tick(delta);
	run_automation();
	if (mode == MODE_PLAYING) {
		update_playing(delta);
	}
}

void UrbexGame::update_playing(double delta) {
	float dt = float(delta);
	if (!player) {
		return;
	}
	update_transition(dt);
	if (end_timer >= 0.0f) {
		end_timer -= dt;
		hud->set_fade(clampf(1.0f - end_timer / 1.6f, 0.0f, 1.0f) * 0.85f);
		if (end_timer < 0.0f) {
			show_results();
		}
		return;
	}
	stats.time += dt;

	if (note_cooldown > 0.0f) {
		note_cooldown -= dt;
	}
	bool blocked = hud->is_note_open() || transition_state.active || note_cooldown > 0.0f;
	player->set_controls_enabled(!blocked);

	Vector3 chest = player->get_chest_position();
	float e = level.ambient_exposure;
	for (const Zone &z : level.shadow_zones) {
		if (z.contains(chest)) {
			e = level.indoor_exposure;
			break;
		}
	}
	for (const LightProbe &lp : level.lights) {
		if (!lp.enabled || !is_powered(lp.power_group)) {
			continue;
		}
		float d = (lp.position - chest).length();
		if (d < lp.radius) {
			float k = 1.0f - d / lp.radius;
			e = std::max(e, lp.strength * std::sqrt(k));
		}
	}
	if (player->is_flashlight_on()) {
		e += 0.45f;
	}
	if (player->is_crouching()) {
		e *= 0.75f;
	}
	player->set_exposure(clampf(e, 0.05f, 1.0f));

	update_alarm(dt);
	update_objectives();

	for (HintZone &h : level.hints) {
		if (!h.shown && h.zone.has_point(player->get_global_position())) {
			h.shown = true;
			notify(h.text, Color(1.0f, 0.85f, 0.45f));
		}
	}
	if (level.tick) {
		level.tick(delta);
	}

	if (level.ambience == Ambience::Underground) {
		drip_timer -= dt;
		if (drip_timer <= 0.0f && level_root) {
			Rng rng(Engine::get_singleton()->get_process_frames());
			drip_timer = rng.range(2.0f, 6.0f);
			Vector3 p = player->get_global_position() + Vector3(rng.range(-8.0f, 8.0f), 2.5f, rng.range(-8.0f, 8.0f));
			if (p.y < 0.0f) {
				play_sound("drip", p, -10.0f, rng.range(0.8f, 1.3f));
			}
		}
	}
	update_hud(delta);
}

void UrbexGame::update_transition(double delta) {
	if (!transition_state.active) {
		return;
	}
	float dt = float(delta);
	Transition &t = transition_state;
	t.timer += dt;
	const float fade = 0.3f;
	float total = fade * 2.0f + t.duration;
	if (!t.moved) {
		hud->set_fade(clampf(t.timer / fade, 0.0f, 1.0f));
		if (t.timer >= fade) {
			t.moved = true;
			player->place(t.target, t.yaw);
			if (!t.message.is_empty()) {
				notify(t.message, Color(0.8f, 0.85f, 0.95f));
			}
		}
	} else {
		float out = t.timer - fade - t.duration;
		hud->set_fade(clampf(1.0f - out / fade, 0.0f, 1.0f));
	}
	if (t.timer >= total) {
		t.active = false;
		hud->set_fade(0.0f);
	}
}

void UrbexGame::transition(const Vector3 &target, float yaw, float duration, const String &message) {
	transition_state = Transition();
	transition_state.active = true;
	transition_state.target = target;
	transition_state.yaw = yaw;
	transition_state.duration = duration;
	transition_state.message = message;
	player->set_controls_enabled(false);
}

void UrbexGame::update_alarm(double delta) {
	if (!alarm_active) {
		return;
	}
	if (!gbr_spawned) {
		alarm_timer -= float(delta);
		if (alarm_timer <= 0.0f) {
			spawn_gbr();
		}
	}
	String text = String("ТРЕВОГА · "_u) + alarm_source;
	if (!gbr_spawned) {
		text += String("\nГБР приедет через "_u) + format_time(std::max(0.0f, alarm_timer));
	} else {
		text += "\nГБР на объекте"_u;
	}
	hud->set_alarm(true, text);
}

void UrbexGame::spawn_gbr() {
	gbr_spawned = true;
	stats.gbr_arrived = true;
	for (int i = 0; i < 2; i++) {
		Guard *g = memnew(Guard);
		g->setup(Guard::KIND_GBR, "ГБР"_u, material_library, true);
		g->set_search_pool(level.gbr_search_points);
		for (const Vector3 &p : level.gbr_search_points) {
			g->add_route_point(p, 2.5f);
		}
		level_root->add_child(g);
		g->set_global_position(level.gbr_spawn + Vector3(float(i) * 1.2f - 0.6f, 0.2f, float(i) * 0.8f));
		g->start_chase_from_alarm(alarm_position);
	}
	play_sound("radio", level.gbr_spawn, 4.0f, 1.0f, 80.0f);
	notify("Приехала ГБР: двое с фонарями прочёсывают объект"_u, Color(1.0f, 0.4f, 0.3f));
}

void UrbexGame::update_objectives() {
	Vector3 p = player->get_global_position();
	for (Objective &o : level.objectives) {
		if (!o.done && o.type == ObjectiveType::Reach && o.zone.has_point(p)) {
			o.done = true;
			notify(String("Выполнено: "_u) + o.title, Color(0.6f, 0.95f, 0.6f));
			play_ui_sound("pickup", -8.0f, 1.3f);
		}
	}
	if (required_done()) {
		if (!exit_announced) {
			exit_announced = true;
			notify(String("Главное сделано. Теперь уходи: "_u) + level.exit_hint, Color(0.6f, 0.95f, 0.6f));
			hud->show_center("Уходи с объекта"_u, 3.0f);
		}
		if (level.exit_zone.has_point(p)) {
			finish(OUTCOME_ESCAPED, String());
		}
	}
}

bool UrbexGame::required_done() const {
	for (const Objective &o : level.objectives) {
		if (!o.optional && !o.done) {
			return false;
		}
	}
	return true;
}

String UrbexGame::objectives_text() const {
	String text;
	for (const Objective &o : level.objectives) {
		String mark = o.done ? String("[x] ") : String("[  ] ");
		String opt = o.optional ? String(" (доп.)"_u) : String();
		text += mark + o.title + opt + "\n";
	}
	if (required_done()) {
		text += String("→ Уйти: "_u) + level.exit_hint;
	}
	return text;
}

void UrbexGame::update_hud(double delta) {
	float dt = float(delta);
	String prompt;
	if (!transition_state.active && !hud->is_note_open() && player->get_focus() && player->are_controls_enabled()) {
		prompt = player->get_focus()->get_prompt(player);
	}
	hud->set_prompt(prompt);
	bool aiming = player->is_aiming() && !hud->is_note_open();
	hud->set_viewfinder(aiming, aiming ? photo_hint(player->get_camera()) : String());
	hud->set_objectives(objectives_text());

	String st = player->get_gait_name();
	if (player->is_flashlight_on()) {
		st += " · фонарь"_u;
	}
	if (player->is_crouching()) {
		st += " · присед"_u;
	}
	hud->set_status(st);
	hud->set_bars(player->get_stamina(), clampf(player->get_noise_radius() / 13.0f, 0.0f, 1.0f), player->get_exposure(), player->get_battery());

	float det = 0.0f;
	bool chase = false;
	TypedArray<Node> guards = get_tree()->get_nodes_in_group("urbex_guards");
	for (int i = 0; i < guards.size(); i++) {
		Guard *g = Object::cast_to<Guard>(guards[i]);
		if (!g) {
			continue;
		}
		det = std::max(det, g->get_detection());
		chase = chase || g->is_chasing();
	}
	TypedArray<Node> cams = get_tree()->get_nodes_in_group("urbex_cameras");
	for (int i = 0; i < cams.size(); i++) {
		SecurityCamera *c = Object::cast_to<SecurityCamera>(cams[i]);
		if (c && c->is_active()) {
			det = std::max(det, c->get_detection());
		}
	}
	detection_display = lerpf(detection_display, det, 1.0f - std::exp(-8.0f * dt));
	hud->set_detection(detection_display, chase);
	float hb = chase ? 1.0f : detection_display;
	heartbeat->set_volume_db(hb > 0.05f ? lerpf(-30.0f, -4.0f, hb) : -80.0f);
	heartbeat->set_pitch_scale(chase ? 1.35f : 1.0f);

	String inv = "Снаряжение:"_u;
	Dictionary items = player->get_inventory();
	Array keys = items.keys();
	if (keys.is_empty()) {
		inv += "\n  пусто"_u;
	}
	for (int i = 0; i < keys.size(); i++) {
		inv += String("\n  ") + String(items[keys[i]]);
	}
	inv += String("\nСнимков: "_u) + String::num_int64(player->get_photos_taken());
	hud->set_inventory(inv);
	if (!alarm_active) {
		hud->set_alarm(false, String());
	}
}

PhysicsDirectSpaceState3D *UrbexGame::get_space() const {
	if (!level_root) {
		return nullptr;
	}
	return level_root->get_world_3d()->get_direct_space_state();
}

bool UrbexGame::line_of_sight(const Vector3 &from, const Vector3 &to, uint32_t mask) const {
	PhysicsDirectSpaceState3D *space = get_space();
	if (!space) {
		return false;
	}
	Ref<PhysicsRayQueryParameters3D> q = PhysicsRayQueryParameters3D::create(from, to, mask);
	Dictionary hit = space->intersect_ray(q);
	if (hit.is_empty()) {
		return true;
	}
	Vector3 pos = hit["position"];
	return (pos - to).length() < 0.35f;
}

float UrbexGame::exposure_for(const Vector3 &observer, const Vector3 &observer_forward, bool observer_light) const {
	if (!player) {
		return 0.0f;
	}
	float e = player->get_exposure();
	if (observer_light) {
		Vector3 to = player->get_chest_position() - observer;
		float d = to.length();
		if (d < 18.0f && d > 0.01f) {
			float c = observer_forward.normalized().dot(to / d);
			if (c > std::cos(16.0f * DEG)) {
				e = std::max(e, 0.95f - d / 40.0f);
			} else if (c > std::cos(28.0f * DEG)) {
				e = std::max(e, 0.6f - d / 50.0f);
			}
		}
	}
	return clampf(e, 0.0f, 1.0f);
}

bool UrbexGame::is_noisy_floor(const Vector3 &position) const {
	Vector3 p = position + Vector3(0.0f, 0.2f, 0.0f);
	for (const AABB &box : level.noisy_floors) {
		if (box.has_point(p)) {
			return true;
		}
	}
	return false;
}

void UrbexGame::emit_noise(const Vector3 &position, float radius, bool from_player) {
	if (!from_player || mode != MODE_PLAYING) {
		return;
	}
	TypedArray<Node> guards = get_tree()->get_nodes_in_group("urbex_guards");
	for (int i = 0; i < guards.size(); i++) {
		if (Guard *g = Object::cast_to<Guard>(guards[i])) {
			g->hear(position, radius);
		}
	}
}

void UrbexGame::raise_alarm(const Vector3 &position, const String &source) {
	if (mode != MODE_PLAYING || end_timer >= 0.0f) {
		return;
	}
	stats.alarms++;
	alarm_position = position;
	alarm_source = source;
	if (!alarm_active) {
		alarm_active = true;
		alarm_timer = level.gbr_delay;
		alarm_player = memnew(AudioStreamPlayer3D);
		alarm_player->set_stream(sound_bank.get(level.ambience == Ambience::Rooftop ? "siren" : "bell"));
		alarm_player->set_volume_db(4.0f);
		alarm_player->set_max_distance(140.0f);
		alarm_player->set_unit_size(14.0f);
		level_root->add_child(alarm_player);
		alarm_player->set_global_position(position + Vector3(0.0f, 2.0f, 0.0f));
		alarm_player->play();
		hud->show_center("ТРЕВОГА"_u, 2.0f);
	}
	notify(String("Сработка: "_u) + source, Color(1.0f, 0.35f, 0.3f));
	TypedArray<Node> guards = get_tree()->get_nodes_in_group("urbex_guards");
	for (int i = 0; i < guards.size(); i++) {
		if (Guard *g = Object::cast_to<Guard>(guards[i])) {
			g->alert(position, true);
		}
	}
}

void UrbexGame::on_guard_spotted(Guard *guard, const Vector3 &where) {
	stats.spotted++;
	stats.chases++;
	notify(String("Тебя спалил "_u) + guard->get_display_name() + "!", Color(1.0f, 0.35f, 0.3f));
	TypedArray<Node> guards = get_tree()->get_nodes_in_group("urbex_guards");
	for (int i = 0; i < guards.size(); i++) {
		Guard *g = Object::cast_to<Guard>(guards[i]);
		if (g && g != guard && (g->get_global_position() - where).length() < 45.0f) {
			g->alert(where, true);
		}
	}
}

void UrbexGame::on_guard_lost(Guard *) {
	notify("Оторвался. Охрана ищет поблизости"_u, Color(0.9f, 0.85f, 0.6f));
}

void UrbexGame::player_caught(const String &who) {
	if (end_timer >= 0.0f) {
		return;
	}
	say(player, who, "Попался. Сиди тут, полиция уже едет."_u);
	finish(OUTCOME_CAUGHT, who);
}

void UrbexGame::player_died(const String &reason) {
	if (end_timer >= 0.0f) {
		return;
	}
	player->set_dead(true);
	finish(OUTCOME_DIED, reason);
}

void UrbexGame::finish(Outcome outcome, const String &reason) {
	if (end_timer >= 0.0f) {
		return;
	}
	pending_outcome = outcome;
	pending_reason = reason;
	end_timer = 1.6f;
	player->set_controls_enabled(false);
	hud->set_viewfinder(false, String());
	if (outcome == OUTCOME_DIED) {
		play_ui_sound("land", 2.0f, 0.7f);
	}
}

void UrbexGame::show_results() {
	mode = MODE_RESULT;
	get_tree()->set_pause(true);
	set_mouse_captured(false);
	heartbeat->set_volume_db(-80.0f);
	if (alarm_player) {
		alarm_player->stop();
	}

	int extra_photos = std::max(0, std::min(10, stats.photos - stats.objective_photos));
	int score = 0;
	String title;
	Color color;
	String body;

	String stat_lines;
	stat_lines += String("Время на объекте: [b]"_u) + format_time(stats.time) + "[/b]\n";
	stat_lines += String("Кадров для отчёта: [b]"_u) + String::num_int64(stats.objective_photos) + "[/b], всего снимков: "_u + String::num_int64(stats.photos) + "\n";
	stat_lines += String("Артефактов: [b]"_u) + String::num_int64(stats.artifacts) + "[/b]\n";
	stat_lines += String("Сработок сигнализации: [b]"_u) + String::num_int64(stats.alarms) + "[/b]\n";
	stat_lines += String("Раз спалился: [b]"_u) + String::num_int64(stats.spotted) + "[/b]\n";
	if (stats.gbr_arrived) {
		stat_lines += "ГБР приезжала на объект\n"_u;
	}

	switch (pending_outcome) {
		case OUTCOME_ESCAPED: {
			score = stats.objective_photos * 150 + extra_photos * 15 + stats.artifacts * 250;
			if (stats.alarms == 0) {
				score += 400;
			}
			if (stats.spotted == 0) {
				score += 500;
			}
			score += std::max(0, int(900.0f - stats.time) / 3);
			title = "ВЫБРАЛСЯ"_u;
			color = Color(0.55f, 0.95f, 0.55f);
			body += "Ты ушёл с объекта. Фотки уже грузятся в паблик.\n\n"_u;
			body += stat_lines;
			body += String("\n[font_size=26][color=#ee8533]") + String::num_int64(score) + " лайков[/color][/font_size]\n"_u;
			if (stats.alarms == 0 && stats.spotted == 0) {
				body += "[color=#8fd18f]Чистый заход: ни одной сработки, никто не видел.[/color]\n"_u;
			}
			save_result(score);
		} break;
		case OUTCOME_CAUGHT:
			title = "ЗАДЕРЖАН"_u;
			color = Color(1.0f, 0.45f, 0.35f);
			body += pending_reason + " держит тебя до приезда наряда полиции.\n\n"_u;
			body += "По ч. 1 ст. 20.17 КоАП РФ самовольное проникновение на охраняемый объект — штраф от 3 до 5 тысяч рублей или обязательные работы. Охранник ЧОП может задержать нарушителя и обязан сразу передать его полиции, протокол составляет уже полиция.\n\n"_u;
			body += stat_lines;
			break;
		case OUTCOME_DIED:
			title = "НЕ ВЕРНУЛСЯ"_u;
			color = Color(0.85f, 0.85f, 0.85f);
			body += pending_reason + "\n\n"_u;
			body += "В реальности сталкеры чаще всего гибнут не от охраны, а от падений: дыры в перекрытиях, шахты лифтов, край крыши, гнилые лестницы.\n\n"_u;
			body += stat_lines;
			break;
	}
	if (!level.legal_note.is_empty()) {
		body += String("\n[color=#a0a49f]") + level.legal_note + "[/color]";
	}
	hud->show_result(title, color, body);
}

void UrbexGame::notify(const String &text, const Color &color) {
	if (hud) {
		hud->push_message(text, color);
	}
}

void UrbexGame::say(Node3D *speaker, const String &who, const String &line) {
	if (!hud || !player || !speaker) {
		return;
	}
	if ((speaker->get_global_position() - player->get_global_position()).length() < 40.0f) {
		hud->show_subtitle(who, line);
	}
}

void UrbexGame::show_note(const String &title, const String &body) {
	hud->show_note(title, body);
	if (player) {
		player->set_controls_enabled(false);
	}
}

String UrbexGame::photo_hint(Camera3D *camera) const {
	if (!camera || !level_root) {
		return String();
	}
	Vector3 fwd = -camera->get_global_transform().basis.get_column(2);
	TypedArray<Node> spots = const_cast<UrbexGame *>(this)->get_tree()->get_nodes_in_group("urbex_photo_spots");
	for (int i = 0; i < spots.size(); i++) {
		PhotoSpot *s = Object::cast_to<PhotoSpot>(spots[i]);
		if (s && !s->is_captured() && s->in_frame(camera, fwd)) {
			return String("[ЛКМ] В кадре: "_u) + s->get_title();
		}
	}
	return "[ЛКМ] Снимок"_u;
}

void UrbexGame::on_photo(Camera3D *camera) {
	hud->trigger_flash();
	stats.photos++;
	Vector3 fwd = -camera->get_global_transform().basis.get_column(2);
	TypedArray<Node> spots = get_tree()->get_nodes_in_group("urbex_photo_spots");
	bool any = false;
	for (int i = 0; i < spots.size(); i++) {
		PhotoSpot *s = Object::cast_to<PhotoSpot>(spots[i]);
		if (!s || s->is_captured() || !s->in_frame(camera, fwd)) {
			continue;
		}
		s->set_captured(true);
		any = true;
		for (int j = 0; j < spots.size(); j++) {
			PhotoSpot *other = Object::cast_to<PhotoSpot>(spots[j]);
			if (other && other->get_spot_id() == s->get_spot_id()) {
				other->set_captured(true);
			}
		}
		bool objective = false;
		for (Objective &o : level.objectives) {
			if (o.type == ObjectiveType::Photo && o.id == s->get_spot_id() && !o.done) {
				o.done = true;
				objective = true;
			}
		}
		if (objective) {
			stats.objective_photos++;
		}
		notify(String("Кадр в отчёт: "_u) + s->get_title(), Color(0.6f, 0.95f, 0.6f));
	}
	if (!any && stats.photos % 3 == 1) {
		notify("Снимок на память. Для отчёта ищи интересные места"_u, Color(0.7f, 0.7f, 0.7f));
	}
}

void UrbexGame::on_item(const String &id, const String &name, bool artifact) {
	if (artifact) {
		stats.artifacts++;
		notify(String("Артефакт: "_u) + name, Color(0.95f, 0.8f, 0.45f));
	} else {
		notify(String("Взято: "_u) + name, Color(0.8f, 0.9f, 0.8f));
	}
	complete_objective(id);
}

void UrbexGame::complete_objective(const String &id) {
	for (Objective &o : level.objectives) {
		if (!o.done && o.id == id && (o.type == ObjectiveType::Item || o.type == ObjectiveType::Action)) {
			o.done = true;
			notify(String("Выполнено: "_u) + o.title, Color(0.6f, 0.95f, 0.6f));
		}
	}
}

bool UrbexGame::is_powered(const String &group) const {
	if (group.is_empty()) {
		return true;
	}
	return std::find(unpowered_groups.begin(), unpowered_groups.end(), group) == unpowered_groups.end();
}

void UrbexGame::register_light(Light3D *light, const String &group) {
	powered_lights.push_back({ light, group, float(light->get_param(Light3D::PARAM_ENERGY)) });
	light->set_visible(is_powered(group));
}

void UrbexGame::set_power(const String &group, bool on, const Vector3 &where) {
	auto it = std::find(unpowered_groups.begin(), unpowered_groups.end(), group);
	if (on && it != unpowered_groups.end()) {
		unpowered_groups.erase(it);
	} else if (!on && it == unpowered_groups.end()) {
		unpowered_groups.push_back(group);
	}
	TypedArray<Node> devices = get_tree()->get_nodes_in_group("urbex_devices");
	for (int i = 0; i < devices.size(); i++) {
		SecurityDevice *d = Object::cast_to<SecurityDevice>(devices[i]);
		if (d && d->get_power_group() == group) {
			d->set_powered(on);
		}
	}
	TypedArray<Node> doors = get_tree()->get_nodes_in_group("urbex_doors");
	for (int i = 0; i < doors.size(); i++) {
		Door *d = Object::cast_to<Door>(doors[i]);
		if (d && d->get_power_group() == group) {
			d->set_powered(on);
		}
	}
	for (PoweredLight &pl : powered_lights) {
		if (pl.group == group) {
			pl.light->set_visible(on);
		}
	}
	notify(on ? "Питание включено"_u : "Обесточено: свет и датчики на этой линии погасли"_u, Color(0.8f, 0.85f, 1.0f));
	if (!on) {
		Guard *closest = nullptr;
		float best = 1e9f;
		TypedArray<Node> guards = get_tree()->get_nodes_in_group("urbex_guards");
		for (int i = 0; i < guards.size(); i++) {
			Guard *g = Object::cast_to<Guard>(guards[i]);
			if (!g || g->get_kind() == Guard::KIND_CONCIERGE || g->is_chasing()) {
				continue;
			}
			float d = (g->get_global_position() - where).length();
			if (d < best) {
				best = d;
				closest = g;
			}
		}
		if (closest && best < 120.0f) {
			say(closest, closest->get_display_name(), "Опять пробки выбило... Пойду гляну щиток."_u);
			closest->alert(where, false);
		}
	}
}

void UrbexGame::play_sound(const String &name, const Vector3 &position, float volume_db, float pitch, float max_distance) {
	if (!level_root) {
		play_ui_sound(name, volume_db, pitch);
		return;
	}
	Ref<AudioStreamWAV> stream = sound_bank.get(std::string(name.utf8().get_data()));
	if (stream.is_null()) {
		return;
	}
	AudioStreamPlayer3D *p = memnew(AudioStreamPlayer3D);
	p->set_stream(stream);
	p->set_volume_db(volume_db);
	p->set_pitch_scale(pitch);
	p->set_max_distance(max_distance);
	p->set_unit_size(6.0f);
	level_root->add_child(p);
	p->set_global_position(position);
	p->connect("finished", callable_mp(static_cast<Node *>(p), &Node::queue_free));
	p->play();
}

void UrbexGame::play_ui_sound(const String &name, float volume_db, float pitch) {
	Ref<AudioStreamWAV> stream = sound_bank.get(std::string(name.utf8().get_data()));
	if (stream.is_null()) {
		return;
	}
	AudioStreamPlayer *p = memnew(AudioStreamPlayer);
	p->set_stream(stream);
	p->set_volume_db(volume_db);
	p->set_pitch_scale(pitch);
	add_child(p);
	p->connect("finished", callable_mp(static_cast<Node *>(p), &Node::queue_free));
	p->play();
}

float UrbexGame::now() const {
	return stats.time;
}

} // namespace urbex
