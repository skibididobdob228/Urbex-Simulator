#include "security/devices.h"

#include "actors/player.h"
#include "core/common.h"
#include "core/game.h"
#include "core/materials.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace urbex {

namespace {

MeshInstance3D *add_box(Node3D *parent, const Vector3 &size, const Vector3 &pos, const Ref<Material> &material) {
	Ref<BoxMesh> mesh;
	mesh.instantiate();
	mesh->set_size(size);
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(mesh);
	mi->set_material_override(material);
	mi->set_position(pos);
	parent->add_child(mi);
	return mi;
}

float segment_distance(const Vector3 &p1, const Vector3 &q1, const Vector3 &p2, const Vector3 &q2) {
	Vector3 d1 = q1 - p1;
	Vector3 d2 = q2 - p2;
	Vector3 r = p1 - p2;
	float a = d1.dot(d1);
	float e = d2.dot(d2);
	float f = d2.dot(r);
	float s = 0.0f;
	float t = 0.0f;
	if (a <= 1e-6f && e <= 1e-6f) {
		return r.length();
	}
	if (a <= 1e-6f) {
		t = clampf(f / e, 0.0f, 1.0f);
	} else {
		float c = d1.dot(r);
		if (e <= 1e-6f) {
			s = clampf(-c / a, 0.0f, 1.0f);
		} else {
			float b = d1.dot(d2);
			float denom = a * e - b * b;
			s = denom > 1e-6f ? clampf((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
			t = (b * s + f) / e;
			if (t < 0.0f) {
				t = 0.0f;
				s = clampf(-c / a, 0.0f, 1.0f);
			} else if (t > 1.0f) {
				t = 1.0f;
				s = clampf((b - c) / a, 0.0f, 1.0f);
			}
		}
	}
	Vector3 c1 = p1 + d1 * s;
	Vector3 c2 = p2 + d2 * t;
	return (c1 - c2).length();
}

} // namespace

void SecurityDevice::make_led(Node3D *parent, const Vector3 &position, float radius) {
	Ref<SphereMesh> mesh;
	mesh.instantiate();
	mesh->set_radius(radius);
	mesh->set_height(radius * 2.0f);
	mesh->set_radial_segments(8);
	mesh->set_rings(4);
	led_material.instantiate();
	led_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	led_material->set_albedo(Color(0.05f, 0.05f, 0.05f));
	led = memnew(MeshInstance3D);
	led->set_mesh(mesh);
	led->set_material_override(led_material);
	led->set_position(position);
	led->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	parent->add_child(led);
}

void SecurityDevice::set_led(const Color &color, bool lit) {
	if (led_material.is_null()) {
		return;
	}
	led_material->set_albedo(lit ? color : Color(0.05f, 0.05f, 0.05f));
}

void SecurityDevice::trip(const Vector3 &where) {
	UrbexGame *game = UrbexGame::get_singleton();
	if (game) {
		game->raise_alarm(where, device_name);
	}
}

void MotionSensor::setup(const MaterialLibrary &materials, float p_range, float fov_deg) {
	range = p_range;
	half_angle = fov_deg * 0.5f * DEG;
	add_to_group("urbex_devices");
	add_box(this, Vector3(0.09f, 0.12f, 0.06f), Vector3(), materials.get("plastic_white"));
	add_box(this, Vector3(0.06f, 0.05f, 0.012f), Vector3(0.0f, -0.015f, -0.033f), materials.get("plastic_dark"));
	make_led(this, Vector3(0.025f, 0.035f, -0.032f), 0.007f);
}

void MotionSensor::_physics_process(double delta) {
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game || !game->is_playing()) {
		return;
	}
	float dt = float(delta);
	led_clock += dt;
	rearm_timer = std::max(0.0f, rearm_timer - dt);
	if (!is_active()) {
		signal = 0.0f;
		set_led(Color(), false);
		return;
	}
	UrbexPlayer *player = game->get_player();
	if (!player) {
		return;
	}
	Vector3 origin = get_global_position();
	Vector3 forward = -get_global_transform().basis.get_column(2).normalized();
	Vector3 target = player->get_chest_position();
	Vector3 to = target - origin;
	float dist = to.length();
	bool sees = false;
	Vector3 dir;
	if (dist < range && dist > 0.05f) {
		dir = to / dist;
		if (forward.dot(dir) > std::cos(half_angle)) {
			sees = game->line_of_sight(origin + forward * 0.08f, target, layer::WORLD | layer::DOORS);
		}
	}
	if (sees) {
		Vector3 v = player->get_real_velocity();
		float radial = v.dot(dir);
		float tangential = (v - dir * radial).length();
		float effective = (tangential + 0.3f * std::fabs(radial)) * (1.0f - 0.45f * dist / range);
		float excess = effective - 0.5f;
		if (excess > 0.0f) {
			signal += excess * 1.3f * dt;
		} else {
			signal -= 0.45f * dt;
		}
	} else {
		signal -= 0.45f * dt;
	}
	signal = clampf(signal, 0.0f, 1.2f);
	if (signal >= 1.0f && rearm_timer <= 0.0f) {
		trip(origin);
		rearm_timer = 6.0f;
		signal = 0.4f;
		game->play_sound("beep", origin, -2.0f);
	}

	if (rearm_timer > 0.0f) {
		set_led(Color(1.0f, 0.1f, 0.05f), std::fmod(led_clock, 0.3f) < 0.15f);
	} else if (signal > 0.05f) {
		set_led(Color(1.0f, 0.15f + 0.5f * (1.0f - signal), 0.05f), true);
	} else {
		set_led(Color(0.1f, 1.0f, 0.2f), std::fmod(led_clock, 3.0f) < 0.08f);
	}
}

void LaserBarrier::setup(const MaterialLibrary &materials, const Vector3 &global_a, const Vector3 &global_b, float floor_y) {
	a = global_a;
	b = global_b;
	add_to_group("urbex_devices");
	set_global_transform(Transform3D());
	for (const Vector3 &p : { a, b }) {
		float h = p.y - floor_y + 0.2f;
		add_box(this, Vector3(0.09f, h, 0.09f), Vector3(p.x, floor_y + h * 0.5f, p.z), materials.get("metal_gray"));
		add_box(this, Vector3(0.12f, 0.12f, 0.12f), p, materials.get("plastic_dark"));
	}
	make_led(this, a + Vector3(0.0f, 0.1f, 0.0f), 0.012f);

	Vector3 d = b - a;
	float len = d.length();
	Ref<BoxMesh> mesh;
	mesh.instantiate();
	mesh->set_size(Vector3(0.01f, 0.01f, len));
	beam_material = materials.get("beam")->duplicate();
	beam = memnew(MeshInstance3D);
	beam->set_mesh(mesh);
	beam->set_material_override(beam_material);
	beam->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	Vector3 z = d / len;
	Vector3 up = std::fabs(z.y) > 0.95f ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
	Vector3 x = up.cross(z).normalized();
	Vector3 y = z.cross(x).normalized();
	beam->set_transform(Transform3D(Basis(x, y, z), (a + b) * 0.5f));
	add_child(beam);
}

void LaserBarrier::_physics_process(double delta) {
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game || !game->is_playing()) {
		return;
	}
	float dt = float(delta);
	led_clock += dt;
	rearm_timer = std::max(0.0f, rearm_timer - dt);
	bool active = is_active();
	beam->set_visible(active);
	if (!active) {
		set_led(Color(), false);
		return;
	}
	UrbexPlayer *player = game->get_player();
	if (!player) {
		return;
	}
	Vector3 feet = player->get_global_position();
	Vector3 p0 = feet + Vector3(0.0f, 0.3f, 0.0f);
	Vector3 p1 = feet + Vector3(0.0f, std::max(0.35f, player->get_height() - 0.3f), 0.0f);
	float dist = segment_distance(a, b, p0, p1);
	if (dist < 0.34f && rearm_timer <= 0.0f) {
		trip(player->get_global_position());
		rearm_timer = 5.0f;
		game->play_sound("beep", a, -2.0f);
	}

