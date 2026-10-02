#include "world/interactable.h"

#include "actors/player.h"
#include "core/common.h"
#include "core/game.h"
#include "world/door.h"

#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>

using namespace godot;

namespace urbex {

StaticBody3D *Interactable::add_hitbox(const Vector3 &size, const Vector3 &offset) {
	StaticBody3D *body = memnew(StaticBody3D);
	body->set_collision_layer(layer::INTERACT);
	body->set_collision_mask(0);
	body->set_position(offset);
	add_child(body);
	Ref<BoxShape3D> shape;
	shape.instantiate();
	shape->set_size(size);
	CollisionShape3D *cs = memnew(CollisionShape3D);
	cs->set_shape(shape);
	body->add_child(cs);
	return body;
}

Interactable *Interactable::find_from(Object *collider) {
	Node *node = Object::cast_to<Node>(collider);
	for (int i = 0; i < 5 && node; i++) {
		if (Interactable *it = Object::cast_to<Interactable>(node)) {
			return it;
		}
		node = node->get_parent();
	}
	return nullptr;
}

void Pickup::setup(const String &id, const String &name, const String &desc, bool is_artifact) {
	item_id = id;
	item_name = name;
	description = desc;
	artifact = is_artifact;
}

String Pickup::get_prompt(UrbexPlayer *) const {
	return String("[E] Взять: "_u) + item_name;
}

void Pickup::interact(UrbexPlayer *player) {
	UrbexGame *game = UrbexGame::get_singleton();
	if (!artifact) {
		player->give_item(item_id, item_name);
	}
	if (item_id == "batteries") {
		player->recharge_battery();
	}
	game->play_sound("pickup", get_global_position(), -6.0f);
	game->on_item(item_id, item_name, artifact);
	if (!description.is_empty()) {
		game->notify(description, Color(0.75f, 0.8f, 0.7f));
	}
	queue_free();
}

void ActionPoint::setup(const String &p_prompt, std::function<void(UrbexPlayer *)> p_action, bool p_one_shot) {
	prompt = p_prompt;
	action = std::move(p_action);
	one_shot = p_one_shot;
}

void ActionPoint::set_requirement(const String &item, const String &missing) {
	required_item = item;
	missing_text = missing;
}

String ActionPoint::get_prompt(UrbexPlayer *player) const {
	if (used && one_shot) {
		return done_prompt;
	}
	if (!required_item.is_empty() && !player->has_item(required_item)) {
		return missing_text;
	}
	return String("[E] ") + prompt;
}

void ActionPoint::interact(UrbexPlayer *player) {
	if (used && one_shot) {
		return;
	}
	if (!required_item.is_empty() && !player->has_item(required_item)) {
		UrbexGame::get_singleton()->notify(missing_text, Color(1.0f, 0.7f, 0.4f));
		return;
	}
	used = true;
	if (action) {
		action(player);
	}
}

void NoteBoard::setup(const String &p_title, const String &p_body, const String &p_verb) {
	title = p_title;
	body = p_body;
	verb = p_verb;
}

String NoteBoard::get_prompt(UrbexPlayer *) const {
	return String("[E] ") + verb + ": " + title;
}

void NoteBoard::interact(UrbexPlayer *) {
	UrbexGame::get_singleton()->show_note(title, body);
}

void ClimbPoint::setup(const String &p_label, const Vector3 &p_target, float p_target_yaw, float p_duration) {
	label = p_label;
	target = p_target;
	target_yaw = p_target_yaw;
	duration = p_duration;
}

void ClimbPoint::set_requirement(const String &item, const String &missing) {
	required_item = item;
	missing_text = missing;
}

void ClimbPoint::set_required_door(Door *door, const String &closed_text) {
	required_door = door;
	door_closed_text = closed_text;
}

void ClimbPoint::set_noise(float radius, const String &snd) {
	noise_radius = radius;
	sound = snd;
}

String ClimbPoint::get_prompt(UrbexPlayer *player) const {
	if (required_door && !required_door->is_open()) {
		return door_closed_text;
	}
	if (!required_item.is_empty() && !player->has_item(required_item)) {
		return missing_text;
	}
	return String("[E] ") + label;
}

void ClimbPoint::interact(UrbexPlayer *player) {
	UrbexGame *game = UrbexGame::get_singleton();
	if (required_door && !required_door->is_open()) {
		game->notify(door_closed_text, Color(1.0f, 0.7f, 0.4f));
		return;
	}
	if (!required_item.is_empty() && !player->has_item(required_item)) {
		game->notify(missing_text, Color(1.0f, 0.7f, 0.4f));
		return;
	}
	if (!sound.is_empty()) {
		game->play_sound(sound, get_global_position(), 0.0f);
	}
	if (noise_radius > 0.0f) {
		game->emit_noise(get_global_position(), noise_radius, true);
	}
	if (!vibration_sensor.is_empty()) {
		game->raise_alarm(get_global_position(), vibration_sensor);
	}
	game->transition(target, target_yaw, duration, arrival_message, arrival_pose);
}

void PowerSwitch::setup(const String &p_group, const String &name, Node3D *lever_node) {
	group = p_group;
	switch_name = name;
	lever = lever_node;
}

String PowerSwitch::get_prompt(UrbexPlayer *) const {
	return String("[E] ") + (on ? "Обесточить: "_u : "Включить: "_u) + switch_name;
}

void PowerSwitch::interact(UrbexPlayer *) {
	on = !on;
	if (lever) {
		lever->set_rotation(Vector3(on ? -0.6f : 0.6f, 0.0f, 0.0f));
	}
	UrbexGame *game = UrbexGame::get_singleton();
	game->play_sound("switch", get_global_position(), 2.0f);
	game->set_power(group, on, get_global_position());
}

void ReedSwitch::setup(Door *p_door) {
	door = p_door;
}

String ReedSwitch::get_prompt(UrbexPlayer *player) const {
	if (!door) {
		return String();
	}
	if (door->is_reed_bypassed()) {
		return "Геркон обойдён магнитом"_u;
	}
	if (!door->is_reed_armed()) {
		return "Геркон ИО 102: питания нет"_u;
	}
	if (!player->has_item("magnet")) {
		return "Геркон ИО 102 на двери. Без магнита не обойти"_u;
	}
	return "[E] Приложить магнит к геркону"_u;
}

void ReedSwitch::interact(UrbexPlayer *player) {
	if (!door || door->is_reed_bypassed() || !door->is_reed_armed()) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	if (!player->has_item("magnet")) {
		game->notify("Нужен неодимовый магнит, чтобы геркон думал, что дверь закрыта"_u, Color(1.0f, 0.7f, 0.4f));
		return;
	}
	door->bypass_reed();
	game->play_sound("click", get_global_position(), -4.0f);
	game->notify("Магнит прилеплен к геркону. Теперь дверь можно открыть без тревоги"_u, Color(0.6f, 0.9f, 0.6f));
}

}
