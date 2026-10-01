#include "actors/player.h"

#include "core/common.h"
#include "core/effects.h"
#include "core/game.h"
#include "core/materials.h"
#include "world/interactable.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/capsule_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <cmath>

using namespace godot;

namespace urbex {

namespace {
constexpr float STAND_HEIGHT = 1.75f;
constexpr float CROUCH_HEIGHT = 1.0f;
constexpr float RADIUS = 0.32f;
constexpr float GRAVITY = 13.0f;
}

void UrbexPlayer::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	build();
}

void UrbexPlayer::build() {
	if (head) {
		return;
	}
	set_collision_layer(layer::PLAYER);
	set_collision_mask(layer::WORLD | layer::DOORS | layer::GUARDS);
	set_floor_max_angle(48.0f * DEG);
	set_floor_snap_length(0.35f);

	capsule.instantiate();
	capsule->set_radius(RADIUS);
	capsule->set_height(STAND_HEIGHT);
	collider = memnew(CollisionShape3D);
	collider->set_shape(capsule);
	collider->set_position(Vector3(0.0f, STAND_HEIGHT * 0.5f, 0.0f));
	add_child(collider);

	head = memnew(Node3D);
	head->set_position(Vector3(0.0f, 1.6f, 0.0f));
	add_child(head);

	camera = memnew(Camera3D);
	camera->set_fov(75.0f);
	camera->set_near(0.05f);
	camera->set_far(1500.0f);
	head->add_child(camera);
	camera->set_current(true);

	flashlight = memnew(SpotLight3D);
	flashlight->set_color(Color(1.0f, 0.94f, 0.82f));
	flashlight->set_param(Light3D::PARAM_RANGE, 26.0f);
	flashlight->set_param(Light3D::PARAM_ENERGY, 3.4f);
	flashlight->set_param(Light3D::PARAM_SPOT_ANGLE, 21.0f);
	flashlight->set_param(Light3D::PARAM_SPOT_ATTENUATION, 0.65f);
	flashlight->set_param(Light3D::PARAM_VOLUMETRIC_FOG_ENERGY, 0.6f);
	flashlight->set_shadow(true);
	flashlight->set_position(Vector3(-0.16f, -0.14f, -0.32f));
	flashlight->set_visible(false);
	camera->add_child(flashlight);

	flashlight_spill = memnew(SpotLight3D);
	flashlight_spill->set_color(Color(1.0f, 0.92f, 0.8f));
	flashlight_spill->set_param(Light3D::PARAM_RANGE, 14.0f);
	flashlight_spill->set_param(Light3D::PARAM_ENERGY, 0.7f);
	flashlight_spill->set_param(Light3D::PARAM_SPOT_ANGLE, 44.0f);
	flashlight_spill->set_param(Light3D::PARAM_SPOT_ATTENUATION, 1.6f);
	flashlight_spill->set_param(Light3D::PARAM_SPECULAR, 0.2f);
	flashlight->add_child(flashlight_spill);
	build_viewmodel();
}

