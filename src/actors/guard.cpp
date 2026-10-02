#include "actors/guard.h"

#include "actors/player.h"
#include "core/common.h"
#include "core/game.h"
#include "core/materials.h"
#include "world/door.h"

#include <godot_cpp/classes/capsule_shape3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/navigation_server3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cmath>

using namespace godot;

namespace urbex {

namespace {

const char *const LINES_SUSPICIOUS[] = {
	"Хм... Кто тут?",
	"Эй! Есть кто?",
	"Что за...",
	"Мне не показалось?",
};
const char *const LINES_INVESTIGATE[] = {
	"Пойду гляну.",
	"Опять эти сталкеры лазят...",
	"Кто там шарится?",
	"Так, проверим.",
};
const char *const LINES_CHASE[] = {
	"Стоять! Охрана!",
	"Стой, кому говорю!",
	"Ну всё, попался!",
	"Стоять на месте!",
};
const char *const LINES_LOST[] = {
	"Ушёл, гад...",
	"Где он? Только что тут был.",
	"Ладно, далеко не убежит.",
};
const char *const LINES_CALM[] = {
	"Показалось.",
	"Кошки, наверное.",
	"Ветер, что ли...",
	"Тихо. Пойду дальше.",
};
const char *const LINES_ALARM[] = {
	"Пульт, принял, иду на сработку.",
	"Сработка! Проверяю.",
	"Понял, выдвигаюсь.",
};
const char *const LINES_GBR[] = {
	"ГБР на объекте, начинаем осмотр.",
	"Выходи, хуже будет!",
	"Проверяем каждый угол.",
};
const char *const LINES_CONCIERGE_SUS[] = {
	"Молодой человек, вы к кому?!",
	"Кто там ходит?",
};
const char *const LINES_CONCIERGE_CALL[] = {
	"Алло, охрана? У нас посторонний в подъезде!",
};

}

void Guard::setup(Kind p_kind, const String &name, const MaterialLibrary &materials, bool with_flashlight) {
	kind = p_kind;
	display_name = name;
	add_to_group("urbex_guards");
	set_collision_layer(layer::GUARDS);
	set_collision_mask(layer::WORLD | layer::DOORS | layer::PLAYER);
	set_floor_max_angle(50.0f * DEG);
	set_floor_snap_length(0.4f);

	Ref<CapsuleShape3D> capsule;
	capsule.instantiate();
	capsule->set_radius(0.33f);
	capsule->set_height(1.8f);
	CollisionShape3D *cs = memnew(CollisionShape3D);
	cs->set_shape(capsule);
	cs->set_position(Vector3(0.0f, 0.9f, 0.0f));
	add_child(cs);

	visual = memnew(Node3D);
	add_child(visual);

	HumanoidLook look;
	look.flashlight = with_flashlight;
	switch (kind) {
		case KIND_CHOP:
		case KIND_WATCHMAN:
			look.outfit = HumanoidLook::OUTFIT_GUARD;
			look.back_text = "ОХРАНА"_u;
			break;
		case KIND_GBR:
			look.outfit = HumanoidLook::OUTFIT_GBR;
			look.torso = "uniform";
			look.back_text = "ГБР"_u;
			look.text_color = Color(0.95f, 0.95f, 0.95f);
			break;
		case KIND_CONCIERGE:
			look.outfit = HumanoidLook::OUTFIT_CONCIERGE;
			look.torso = "cardigan";
			look.legs = "pants";
			look.cap = false;
			look.hair = true;
			look.scale = 0.94f;
			break;
	}
	rig.build(visual, materials, look);

	look_pivot = memnew(Node3D);
	look_pivot->set_position(Vector3(0.0f, 1.45f, 0.0f));
	visual->add_child(look_pivot);

	if (with_flashlight) {
		flashlight = memnew(SpotLight3D);
		flashlight->set_color(Color(1.0f, 0.9f, 0.75f));
		flashlight->set_param(Light3D::PARAM_RANGE, 18.0f);
		flashlight->set_param(Light3D::PARAM_ENERGY, 2.6f);
		flashlight->set_param(Light3D::PARAM_SPOT_ANGLE, 23.0f);
		flashlight->set_param(Light3D::PARAM_SPOT_ATTENUATION, 0.9f);
		flashlight->set_param(Light3D::PARAM_VOLUMETRIC_FOG_ENERGY, 0.8f);
		flashlight->set_shadow(true);
		flashlight->set_position(Vector3(0.28f, -0.15f, -0.25f));
		look_pivot->add_child(flashlight);
	}

	indicator = memnew(Label3D);
	indicator->set_font_size(96);
	indicator->set_pixel_size(0.004f);
	indicator->set_outline_size(16);
	indicator->set_billboard_mode(BaseMaterial3D::BILLBOARD_ENABLED);
	indicator->set_draw_flag(Label3D::FLAG_DISABLE_DEPTH_TEST, true);
	indicator->set_position(Vector3(0.0f, 2.25f, 0.0f));
	indicator->set_visible(false);
	add_child(indicator);

	agent = memnew(NavigationAgent3D);
	agent->set_path_desired_distance(0.6f);
	agent->set_target_desired_distance(0.8f);
	agent->set_radius(0.35f);
	agent->set_height(1.8f);
	agent->set_avoidance_enabled(false);
	add_child(agent);

	switch (kind) {
		case KIND_CHOP:
			set_speeds(1.5f, 4.4f);
			set_vision(16.0f, 110.0f);
			break;
		case KIND_WATCHMAN:
			set_speeds(1.4f, 4.2f);
			set_vision(15.0f, 100.0f);
			break;
		case KIND_GBR:
			set_speeds(1.9f, 5.0f);
			set_vision(20.0f, 115.0f);
			break;
		case KIND_CONCIERGE:
			set_speeds(0.0f, 0.0f);
			set_vision(9.0f, 100.0f);
			break;
	}
}

void Guard::add_route_point(const Vector3 &position, float wait) {
	route.push_back({ position, wait });
}

void Guard::set_post(const Vector3 &position, const std::vector<float> &yaws, float interval, bool sit) {
	stationary = true;
	post_position = position;
	post_yaws = yaws;
	post_interval = interval;
	sit_at_post = sit;
	if (!yaws.empty()) {
		yaw = yaws[0];
	}
}

void Guard::set_vision(float range, float fov_deg) {
	vision_range = range;
	half_fov = fov_deg * 0.5f * DEG;
}

void Guard::set_speeds(float walk, float run) {
	walk_speed = walk;
	run_speed = run;
}

void Guard::set_initial_yaw(float value) {
	yaw = value;
	if (visual) {
		visual->set_rotation(Vector3(0.0f, yaw, 0.0f));
	}
}

Vector3 Guard::forward() const {
	return yaw_forward(yaw + look_offset);
}

Vector3 Guard::eye_position() const {
	return get_global_position() + Vector3(0.0f, sitting ? 1.2f : 1.65f, 0.0f);
}

bool Guard::arrived(const Vector3 &target) const {
	Vector3 pos = get_global_position();
	return flat(target - pos).length() < 0.9f && std::fabs(target.y - pos.y) < 2.0f;
}

void Guard::bark(const char *const *lines, int count, bool radio) {
	if (bark_cooldown > 0.0f && !radio) {
		return;
	}
	bark_cooldown = 3.0f;
	UrbexGame *game = UrbexGame::get_singleton();
	const char *line = lines[(bark_index++ + int(get_instance_id() % 7)) % count];
	game->say(this, display_name, String::utf8(line));
	if (radio) {
		game->play_sound("radio", eye_position(), -4.0f);
	}
}

void Guard::set_state(State next) {
	State prev = state;
	state = next;
	state_timer = 0.0f;
	wait_timer = 0.0f;
	has_nav_target = false;
	stuck_timer = 0.0f;
	UrbexGame *game = UrbexGame::get_singleton();

	switch (next) {
		case STATE_CHASE:
			running = true;
			look_offset = 0.0f;
			lost_timer = 0.0f;
			chase_time = 0.0f;
			detection = 1.0f;
			if (prev != STATE_CHASE) {
				bark(LINES_CHASE, 4, true);
				game->on_guard_spotted(this, last_known);
			}
			break;
		case STATE_SEARCH:
			running = false;
			wait_timer = 3.0f;
			break;
		case STATE_PATROL:
		case STATE_RETURN: {
			running = false;
			state = STATE_PATROL;
			if (!route.empty()) {
				float best = 1e9f;
				Vector3 pos = get_global_position();
				for (size_t i = 0; i < route.size(); i++) {
					float d = (route[i].position - pos).length();
					if (d < best) {
						best = d;
						route_index = int(i);
					}
				}
			}
		} break;
		default:
			break;
	}
}

void Guard::hear(const Vector3 &position, float radius) {
	UrbexGame *game = UrbexGame::get_singleton();
	Vector3 ear = eye_position();
	float d = (position - ear).length();
	float effective = radius;
	if (!game->line_of_sight(ear, position + Vector3(0.0f, 0.4f, 0.0f), layer::WORLD | layer::DOORS)) {
		effective *= 0.55f;
	}
	if (std::fabs(position.y - get_global_position().y) > 2.6f) {
		effective *= 0.6f;
	}
	if (d > effective) {
		return;
	}
	if (kind == KIND_CONCIERGE) {
		if (state == STATE_PATROL) {
			suspicious_point = position;
			set_state(STATE_SUSPICIOUS);
			bark(LINES_CONCIERGE_SUS, 2);
		}
		return;
	}
	if (state == STATE_CHASE) {
		if (!sees_player) {
			last_known = position;
		}
		return;
	}
	bool loud = radius >= 9.0f;
	if (state == STATE_PATROL || state == STATE_RETURN) {
		if (loud) {
			investigate_target = position;
			set_state(STATE_INVESTIGATE);
			running = radius >= 13.0f;
			bark(LINES_INVESTIGATE, 4);
		} else {
			suspicious_point = position;
			set_state(STATE_SUSPICIOUS);
			bark(LINES_SUSPICIOUS, 4);
		}
	} else if (state == STATE_SUSPICIOUS) {
		suspicious_point = position;
		if (loud) {
			investigate_target = position;
			set_state(STATE_INVESTIGATE);
			running = true;
		} else {
			state_timer = 0.0f;
		}
	} else {
		investigate_target = position;
		bool was_running = running;
		set_state(STATE_INVESTIGATE);
		running = was_running || loud;
	}
}

void Guard::alert(const Vector3 &position, bool urgent) {
	if (kind == KIND_CONCIERGE || state == STATE_CHASE) {
		return;
	}
	investigate_target = random_search_point(position, 2.0f);
	set_state(STATE_INVESTIGATE);
	running = urgent;
	bark(kind == KIND_GBR ? LINES_GBR : LINES_ALARM, 3, true);
}

void Guard::start_chase_from_alarm(const Vector3 &position) {
	investigate_target = position;
	set_state(STATE_INVESTIGATE);
	running = true;
	bark(LINES_GBR, 3, true);
}

Vector3 Guard::random_search_point(const Vector3 &around, float radius) {
	Rng rng(get_instance_id() * 31 + Engine::get_singleton()->get_physics_frames());
	float ang = rng.range(0.0f, 2.0f * PI);
	float r = rng.range(radius * 0.3f, radius);
	Vector3 candidate = around + Vector3(std::cos(ang) * r, 0.0f, std::sin(ang) * r);
	if (!search_pool.empty() && rng.chance(0.35f)) {
		candidate = search_pool[rng.rangei(0, int(search_pool.size()) - 1)];
	}
	RID map = get_world_3d()->get_navigation_map();
	Vector3 snapped = NavigationServer3D::get_singleton()->map_get_closest_point(map, candidate);
	if ((snapped - candidate).length() > 4.0f) {
		return around;
	}
	return snapped;
}

void Guard::face(const Vector3 &point, float dt, float speed) {
	Vector3 d = flat(point - get_global_position());
	if (d.length() < 0.05f) {
		return;
	}
	yaw = approach_angle(yaw, yaw_towards(get_global_position(), point), speed * dt);
}

void Guard::stand_still(float dt) {
	Vector3 vel = get_velocity();
	vel.x = lerpf(vel.x, 0.0f, 1.0f - std::exp(-10.0f * dt));
	vel.z = lerpf(vel.z, 0.0f, 1.0f - std::exp(-10.0f * dt));
	if (!is_on_floor()) {
		vel.y -= 13.0f * dt;
	} else {
		vel.y = -0.5f;
	}
	set_velocity(vel);
	move_and_slide();
}

bool Guard::move_to(const Vector3 &target, float speed, float dt) {
	if (speed <= 0.0f) {
		stand_still(dt);
		return true;
	}
	if (!has_nav_target || (target - nav_target).length() > 0.5f) {
		agent->set_target_position(target);
		nav_target = target;
		has_nav_target = true;
		stuck_timer = 0.0f;
		stuck_origin = get_global_position();
	}
	Vector3 pos = get_global_position();
	Vector3 next = agent->get_next_path_position();
	bool finished = agent->is_navigation_finished();
	Vector3 desired;
	if (!finished) {
		Vector3 d = flat(next - pos);
		if (d.length() > 0.05f) {
			desired = d.normalized() * speed;
		}
	}
	Vector3 vel = get_velocity();
	float k = 1.0f - std::exp(-9.0f * dt);
	vel.x = lerpf(vel.x, desired.x, k);
	vel.z = lerpf(vel.z, desired.z, k);
	if (!is_on_floor()) {
		vel.y -= 13.0f * dt;
	} else {
		vel.y = -0.5f;
	}
	set_velocity(vel);
	move_and_slide();

	if (desired.length() > 0.1f) {
		yaw = approach_angle(yaw, std::atan2(-desired.x, -desired.z), 6.0f * dt);
		open_doors_ahead();
		if ((get_global_position() - stuck_origin).length() > 0.6f) {
			stuck_origin = get_global_position();
			stuck_timer = 0.0f;
		} else {
			stuck_timer += dt;
			if (stuck_timer > 2.5f) {
				stuck_timer = 0.0f;
				return true;
			}
		}
	}
	return finished || arrived(target);
}

void Guard::open_doors_ahead() {
	UrbexGame *game = UrbexGame::get_singleton();
	if (!doors_cached) {
		doors_cached = true;
		TypedArray<Node> nodes = get_tree()->get_nodes_in_group("urbex_doors");
		for (int i = 0; i < nodes.size(); i++) {
			if (Door *d = Object::cast_to<Door>(nodes[i])) {
				doors.push_back(d);
			}
		}
	}
	(void)game;
	Vector3 pos = get_global_position();
	Vector3 vel = flat(get_velocity());
	for (Door *door : doors) {
		if (door->is_open() || door->get_style() == Door::STYLE_HATCH) {
			continue;
		}
		Vector3 c = door->get_center();
		Vector3 to = flat(c - pos);
		float d = to.length();
		if (d < 1.9f && std::fabs(c.y - (pos.y + 1.0f)) < 1.6f && (vel.dot(to) > 0.0f || d < 1.1f)) {
			door->open_by_guard(pos);
		}
	}
}

void Guard::update_vision(float dt) {
	vision_tick -= dt;
	if (vision_tick > 0.0f) {
		return;
	}
	vision_tick = 0.1f;
	sees_player = false;
	UrbexGame *game = UrbexGame::get_singleton();
	UrbexPlayer *player = game->get_player();
	if (!player) {
		return;
	}
	Vector3 eye = eye_position();
	Vector3 target = player->get_chest_position();
	Vector3 to = target - eye;
	float dist = to.length();
	bool light_on = flashlight && flashlight->is_visible();
	float exposure = game->exposure_for(eye, forward(), light_on);
	float range = std::max(3.0f, vision_range * (0.3f + 0.7f * exposure));
	if (state == STATE_CHASE) {
		range = std::max(range, vision_range * 0.9f);
	}
	if (kind == KIND_CONCIERGE && post_index % 2 == 1) {
		range *= 0.45f;
	}
	if (dist > range || dist < 0.01f) {
		return;
	}
	Vector3 fwd = flat(forward()).normalized();
	Vector3 hdir = flat(to);
	float cosang = hdir.length() > 0.01f ? fwd.dot(hdir.normalized()) : 1.0f;
	float vertical = std::fabs(to.y) / std::max(0.1f, hdir.length());
	bool in_fov = (cosang > std::cos(half_fov) && vertical < 1.6f) || dist < 1.5f;
	if (!in_fov) {
		return;
	}
	bool los = game->line_of_sight(eye, target, layer::WORLD | layer::DOORS) || game->line_of_sight(eye, player->get_eye_position(), layer::WORLD | layer::DOORS);
	if (!los) {
		return;
	}
	sees_player = true;
	seen_distance = dist;
	seen_peripheral = cosang < std::cos(half_fov * 0.55f);
	seen_exposure = exposure;
	last_known = player->get_global_position();
	seen_range_value = range;
}

void Guard::update_detection(float dt) {
	UrbexGame *game = UrbexGame::get_singleton();
	UrbexPlayer *player = game->get_player();
	if (sees_player && player) {
		float range = std::max(0.5f, seen_range_value);
		float rate = (0.4f + 2.2f * (1.0f - seen_distance / range)) * (0.45f + seen_exposure);
		if (seen_peripheral) {
			rate *= 0.45f;
		}
		float speed = flat(player->get_real_velocity()).length();
		if (player->is_crouching() && speed < 0.2f) {
			rate *= 0.55f;
		}
		if (player->get_gait() == UrbexPlayer::GAIT_RUN) {
			rate *= 1.5f;
		}
		if (state == STATE_INVESTIGATE || state == STATE_SEARCH || state == STATE_SUSPICIOUS) {
			rate *= 1.4f;
		}
		if (game->is_alarm_active()) {
			rate *= 1.2f;
		}
		if (seen_distance < 2.0f) {
			rate += 3.0f;
		}
		detection += rate * dt;
	} else if (state != STATE_CHASE) {
		float decay = (state == STATE_SEARCH || state == STATE_INVESTIGATE) ? 0.12f : 0.22f;
		detection -= decay * dt;
	}
	detection = clampf(detection, 0.0f, 1.0f);
}

void Guard::update_state(float dt) {
	UrbexGame *game = UrbexGame::get_singleton();
	UrbexPlayer *player = game->get_player();
	Vector3 pos = get_global_position();
	state_timer += dt;
	look_clock += dt;

	if (kind == KIND_CONCIERGE) {
		stand_still(dt);
		if (state == STATE_PATROL) {
			look_offset = std::sin(look_clock * 0.4f) * 0.25f;
			post_timer += dt;
			if (post_timer > post_interval && !post_yaws.empty()) {
				post_timer = 0.0f;
				post_index = (post_index + 1) % int(post_yaws.size());
			}
			if (!post_yaws.empty()) {
				yaw = approach_angle(yaw, post_yaws[post_index], 2.0f * dt);
			}
			if (detection >= 0.3f) {
				suspicious_point = last_known;
				set_state(STATE_SUSPICIOUS);
				bark(LINES_CONCIERGE_SUS, 2);
			}
		} else {
			look_offset = 0.0f;
			post_index = 0;
			face(sees_player ? last_known : suspicious_point, dt, 3.0f);
			if (detection >= 1.0f && alarm_called_cooldown <= 0.0f) {
				alarm_called_cooldown = 40.0f;
				bark(LINES_CONCIERGE_CALL, 1, true);
				game->raise_alarm(last_known, "Консьержка вызвала охрану"_u);
			}
			if (sees_player) {
				state_timer = 0.0f;
			} else if (state_timer > 4.0f) {
				set_state(STATE_PATROL);
			}
		}
		return;
	}

	switch (state) {
		case STATE_PATROL: {
			if (detection >= 1.0f) {
				set_state(STATE_CHASE);
				break;
			}
			if (detection >= 0.35f) {
				suspicious_point = last_known;
				set_state(STATE_SUSPICIOUS);
				bark(LINES_SUSPICIOUS, 4);
				break;
			}
			if (stationary || route.empty()) {
				if (stationary && flat(post_position - pos).length() > 0.7f) {
					move_to(post_position, walk_speed, dt);
				} else {
					stand_still(dt);
					post_timer += dt;
					look_offset = std::sin(look_clock * 0.5f) * 0.5f;
					if (post_timer > post_interval && !post_yaws.empty()) {
						post_timer = 0.0f;
						post_index = (post_index + 1) % int(post_yaws.size());
					}
					if (!post_yaws.empty()) {
						yaw = approach_angle(yaw, post_yaws[post_index], 1.5f * dt);
					}
				}
			} else {
				RoutePoint &rp = route[route_index];
				if (wait_timer > 0.0f) {
					wait_timer -= dt;
					stand_still(dt);
					look_offset = std::sin(look_clock * 0.9f) * 0.9f;
					if (wait_timer <= 0.0f) {
						route_index = (route_index + 1) % int(route.size());
						has_nav_target = false;
					}
				} else {
					look_offset = lerpf(look_offset, std::sin(look_clock * 0.6f) * 0.35f, 1.0f - std::exp(-2.0f * dt));
					if (move_to(rp.position, walk_speed, dt)) {
						wait_timer = std::max(0.01f, rp.wait);
						has_nav_target = false;
					}
				}
			}
		} break;

		case STATE_SUSPICIOUS: {
			stand_still(dt);
			look_offset = lerpf(look_offset, 0.0f, 1.0f - std::exp(-5.0f * dt));
			if (sees_player) {
				suspicious_point = last_known;
			}
			face(suspicious_point, dt, 3.0f);
			if (detection >= 1.0f) {
				set_state(STATE_CHASE);
				break;
			}
			if (state_timer > 2.8f) {
				investigate_target = suspicious_point;
				set_state(STATE_INVESTIGATE);
				running = false;
				bark(LINES_INVESTIGATE, 4);
			}
		} break;

		case STATE_INVESTIGATE: {
			if (detection >= 1.0f) {
				set_state(STATE_CHASE);
				break;
			}
			if (sees_player && detection >= 0.35f) {
				investigate_target = last_known;
			}
			look_offset = lerpf(look_offset, std::sin(look_clock * 1.3f) * 0.4f, 1.0f - std::exp(-3.0f * dt));
			if (move_to(investigate_target, running ? run_speed : walk_speed * 1.15f, dt)) {
				search_left = kind == KIND_GBR ? 3 : 2;
				set_state(STATE_SEARCH);
			}
		} break;

		case STATE_SEARCH: {
			if (detection >= 1.0f) {
				set_state(STATE_CHASE);
				break;
			}
			if (wait_timer > 0.0f) {
				wait_timer -= dt;
				stand_still(dt);
				look_offset = std::sin(look_clock * 1.5f) * 1.2f;
				if (sees_player && detection > 0.3f) {
					face(last_known, dt, 3.0f);
				}
				if (wait_timer <= 0.0f) {
					if (search_left > 0) {
						search_left--;
						investigate_target = random_search_point(investigate_target, 7.0f);
						has_nav_target = false;
					} else {
						bark(LINES_CALM, 4);
						set_state(STATE_RETURN);
					}
				}
			} else {
				look_offset = lerpf(look_offset, 0.0f, 1.0f - std::exp(-3.0f * dt));
				if (move_to(investigate_target, walk_speed * 1.2f, dt)) {
					wait_timer = 2.5f;
				}
			}
		} break;

		case STATE_CHASE: {
			chase_time += dt;
			Vector3 target = last_known;
			if (sees_player) {
				lost_timer = 0.0f;
			} else {
				lost_timer += dt;
			}
			bool reached = move_to(target, run_speed, dt);
			if (player && sees_player) {
				Vector3 pp = player->get_global_position();
				if (flat(pp - pos).length() < 1.35f && std::fabs(pp.y - pos.y) < 1.4f) {
					game->player_caught(display_name);
					return;
				}
			}
			if (chase_time > 7.0f && !reported_chase && kind != KIND_GBR) {
				reported_chase = true;
				game->raise_alarm(last_known, display_name + " вызвал ГБР по рации"_u);
			}
			if (lost_timer > 5.0f || (reached && !sees_player && lost_timer > 1.5f)) {
				investigate_target = last_known;
				search_left = 3;
				bark(LINES_LOST, 3, true);
				game->on_guard_lost(this);
				set_state(STATE_SEARCH);
			}
		} break;

		case STATE_RETURN:
			set_state(STATE_PATROL);
			break;
	}
}

void Guard::_physics_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !agent) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game || !game->is_playing()) {
		return;
	}
	float dt = float(delta);
	bark_cooldown = std::max(0.0f, bark_cooldown - dt);
	alarm_called_cooldown = std::max(0.0f, alarm_called_cooldown - dt);
	update_vision(dt);
	update_detection(dt);
	update_state(dt);
}

