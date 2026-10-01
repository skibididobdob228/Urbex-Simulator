#include "world/door.h"

#include "actors/player.h"
#include "core/common.h"
#include "core/game.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

namespace urbex {

namespace {

MeshInstance3D *make_box(Node *parent, const Vector3 &size, const Vector3 &pos, const Ref<Material> &material) {
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

}

void Door::setup(Style p_style, float p_width, float p_height, const String &name, const Ref<Material> &panel_material, const Ref<Material> &detail_material) {
	style = p_style;
	width = p_width;
	height = p_height;
	door_name = name;
	switch (style) {
		case STYLE_WOOD:
			thickness = 0.05f;
			break;
		case STYLE_METAL:
		case STYLE_GATE:
			thickness = 0.07f;
			break;
		case STYLE_BLAST:
			thickness = 0.24f;
			break;
		case STYLE_HATCH:
			thickness = 0.08f;
			break;
	}
	add_to_group("urbex_doors");

	panel = memnew(AnimatableBody3D);
	panel->set_sync_to_physics(false);
	panel->set_collision_layer(layer::DOORS);
	panel->set_collision_mask(0);
	add_child(panel);

	Vector3 size = style == STYLE_HATCH ? Vector3(width, thickness, height) : Vector3(width, height, thickness);
	Ref<BoxShape3D> shape;
	shape.instantiate();
	shape->set_size(size);
	CollisionShape3D *cs = memnew(CollisionShape3D);
	cs->set_shape(shape);
	panel->add_child(cs);

	panel_visual = memnew(Node3D);
	panel->add_child(panel_visual);
	make_box(panel_visual, size, Vector3(), panel_material);

	if (style == STYLE_HATCH) {
		make_box(panel_visual, Vector3(0.3f, 0.04f, 0.04f), Vector3(0.0f, thickness * 0.5f + 0.03f, height * 0.38f), detail_material);
	} else {
		float hx = width * 0.5f - 0.12f;
		float hy = style == STYLE_BLAST ? 0.0f : -height * 0.5f + 1.0f;
		float hz = thickness * 0.5f + 0.03f;
		if (style == STYLE_BLAST) {
			for (float y : { -0.7f, 0.0f, 0.7f }) {
				for (float side : { -1.0f, 1.0f }) {
					make_box(panel_visual, Vector3(0.3f, 0.06f, 0.06f), Vector3(hx - 0.1f, y, hz * side + 0.03f * side), detail_material);
				}
			}
			for (float side : { -1.0f, 1.0f }) {
				Ref<CylinderMesh> wheel;
				wheel.instantiate();
				wheel->set_top_radius(0.22f);
				wheel->set_bottom_radius(0.22f);
				wheel->set_height(0.04f);
				wheel->set_radial_segments(16);
				MeshInstance3D *w = memnew(MeshInstance3D);
				w->set_mesh(wheel);
				w->set_material_override(detail_material);
				w->set_rotation(Vector3(PI * 0.5f, 0.0f, 0.0f));
				w->set_position(Vector3(0.0f, 0.15f, (hz + 0.05f) * side));
				panel_visual->add_child(w);
			}
		} else {
			for (float side : { -1.0f, 1.0f }) {
				make_box(panel_visual, Vector3(0.14f, 0.035f, 0.05f), Vector3(hx, hy, hz * side), detail_material);
			}
		}
	}
	update_panel();
}

void Door::set_padlock(bool value) {
	padlock = value;
	if (padlock && !padlock_mesh) {
		UrbexGame *game = UrbexGame::get_singleton();
		Ref<Material> steel = game ? Ref<Material>(game->get_materials().get("steel")) : Ref<Material>();
		Vector3 pos = style == STYLE_HATCH ? Vector3(0.0f, thickness * 0.5f + 0.06f, height * 0.45f) : Vector3(width * 0.5f - 0.08f, -height * 0.5f + 1.05f, thickness * 0.5f + 0.06f);
		padlock_mesh = make_box(panel_visual, Vector3(0.07f, 0.09f, 0.035f), pos, steel);
	}
	if (padlock_mesh) {
		padlock_mesh->set_visible(padlock);
	}
}

void Door::set_reed(bool value, const String &group, const Ref<Material> &material) {
	reed = value;
	power_group = group;
	if (!reed || reed_mesh) {
		return;
	}
	Vector3 pos = style == STYLE_HATCH ? Vector3(width * 0.5f + 0.05f, 0.03f, height * 0.4f) : Vector3(width * 0.5f - 0.15f, height + 0.03f, thickness * 0.5f + 0.04f);
	reed_mesh = make_box(this, Vector3(0.1f, 0.03f, 0.03f), pos, material);
	ReedSwitch *rs = memnew(ReedSwitch);
	rs->setup(this);
	rs->set_position(pos);
	add_child(rs);
	rs->add_hitbox(Vector3(0.5f, 0.35f, 0.4f), Vector3());
}

void Door::set_jammed(bool value, const String &text, bool block_navigation) {
	jammed = value;
	jammed_text = text;
	if (panel && block_navigation) {
		panel->set_collision_layer(value ? (layer::DOORS | layer::WORLD) : layer::DOORS);
	}
}

bool Door::opens_from_inside(UrbexPlayer *player) const {
	return exit_button && to_local(player->get_global_position()).z < 0.0f;
}

void Door::close_door() {
	if (!open) {
		return;
	}
	open = false;
	target_angle = 0.0f;
	moving = true;
	fast_swing = false;
	UrbexGame::get_singleton()->play_sound("metal_door", get_center(), -6.0f, 1.1f);
}

void Door::bypass_reed() {
	reed_bypassed = true;
}

void Door::open_instantly() {
	open = true;
	angle = style == STYLE_HATCH ? -110.0f * DEG : 95.0f * DEG;
	target_angle = angle;
	update_panel();
}

float Door::swing_speed() const {
	switch (style) {
		case STYLE_BLAST:
			return fast_swing ? 1.3f : 0.55f;
		case STYLE_GATE:
			return 1.2f;
		case STYLE_HATCH:
			return 2.0f;
		default:
			return 2.6f;
	}
}

void Door::update_panel() {
	if (!panel) {
		return;
	}
	Transform3D xf;
	if (style == STYLE_HATCH) {
		Vector3 hinge(0.0f, 0.0f, -height * 0.5f);
		Basis rot(Vector3(1, 0, 0), angle);
		xf = Transform3D(rot, hinge) * Transform3D(Basis(), Vector3(0.0f, thickness * 0.5f, height * 0.5f));
	} else {
		Vector3 hinge(-width * 0.5f, 0.0f, 0.0f);
		Basis rot(Vector3(0, 1, 0), angle);
		xf = Transform3D(rot, hinge) * Transform3D(Basis(), Vector3(width * 0.5f, height * 0.5f, 0.0f));
	}
	panel->set_transform(xf);
}

Vector3 Door::get_center() const {
	if (style == STYLE_HATCH) {
		return get_global_position();
	}
	return get_global_transform().xform(Vector3(0.0f, height * 0.5f, 0.0f));
}

String Door::locked_reason(UrbexPlayer *player) const {
	if (opens_from_inside(player)) {
		return String();
	}
	if (jammed) {
		return jammed_text;
	}
	if (padlock) {
		return player->has_item("boltcutter") ? String("[E] Перекусить навесной замок болторезом (шумно)"_u) : String("Навесной замок. Нужен болторез"_u);
	}
	if (!key_id.is_empty()) {
		return player->has_item(key_id) ? String("[E] Отпереть: "_u) + door_name + " (" + item_display_name(key_id) + ")" : String("Заперто: "_u) + door_name + ". Нужен "_u + item_display_name(key_id).to_lower();
	}
	return String();
}

String Door::get_prompt(UrbexPlayer *player) const {
	if (open) {
		if (style == STYLE_BLAST) {
			return moving ? String() : String("[E] Закрыть гермодверь"_u);
		}
		return String("[E] Закрыть: "_u) + door_name;
	}
	String reason = locked_reason(player);
	if (!reason.is_empty()) {
		return reason;
	}
	if (opens_from_inside(player)) {
		return String("[E] Нажать кнопку выхода: "_u) + door_name;
	}
	String suffix = is_reed_armed() ? String("  (на двери геркон!)"_u) : String();
	if (style == STYLE_BLAST) {
		return String("[E] Провернуть кремальеры и открыть: "_u) + door_name + " (громко)"_u + suffix;
	}
	return String("[E] Открыть: "_u) + door_name + suffix;
}

void Door::interact(UrbexPlayer *player) {
	UrbexGame *game = UrbexGame::get_singleton();
	if (open) {
		if (moving && style == STYLE_BLAST) {
			return;
		}
		open = false;
		target_angle = 0.0f;
		moving = true;
		game->play_sound(style == STYLE_WOOD ? "creak" : "creak_low", get_center(), -4.0f);
		game->emit_noise(get_center(), style == STYLE_BLAST ? 14.0f : 3.0f, true);
		return;
	}
	if (opens_from_inside(player)) {
		game->play_sound("beep_low", get_center(), -6.0f);
		start_open(player->get_global_position(), false);
		return;
	}
	if (jammed) {
		game->notify(jammed_text, Color(1.0f, 0.7f, 0.4f));
		return;
	}
	if (padlock) {
		if (!player->has_item("boltcutter")) {
			game->play_sound("click", get_center(), -6.0f);
			game->notify("Навесной замок. Без болтореза не открыть"_u, Color(1.0f, 0.7f, 0.4f));
			return;
		}
		set_padlock(false);
		game->play_sound("cut", get_center(), 3.0f);
		game->emit_noise(get_center(), 9.0f, true);
		game->notify("Дужка замка перекушена"_u, Color(0.8f, 0.85f, 0.75f));
		return;
	}
	if (!key_id.is_empty()) {
		if (!player->has_item(key_id)) {
			game->play_sound("click", get_center(), -6.0f);
			game->notify(String("Заперто. Нужен "_u) + item_display_name(key_id).to_lower(), Color(1.0f, 0.7f, 0.4f));
			return;
		}
		game->notify(String("Открыто: "_u) + item_display_name(key_id).to_lower(), Color(0.8f, 0.85f, 0.75f));
		key_id = String();
	}
	start_open(player->get_global_position(), false);
}

void Door::start_open(const Vector3 &from, bool by_guard) {
	if (open) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	open = true;
	moving = true;
	fast_swing = by_guard;
	if (style == STYLE_HATCH) {
		target_angle = -110.0f * DEG;
	} else {
		Vector3 local = to_local(from);
		float sign = local.z >= 0.0f ? 1.0f : -1.0f;
		float max_angle = style == STYLE_BLAST ? 92.0f : 100.0f;
		target_angle = sign * max_angle * DEG;
	}

	Vector3 c = get_center();
	float noise = 5.0f;
	switch (style) {
		case STYLE_WOOD:
			game->play_sound("creak", c, -2.0f, 0.9f + float(Engine::get_singleton()->get_physics_frames() % 7) * 0.03f);
			noise = 5.0f;
			break;
		case STYLE_METAL:
		case STYLE_GATE:
			game->play_sound("metal_door", c, -3.0f);
			game->play_sound("creak_low", c, -6.0f);
			noise = 8.0f;
			break;
		case STYLE_BLAST:
			game->play_sound("blast_door", c, 4.0f, 1.0f, 60.0f);
			noise = 18.0f;
			break;
		case STYLE_HATCH:
			game->play_sound("metal_door", c, -2.0f, 1.2f);
			noise = 8.0f;
			break;
	}
	if (!by_guard) {
		game->emit_noise(c, noise, true);
		if (is_reed_armed()) {
			game->raise_alarm(c, String("Геркон на двери: "_u) + door_name);
		}
	}
}

void Door::open_by_guard(const Vector3 &from) {
	if (open || jammed) {
		return;
	}
	start_open(from, true);
}

void Door::_physics_process(double delta) {
	if (!moving) {
		return;
	}
	angle = approach(angle, target_angle, swing_speed() * float(delta));
	if (std::fabs(angle - target_angle) < 0.0001f) {
		moving = false;
	}
	update_panel();
}

}