void UrbexPlayer::build_viewmodel() {
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game) {
		return;
	}
	const MaterialLibrary &m = game->get_materials();
	viewmodel = memnew(Node3D);
	camera->add_child(viewmodel);
	int priority = 1;
	auto part = [&](Node3D *parent, const Ref<Mesh> &mesh, const Vector3 &pos, const Vector3 &rot, const char *mat) -> Ref<StandardMaterial3D> {
		Ref<StandardMaterial3D> base = m.get(mat);
		Ref<StandardMaterial3D> vm = base->duplicate();
		vm->set_flag(BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
		vm->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		vm->set_render_priority(priority++);
		MeshInstance3D *mi = memnew(MeshInstance3D);
		mi->set_mesh(mesh);
		mi->set_material_override(vm);
		mi->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
		mi->set_transform(Transform3D(Basis::from_euler(rot), pos));
		parent->add_child(mi);
		return vm;
	};
	auto capsule = [](float r, float h) {
		Ref<CapsuleMesh> c;
		c.instantiate();
		c->set_radius(r);
		c->set_height(h);
		c->set_radial_segments(10);
		c->set_rings(3);
		return c;
	};
	auto box = [](const Vector3 &size) {
		Ref<BoxMesh> b;
		b.instantiate();
		b->set_size(size);
		return b;
	};
	auto cyl = [](float r, float h) {
		Ref<CylinderMesh> c;
		c.instantiate();
		c->set_top_radius(r);
		c->set_bottom_radius(r);
		c->set_height(h);
		c->set_radial_segments(12);
		return c;
	};

	phone_arm = memnew(Node3D);
	phone_arm->set_position(Vector3(0.2f, -0.24f, -0.34f));
	viewmodel->add_child(phone_arm);
	part(phone_arm, capsule(0.042f, 0.34f), Vector3(0.06f, -0.08f, 0.13f), Vector3(1.15f, 0.35f, 0.0f), "jacket_resident");
	part(phone_arm, capsule(0.04f, 0.12f), Vector3(0.0f, -0.01f, 0.0f), Vector3(0.2f, 0.0f, 0.15f), "rubber");
	part(phone_arm, box(Vector3(0.075f, 0.15f, 0.01f)), Vector3(-0.02f, 0.03f, -0.025f), Vector3(-0.25f, -0.15f, 0.0f), "black");
	Ref<StandardMaterial3D> screen = part(phone_arm, box(Vector3(0.066f, 0.134f, 0.002f)), Vector3(-0.02f, 0.031f, -0.019f), Vector3(-0.25f, -0.15f, 0.0f), "phone_screen");
	screen->set_albedo(Color(0.08f, 0.1f, 0.16f));
	screen->set_emission(Color(0.25f, 0.35f, 0.55f));
	screen->set_emission_energy_multiplier(0.6f);

	torch_arm = memnew(Node3D);
	torch_arm->set_position(Vector3(-0.19f, -0.6f, -0.38f));
	viewmodel->add_child(torch_arm);
	part(torch_arm, capsule(0.042f, 0.36f), Vector3(-0.05f, -0.08f, 0.15f), Vector3(1.2f, -0.3f, 0.0f), "jacket_resident");
	part(torch_arm, capsule(0.04f, 0.12f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -0.2f), "rubber");
	part(torch_arm, cyl(0.022f, 0.19f), Vector3(0.02f, 0.03f, -0.07f), Vector3(PI * 0.5f, 0.0f, 0.0f), "black");
	part(torch_arm, cyl(0.03f, 0.04f), Vector3(0.02f, 0.03f, -0.17f), Vector3(PI * 0.5f, 0.0f, 0.0f), "black");
	part(torch_arm, cyl(0.026f, 0.005f), Vector3(0.02f, 0.03f, -0.19f), Vector3(PI * 0.5f, 0.0f, 0.0f), "lamp_cold");
	torch_arm->set_visible(false);
}

void UrbexPlayer::place(const Vector3 &position, float p_yaw) {
	build();
	set_global_position(position);
	yaw = p_yaw;
	pitch = 0.0f;
	set_velocity(Vector3());
	set_rotation(Vector3(0.0f, yaw, 0.0f));
	head->set_rotation(Vector3());
	was_on_floor = true;
	last_vertical_speed = 0.0f;
}

void UrbexPlayer::set_look(float p_yaw, float p_pitch) {
	build();
	yaw = p_yaw;
	pitch = p_pitch;
	set_rotation(Vector3(0.0f, yaw, 0.0f));
	head->set_rotation(Vector3(pitch, 0.0f, 0.0f));
}

void UrbexPlayer::set_flashlight(bool on) {
	flashlight_on = on;
	if (flashlight) {
		flashlight->set_visible(on && battery > 0.0f);
	}
}

void UrbexPlayer::set_controls_enabled(bool enabled) {
	controls_enabled = enabled;
	if (!enabled) {
		aiming = false;
	}
}

float UrbexPlayer::get_height() const {
	return lerpf(STAND_HEIGHT, CROUCH_HEIGHT, crouch_blend);
}

