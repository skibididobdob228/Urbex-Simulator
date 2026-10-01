#pragma once

#include "core/builder.h"

namespace urbex {

namespace props {

void hospital_bed(LevelBuilder &b, const godot::Vector3 &pos, float yaw, bool mattress);
void wheelchair(LevelBuilder &b, const godot::Vector3 &pos, float yaw, bool tipped);
void cabinet(LevelBuilder &b, const godot::Vector3 &pos, float yaw, bool open);
void chair(LevelBuilder &b, const godot::Vector3 &pos, float yaw, bool fallen);
void table(LevelBuilder &b, const godot::Vector3 &pos, float yaw, float width, float depth);
void barrel(LevelBuilder &b, const godot::Vector3 &pos, const char *material, bool open_top);
void mattress(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void bottles(LevelBuilder &b, const godot::Vector3 &pos, int count, uint32_t seed);
void cardboard(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void car(LevelBuilder &b, const godot::Vector3 &pos, float yaw, const char *paint, bool lights_on);
void uaz_van(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void truck(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void bench(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void trash_container(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void swing(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void slide(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void birch(LevelBuilder &b, const godot::Vector3 &pos, float height, uint32_t seed);
void soviet_lamp(LevelBuilder &b, const godot::Vector3 &pos, float yaw, bool lit = true);
void radiator(LevelBuilder &b, const godot::Vector3 &pos, float yaw, int sections);
void garbage_chute(LevelBuilder &b, const godot::Vector3 &pos, float yaw, float height);
void electric_panel(LevelBuilder &b, const godot::Vector3 &pos, float yaw, bool open);
void water_tank(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void valve(LevelBuilder &b, const godot::Vector3 &pos, const godot::Vector3 &axis, float radius);
void vent_duct(LevelBuilder &b, const godot::Vector3 &a, const godot::Vector3 &c, float width, float height, float ceiling);
void cable_tray(LevelBuilder &b, const godot::Vector3 &a, const godot::Vector3 &c);
void hanging_cable(LevelBuilder &b, const godot::Vector3 &a, const godot::Vector3 &c, float sag);
void stretcher(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void sink(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void wall_phone(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void wall_clock(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void fire_shield(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void extinguisher(LevelBuilder &b, const godot::Vector3 &pos);
void lockers(LevelBuilder &b, const godot::Vector3 &pos, float yaw, int count);
void crates(LevelBuilder &b, const godot::Vector3 &pos, float yaw, int count, const godot::String &label, uint32_t seed);
void window_frame(LevelBuilder &b, const godot::Vector3 &center, float yaw, float width, float height, float broken);
void rebar(LevelBuilder &b, const godot::Vector3 &pos, const godot::Vector3 &dir, int count, uint32_t seed);
void balcony(LevelBuilder &b, const godot::Vector3 &pos, float yaw, bool glazed, uint32_t seed);
void ac_unit(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void antenna(LevelBuilder &b, const godot::Vector3 &pos, float height);
void satellite_dish(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void tec_chimney(LevelBuilder &b, const godot::Vector3 &pos, float height, float radius);
void shipping_container(LevelBuilder &b, const godot::Vector3 &pos, float yaw, const char *material);
void pallets(LevelBuilder &b, const godot::Vector3 &pos, float yaw, int count);
void ritual_circle(LevelBuilder &b, const godot::Vector3 &center, float radius);
void mailboxes(LevelBuilder &b, const godot::Vector3 &pos, float yaw, int columns, int rows);
void flower_pot(LevelBuilder &b, const godot::Vector3 &pos);
void doormat(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void wheel_valve_door_frame(LevelBuilder &b, const godot::Vector3 &pos, float yaw, float width, float height);
void drain_pipe(LevelBuilder &b, const godot::Vector3 &top, float height);
void lightning_rod(LevelBuilder &b, const godot::Vector3 &pos, float height);
void radio_station(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void tv_old(LevelBuilder &b, const godot::Vector3 &pos, float yaw);
void kettle(LevelBuilder &b, const godot::Vector3 &pos);
void puddle(LevelBuilder &b, const godot::Vector3 &pos, float size, uint32_t seed);

}

}