	float nearest = segment_distance(a, b, feet, feet + Vector3(0, 1.7f, 0));
	float alpha = 0.08f;
	if (player->is_flashlight_on() && nearest < 12.0f) {
		alpha = 0.32f;
	}
	if (rearm_timer > 0.0f) {
		alpha = std::fmod(led_clock, 0.4f) < 0.2f ? 0.6f : 0.15f;
	}
	beam_material->set_albedo(Color(1.0f, 0.05f, 0.05f, alpha));
	set_led(rearm_timer > 0.0f ? Color(1, 0.1f, 0.05f) : Color(0.1f, 1.0f, 0.2f), rearm_timer > 0.0f || std::fmod(led_clock, 2.0f) < 0.1f);
}

void SecurityCamera::setup(const MaterialLibrary &materials, float p_base_yaw, float sweep_deg, float pitch_deg, bool is_fake) {
	base_yaw = p_base_yaw;
	sweep = sweep_deg * DEG;
	pitch = pitch_deg * DEG;
	fake = is_fake;
	add_to_group("urbex_devices");
	add_to_group("urbex_cameras");

	add_box(this, Vector3(0.06f, 0.06f, 0.22f), Vector3(0.0f, 0.0f, 0.11f), materials.get("metal_gray"));
	pivot = memnew(Node3D);
	add_child(pivot);
	pivot->set_rotation(Vector3(pitch, 0.0f, 0.0f));
	add_box(pivot, Vector3(0.14f, 0.12f, 0.3f), Vector3(0.0f, -0.02f, -0.05f), materials.get("plastic_white"));
	add_box(pivot, Vector3(0.18f, 0.02f, 0.36f), Vector3(0.0f, 0.05f, -0.07f), materials.get("plastic_white"));
	Ref<CylinderMesh> lens;
	lens.instantiate();
	lens->set_top_radius(0.04f);
	lens->set_bottom_radius(0.045f);
	lens->set_height(0.03f);
	MeshInstance3D *lens_mi = memnew(MeshInstance3D);
	lens_mi->set_mesh(lens);
	lens_mi->set_material_override(materials.get("plastic_dark"));
	lens_mi->set_rotation(Vector3(PI * 0.5f, 0.0f, 0.0f));
	lens_mi->set_position(Vector3(0.0f, -0.02f, -0.21f));
	pivot->add_child(lens_mi);
	make_led(pivot, Vector3(0.05f, 0.02f, -0.205f), 0.008f);
	yaw = 0.0f;
}