Vector3 UrbexPlayer::get_eye_position() const {
	return camera ? camera->get_global_position() : get_global_position() + Vector3(0, 1.6f, 0);
}

Vector3 UrbexPlayer::get_chest_position() const {
	return get_global_position() + Vector3(0.0f, get_height() * 0.72f, 0.0f);
}

Vector3 UrbexPlayer::get_look_direction() const {
	return camera ? -camera->get_global_transform().basis.get_column(2).normalized() : yaw_forward(yaw);
}

String UrbexPlayer::get_gait_name() const {
	switch (gait) {
		case GAIT_IDLE:
			return crouching ? "Присел"_u : "Стоишь"_u;
		case GAIT_SNEAK:
			return crouching ? "Ползком"_u : "Крадёшься"_u;
		case GAIT_CROUCH:
			return "Гусиный шаг"_u;
		case GAIT_WALK:
			return "Шаг"_u;
		case GAIT_RUN:
			return "Бег"_u;
	}
	return String();
}

bool UrbexPlayer::has_item(const String &id) const {
	return inventory.has(id);
}

void UrbexPlayer::give_item(const String &id, const String &name) {
	inventory[id] = name;
}

void UrbexPlayer::remove_item(const String &id) {
	inventory.erase(id);
}

void UrbexPlayer::make_noise(float radius) {
	noise_radius = std::max(noise_radius, radius);
	UrbexGame::get_singleton()->emit_noise(get_global_position(), radius, true);
}

void UrbexPlayer::_unhandled_input(const Ref<InputEvent> &event) {
	if (!controls_enabled || dead || !head) {
		return;
	}
	Ref<InputEventMouseMotion> motion = event;
	if (motion.is_valid() && Input::get_singleton()->get_mouse_mode() == Input::MOUSE_MODE_CAPTURED) {
		UrbexGame *game = UrbexGame::get_singleton();
		float sens = mouse_sensitivity * (game ? game->get_sensitivity() : 1.0f) * (aiming ? 0.55f : 1.0f);
		Vector2 rel = motion->get_relative();
		sway_target += Vector2(rel.x, rel.y) * 0.00035f;
		sway_target = Vector2(clampf(sway_target.x, -0.04f, 0.04f), clampf(sway_target.y, -0.04f, 0.04f));
		yaw = wrap_angle(yaw - rel.x * sens);
		pitch = clampf(pitch - rel.y * sens, -1.5f, 1.45f);
		set_rotation(Vector3(0.0f, yaw, 0.0f));
		head->set_rotation(Vector3(pitch, 0.0f, 0.0f));
	}
}

bool UrbexPlayer::can_stand() const {
	PhysicsDirectSpaceState3D *space = get_world_3d()->get_direct_space_state();
	Vector3 base = get_global_position();
	TypedArray<RID> exclude;
	exclude.push_back(get_rid());
	for (float ox : { -0.2f, 0.2f }) {
		for (float oz : { -0.2f, 0.2f }) {
			Ref<PhysicsRayQueryParameters3D> q = PhysicsRayQueryParameters3D::create(base + Vector3(ox, 0.6f, oz), base + Vector3(ox, STAND_HEIGHT + 0.05f, oz), layer::WORLD | layer::DOORS, exclude);
			if (!space->intersect_ray(q).is_empty()) {
				return false;
			}
		}
	}
	return true;
}

void UrbexPlayer::update_crouch(float dt) {
	Input *in = Input::get_singleton();
	bool active = controls_enabled && !dead;
	if (active && in->is_action_just_pressed("crouch")) {
		crouch_toggled = !crouch_toggled;
	}
	bool want = crouch_toggled || (active && in->is_action_pressed("crouch_hold"));
	if (!want && crouching && !can_stand()) {
		want = true;
	}
	crouching = want;
	crouch_blend = approach(crouch_blend, crouching ? 1.0f : 0.0f, dt * 5.0f);
	float h = get_height();
	capsule->set_height(h);
	collider->set_position(Vector3(0.0f, h * 0.5f, 0.0f));
}