void Guard::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !visual) {
		return;
	}
	float dt = float(delta);
	visual->set_rotation(Vector3(0.0f, yaw, 0.0f));
	float pitch = -0.18f;
	UrbexGame *game = UrbexGame::get_singleton();
	if (state == STATE_CHASE && sees_player && game && game->get_player()) {
		Vector3 to = game->get_player()->get_chest_position() - look_pivot->get_global_position();
		pitch = std::atan2(to.y, flat(to).length());
	}
	look_pivot->set_rotation(Vector3(pitch, look_offset, 0.0f));
	rig.look(look_offset * 0.8f, pitch * 0.5f);
	float speed = flat(get_real_velocity()).length();
	bool should_sit = sit_at_post && state == STATE_PATROL && flat(post_position - get_global_position()).length() < 0.7f && speed < 0.3f;
	if (should_sit != sitting) {
		sitting = should_sit;
		rig.sit(sitting);
	}
	anim_phase += speed * dt * 2.4f;
	if (!sitting) {
		rig.animate(anim_phase, clampf(speed / 3.0f, 0.0f, 1.0f), flashlight ? 1.1f : 0.0f);
	}

	if (state == STATE_CHASE) {
		indicator->set_visible(true);
		indicator->set_text("!");
		indicator->set_modulate(Color(1.0f, 0.15f, 0.1f));
	} else if (detection > 0.05f || state == STATE_SUSPICIOUS || state == STATE_INVESTIGATE || state == STATE_SEARCH) {
		indicator->set_visible(true);
		indicator->set_text("?");
		float k = std::max(detection, 0.4f);
		indicator->set_modulate(Color(1.0f, 1.0f - k * 0.6f, 0.2f));
	} else {
		indicator->set_visible(false);
	}
}

}