void SecurityCamera::_physics_process(double delta) {
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game || !game->is_playing()) {
		return;
	}
	float dt = float(delta);
	led_clock += dt;
	rearm_timer = std::max(0.0f, rearm_timer - dt);
	if (!is_active()) {
		detection = std::max(0.0f, detection - dt);
		set_led(Color(), false);
		return;
	}
	if (fake) {
		set_led(Color(1.0f, 0.1f, 0.05f), std::fmod(led_clock, 1.5f) < 0.75f);
		return;
	}

	UrbexPlayer *player = game->get_player();
	vision_tick -= dt;
	if (vision_tick <= 0.0f && player) {
		vision_tick = 0.1f;
		Vector3 origin = pivot->get_global_position();
		Vector3 forward = -pivot->get_global_transform().basis.get_column(2).normalized();
		Vector3 target = player->get_chest_position();
		Vector3 to = target - origin;
		float dist = to.length();
		sees_player = false;
		if (dist < range && dist > 0.1f) {
			Vector3 dir = to / dist;
			float cone = std::cos(tracking ? half_fov * 1.6f : half_fov);
			if (forward.dot(dir) > cone) {
				sees_player = game->line_of_sight(origin + forward * 0.3f, target, layer::WORLD | layer::DOORS);
			}
		}
	}

	if (sees_player && player) {
		Vector3 origin = pivot->get_global_position();
		float dist = (player->get_chest_position() - origin).length();
		float exposure = std::max(player->get_exposure(), dist < 9.0f ? 0.45f : 0.2f);
		float rate = (0.5f + 1.5f * (1.0f - dist / range)) * (0.35f + exposure);
		if (player->is_crouching() && player->get_real_velocity().length() < 0.2f) {
			rate *= 0.5f;
		}
		detection += rate * dt;
		if (detection >= 1.0f) {
			detection = 1.0f;
			tracking = true;
			if (rearm_timer <= 0.0f) {
				trip(player->get_global_position());
				rearm_timer = 8.0f;
			}
		}
		if (tracking) {
			Vector3 local = to_local(player->get_chest_position());
			float target_yaw = std::atan2(-local.x, -local.z);
			yaw = approach_angle(yaw, target_yaw, 1.2f * dt);
		}
	} else {
		detection = std::max(0.0f, detection - 0.3f * dt);
		if (detection <= 0.0f) {
			tracking = false;
		}
		if (!tracking) {
			if (pause > 0.0f) {
				pause -= dt;
			} else {
				yaw += direction * 0.35f * dt;
				if (yaw > sweep) {
					yaw = sweep;
					direction = -1.0f;
					pause = 1.6f;
				} else if (yaw < -sweep) {
					yaw = -sweep;
					direction = 1.0f;
					pause = 1.6f;
				}
			}
		}
	}
	pivot->set_rotation(Vector3(pitch, yaw, 0.0f));
	bool blink = detection > 0.05f ? std::fmod(led_clock, 0.25f) < 0.12f : std::fmod(led_clock, 2.0f) < 1.0f;
	set_led(Color(1.0f, 0.1f, 0.05f), blink);
}

void PhotoSpot::setup(const String &id, const String &p_title, float distance) {
	spot_id = id;
	title = p_title;
	max_distance = distance;
	add_to_group("urbex_photo_spots");
}

void PhotoSpot::set_zone(const AABB &aabb) {
	has_zone = true;
	zone = aabb;
}

bool PhotoSpot::in_frame(Node3D *camera, Vector3 forward) const {
	UrbexGame *game = UrbexGame::get_singleton();
	Vector3 cam = camera->get_global_position();
	Vector3 target = get_global_position();
	if (has_zone && !zone.has_point(cam)) {
		return false;
	}
	Vector3 to = target - cam;
	float dist = to.length();
	if (dist > max_distance || dist < 0.3f) {
		return false;
	}
	if (forward.normalized().dot(to / dist) < std::cos(half_angle)) {
		return false;
	}
	return game->line_of_sight(cam, target, layer::WORLD | layer::DOORS);
}

} // namespace urbex