void UrbexPlayer::update_flashlight(float dt) {
	if (flashlight_on && battery > 0.0f) {
		battery = std::max(0.0f, battery - dt / 420.0f);
		float energy = 3.4f;
		if (battery < 0.15f) {
			flicker_timer -= dt;
			if (flicker_timer <= 0.0f) {
				flicker_timer = 0.05f + float(Engine::get_singleton()->get_physics_frames() % 13) * 0.02f;
			}
			energy = (Engine::get_singleton()->get_physics_frames() % 9 < 2) ? 0.4f : 3.4f * (0.4f + battery * 4.0f);
		}
		flashlight->set_param(Light3D::PARAM_ENERGY, energy);
		flashlight->set_visible(true);
		flashlight_spill->set_param(Light3D::PARAM_ENERGY, energy * 0.2f);
		if (battery <= 0.0f) {
			flashlight_on = false;
			UrbexGame::get_singleton()->notify("Фонарик сел. Ищи батарейки"_u, Color(1.0f, 0.7f, 0.4f));
		}
	} else {
		flashlight->set_visible(false);
	}
}

void UrbexPlayer::emit_step(float radius) {
	UrbexGame *game = UrbexGame::get_singleton();
	Vector3 feet = get_global_position();
	bool noisy = game->is_noisy_floor(feet);
	if (noisy) {
		radius *= 2.0f;
	}
	float volume = -6.0f;
	switch (gait) {
		case GAIT_RUN:
			volume = 0.0f;
			break;
		case GAIT_WALK:
			volume = -7.0f;
			break;
		case GAIT_CROUCH:
			volume = -15.0f;
			break;
		case GAIT_SNEAK:
			volume = -21.0f;
			break;
		default:
			break;
	}
	if (noisy) {
		volume += 5.0f;
	}
	step_index++;
	float pitch_shift = 0.92f + float((step_index * 7) % 5) * 0.04f;
	String name = String(noisy ? "step_glass_" : "step_") + String::num_int64(step_index % 4);
	game->play_sound(name, feet, volume, pitch_shift, 30.0f);
	if (gait == GAIT_RUN && game->get_level_root()) {
		fx::dust_puff(game->get_level_root(), game->get_materials(), feet + Vector3(0.0f, 0.08f, 0.0f), 0.45f);
	}
	noise_radius = std::max(noise_radius, radius);
	game->emit_noise(feet, radius, true);
}

void UrbexPlayer::take_photo() {
	UrbexGame *game = UrbexGame::get_singleton();
	photos_taken++;
	photo_cooldown = 0.7f;
	game->play_sound("shutter", get_eye_position(), -4.0f);
	noise_radius = std::max(noise_radius, 2.5f);
	game->emit_noise(get_global_position(), 2.5f, true);
	if (photo_flash && game->get_level_root()) {
		FxFlash *flash = memnew(FxFlash);
		flash->setup(9.0f, 16.0f, 0.18f, Color(0.95f, 0.97f, 1.0f));
		game->get_level_root()->add_child(flash);
		flash->set_global_position(get_eye_position() + get_look_direction() * 0.4f);
		game->emit_noise(get_global_position(), 14.0f, true);
	}
	game->on_photo(camera);
}

void UrbexPlayer::update_focus() {
	focus = nullptr;
	if (!controls_enabled || dead) {
		return;
	}
	PhysicsDirectSpaceState3D *space = get_world_3d()->get_direct_space_state();
	Vector3 from = get_eye_position();
	Vector3 to = from + get_look_direction() * 2.6f;
	TypedArray<RID> exclude;
	exclude.push_back(get_rid());
	Ref<PhysicsRayQueryParameters3D> q = PhysicsRayQueryParameters3D::create(from, to, layer::WORLD | layer::DOORS | layer::INTERACT, exclude);
	Dictionary hit = space->intersect_ray(q);
	if (hit.is_empty()) {
		return;
	}
	Object *collider = hit["collider"];
	focus = Interactable::find_from(collider);
}

void UrbexPlayer::_physics_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !head) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game || !game->is_playing()) {
		return;
	}
	float dt = float(delta);
	Input *in = Input::get_singleton();
	bool active = controls_enabled && !dead;

	update_crouch(dt);

	Vector2 mv = active ? in->get_vector("move_left", "move_right", "move_forward", "move_back") : Vector2();
	bool sneak = active && in->is_action_pressed("sneak");
	aiming = active && in->is_action_pressed("aim");
	bool moving_input = mv.length() > 0.1f;
	bool run = active && moving_input && in->is_action_pressed("run") && !crouching && !aiming && !exhausted;

	float speed = 3.0f;
	if (crouching) {
		speed = sneak ? 0.65f : 1.5f;
	} else if (run) {
		speed = 5.6f;
	} else if (sneak) {
		speed = 1.1f;
	}
	if (aiming) {
		speed *= 0.6f;
	}

	if (run) {
		stamina -= dt / 6.5f;
		if (stamina <= 0.0f) {
			stamina = 0.0f;
			exhausted = true;
			game->notify("Сбилось дыхание"_u, Color(1.0f, 0.75f, 0.5f));
		}
	} else {
		stamina = std::min(1.0f, stamina + dt / (moving_input ? 11.0f : 7.0f));
		if (exhausted && stamina > 0.35f) {
			exhausted = false;
		}
	}

	Vector3 dir = get_global_transform().basis.xform(Vector3(mv.x, 0.0f, mv.y));
	dir.y = 0.0f;
	float mag = std::min(1.0f, mv.length());
	if (dir.length() > 0.001f) {
		dir = dir.normalized() * mag;
	}

	Vector3 vel = get_velocity();
	Vector3 horizontal(vel.x, 0.0f, vel.z);
	Vector3 target = dir * speed;
	bool on_floor = is_on_floor();
	float accel = on_floor ? 11.0f : 2.0f;
	horizontal = horizontal.lerp(target, 1.0f - std::exp(-accel * dt));
	vel.x = horizontal.x;
	vel.z = horizontal.z;
	if (!on_floor) {
		vel.y -= GRAVITY * dt;
	} else if (vel.y < 0.0f) {
		vel.y = -0.5f;
	}
	if (active && on_floor && in->is_action_just_pressed("jump") && !crouching) {
		vel.y = 4.8f;
		make_noise(3.0f);
	}
	last_vertical_speed = std::min(last_vertical_speed, vel.y);
	set_velocity(vel);
	move_and_slide();

	bool now_on_floor = is_on_floor();
	if (now_on_floor && !was_on_floor) {
		float impact = -last_vertical_speed;
		if (impact > 13.5f && !dead) {
			game->player_died("Сорвался с высоты. Перекрытия и шахты в заброшках не прощают ошибок"_u);
		} else if (impact > 7.5f) {
			game->play_sound("land", get_global_position(), 4.0f);
			make_noise(11.0f);
			add_shake(0.6f);
			game->notify("Жёсткое приземление. Слышно было на весь этаж"_u, Color(1.0f, 0.7f, 0.45f));
		} else if (impact > 3.5f) {
			game->play_sound("land", get_global_position(), -6.0f);
			make_noise(4.0f);
		}
	}
	if (now_on_floor) {
		last_vertical_speed = 0.0f;
	}
	was_on_floor = now_on_floor;

	if (get_global_position().y < game->get_level().kill_height && !dead) {
		game->player_died("Падение. Дальше только темнота"_u);
	}

	Vector3 real = get_real_velocity();
	float hspeed = Vector2(real.x, real.z).length();
	if (hspeed < 0.2f) {
		gait = GAIT_IDLE;
	} else if (run) {
		gait = GAIT_RUN;
	} else if (sneak) {
		gait = GAIT_SNEAK;
	} else if (crouching) {
		gait = GAIT_CROUCH;
	} else {
		gait = GAIT_WALK;
	}

	if (now_on_floor && hspeed > 0.2f) {
		float stride = 0.75f;
		float radius = 6.0f;
		switch (gait) {
			case GAIT_RUN:
				stride = 1.05f;
				radius = 13.0f;
				break;
			case GAIT_WALK:
				stride = 0.75f;
				radius = 6.0f;
				break;
			case GAIT_CROUCH:
				stride = 0.55f;
				radius = 2.4f;
				break;
			case GAIT_SNEAK:
				stride = 0.5f;
				radius = crouching ? 0.8f : 1.3f;
				break;
			default:
				break;
		}
		step_distance += hspeed * dt;
		bob_phase += hspeed * dt * PI / stride;
		if (step_distance >= stride) {
			step_distance = 0.0f;
			emit_step(radius);
		}
	}
	noise_radius = std::max(0.0f, noise_radius - 8.0f * dt);

	photo_cooldown = std::max(0.0f, photo_cooldown - dt);
	update_focus();
	if (active && in->is_action_just_pressed("interact") && focus) {
		focus->interact(this);
	}
	if (active && in->is_action_just_pressed("flashlight")) {
		if (battery > 0.0f) {
			flashlight_on = !flashlight_on;
			game->play_sound("click", get_eye_position(), -10.0f);
		} else {
			game->notify("Фонарик разряжен"_u, Color(1.0f, 0.7f, 0.4f));
		}
	}
	if (active && in->is_action_just_pressed("camera_flash")) {
		photo_flash = !photo_flash;
		game->play_sound("click", get_eye_position(), -8.0f, 1.4f);
		game->notify(photo_flash ? "Вспышка включена: снимки ярче, но охрана увидит вспышку издалека"_u : "Вспышка выключена"_u, Color(0.8f, 0.85f, 0.95f));
	}
	if (active && aiming && in->is_action_just_pressed("photo") && photo_cooldown <= 0.0f) {
		take_photo();
	}
	update_flashlight(dt);
}

void UrbexPlayer::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !head) {
		return;
	}
	float dt = float(delta);
	Vector3 real = get_real_velocity();
	float hspeed = Vector2(real.x, real.z).length();
	float target_bob = is_on_floor() ? clampf(hspeed / 5.6f, 0.0f, 1.0f) : 0.0f;
	bob_amount = lerpf(bob_amount, target_bob, 1.0f - std::exp(-8.0f * dt));
	float head_y = lerpf(1.6f, 0.92f, crouch_blend);
	Vector3 offset(std::sin(bob_phase) * 0.035f * bob_amount, std::fabs(std::cos(bob_phase)) * 0.05f * bob_amount, 0.0f);
	if (shake > 0.0f) {
		uint64_t f = Engine::get_singleton()->get_process_frames();
		offset += Vector3(float(int(f * 7919 % 21) - 10) * 0.004f, float(int(f * 104729 % 21) - 10) * 0.004f, 0.0f) * shake;
		shake = std::max(0.0f, shake - dt * 1.5f);
	}
	head->set_position(Vector3(0.0f, head_y, 0.0f) + offset);
	aim_blend = approach(aim_blend, aiming ? 1.0f : 0.0f, dt * 6.0f);
	camera->set_fov(lerpf(75.0f, 42.0f, aim_blend));
	if (viewmodel) {
		sway = sway.lerp(sway_target, 1.0f - std::exp(-10.0f * dt));
		sway_target = sway_target.lerp(Vector2(), 1.0f - std::exp(-6.0f * dt));
		Vector3 bob(std::sin(bob_phase) * 0.012f * bob_amount, -std::fabs(std::cos(bob_phase)) * 0.016f * bob_amount, 0.0f);
		viewmodel->set_position(Vector3(-sway.x, sway.y, 0.0f) + bob);
		torch_blend = approach(torch_blend, is_flashlight_on() ? 1.0f : 0.0f, dt * 4.0f);
		torch_arm->set_visible(torch_blend > 0.01f);
		torch_arm->set_position(Vector3(-0.19f, lerpf(-0.6f, -0.25f, torch_blend), -0.38f));
		phone_blend = approach(phone_blend, (aiming || dead || !controls_enabled) ? 0.0f : 1.0f, dt * 5.0f);
		phone_arm->set_visible(phone_blend > 0.01f);
		phone_arm->set_position(Vector3(0.2f, lerpf(-0.6f, -0.25f, phone_blend), -0.34f));
	}
}

}
