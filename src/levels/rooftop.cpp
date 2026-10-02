#include "levels/kit.h"
#include "levels/levels.h"

#include "actors/humanoid.h"
#include "actors/player.h"
#include "core/game.h"

#include <godot_cpp/classes/omni_light3d.hpp>

#include <memory>

using namespace godot;

namespace urbex {

namespace {

constexpr float FH = 3.0f;
constexpr int FLOORS = 12;
constexpr float TECH = FH * FLOORS;
constexpr float TECH_H = 2.4f;
constexpr float ROOF = TECH + TECH_H + 0.3f;
constexpr float XW = 10.0f;
constexpr float ZW = 7.0f;

void stairs(LevelBuilder &b, float y) {
	float ym = y + FH * 0.5f;
	float yt = y + FH;
	b.ramp(Vector3(8.6f, y + 0.01f, -2.5f), Vector3(8.6f, ym, -5.1f), 1.6f, "concrete_floor", true);
	b.box(Vector3(7.0f, ym - 0.125f, -5.95f), Vector3(5.6f, 0.25f, 1.7f), "concrete_floor");
	b.ramp(Vector3(5.2f, ym, -5.1f), Vector3(5.2f, yt, -2.5f), 1.6f, "concrete_floor", true);
	b.box(Vector3(6.8f, y + (FH - 0.3f) * 0.5f, -3.8f), Vector3(1.6f, FH - 0.3f, 2.6f), "paint_beige");
}

struct Resident {
	Node3D *node = nullptr;
	HumanoidRig rig;
	float timer = 25.0f;
	int phase = 0;
	float phase_time = 0.0f;
	float anim = 0.0f;
};

struct StairLight {
	OmniLight3D *light = nullptr;
	int probe = -1;
	Vector3 position;
	float timer = 0.0f;
};

void facade_dressing(LevelBuilder &b) {
	Rng r(31);
	for (int n = 1; n < FLOORS; n++) {
		float y = FH * float(n) + 0.05f;
		for (float x : { -7.0f, -2.5f }) {
			props::balcony(b, Vector3(x, y, -ZW - 0.2f), 0.0f, r.chance(0.55f), uint32_t(n * 17) + uint32_t(x * -3.0f));
		}
		for (float x : { 2.5f, 7.0f }) {
			props::balcony(b, Vector3(x, y, ZW + 0.2f), PI, r.chance(0.55f), uint32_t(n * 23) + uint32_t(x * 5.0f));
		}
		if (r.chance(0.55f)) {
			props::ac_unit(b, Vector3(XW + 0.2f, y + 2.0f, r.range(-5.0f, 5.0f)), -PI * 0.5f);
		}
		if (r.chance(0.5f)) {
			props::ac_unit(b, Vector3(-XW - 0.2f, y + 2.0f, r.range(-5.0f, 5.0f)), PI * 0.5f);
		}
		if (r.chance(0.35f)) {
			props::satellite_dish(b, Vector3(-XW - 0.52f, y + 1.6f, r.range(-5.0f, 5.0f)), PI * 0.5f);
		}
		if (r.chance(0.3f)) {
			props::satellite_dish(b, Vector3(r.range(-0.8f, 3.4f), y + 1.8f, -ZW - 0.52f), 0.0f);
		}
	}
	for (float sx : { -1.0f, 1.0f }) {
		for (float sz : { -1.0f, 1.0f }) {
			props::drain_pipe(b, Vector3(sx * (XW + 0.28f), ROOF + 0.2f, sz * (ZW + 0.28f)), ROOF + 0.2f);
		}
	}
}

void stair_dressing(LevelBuilder &b) {
	for (int n = 0; n < FLOORS; n++) {
		float y = FH * float(n);
		float ym = y + FH * 0.5f;
		props::garbage_chute(b, Vector3(9.4f, ym, -6.5f), PI * 0.5f, FH);
		props::radiator(b, Vector3(6.2f, ym, -6.84f), 0.0f, 9);
		props::electric_panel(b, Vector3(9.81f, y + 1.55f, -1.75f), PI * 0.5f, n == 7);
		if (n % 3 == 1) {
			props::flower_pot(b, Vector3(4.4f, y + 0.02f, -1.3f));
		}
	}
}

}

void build_rooftop(LevelBuilder &b, LevelData &d, UrbexGame &g) {
	Kit k(b, d, g);
	d.id = "rooftop";
	d.title = "Руфинг: высотка на рассвете"_u;
	d.subtitle = "12 этажей, консьержка и камера на кровле"_u;
	d.briefing = "[b]Объект:[/b] двенадцатиэтажная башня в закрытом ЖК. До рассвета полчаса — надо успеть на крышу.\n\n"_u
				 "[b]Подъезд[/b] на северной стороне, дверь на домофоне. Ключа нет: жди, пока кто-нибудь из жильцов выйдет, и проскочи. За стеклом сидит консьержка — когда она смотрит в телевизор, у тебя есть пара секунд.\n\n"_u
				 "[b]Наверх[/b] — по лестнице, в подъезде свет на датчиках движения: загорается, когда проходишь мимо. Лифт есть, но холл у лифта она видит.\n\n"_u
				 "[b]Чердак:[/b] дверь на технический этаж заперта и стоит на герконе. Ключи часто оставляют в электрощитке на последнем этаже. У тебя есть неодимовый магнит.\n\n"_u
				 "[b]Нужно:[/b] снять панораму делового центра на западе, рассвет на востоке и оставить стикер команды. Потом спуститься и уйти со двора через калитку на юге."_u;
	d.exit_hint = "через калитку ЖК на юге"_u;
	d.spawn_position = Vector3(0.0f, 0.1f, -46.0f);
	d.spawn_yaw = PI;
	d.exit_zone = AABB(Vector3(-80.0f, -1.0f, -70.0f), Vector3(160.0f, 8.0f, 32.0f));
	d.ambient_exposure = 0.34f;
	d.indoor_exposure = 0.1f;
	d.kill_height = -10.0f;
	d.gbr_delay = 70.0f;
	d.gbr_spawn = Vector3(1.5f, 0.1f, -33.0f);
	d.gbr_van = true;
	d.gbr_van_position = Vector3(6.0f, 0.05f, -40.5f);
	d.gbr_van_yaw = PI * 0.5f;
	d.ambience = Ambience::Rooftop;
	d.nav_bounds = AABB(Vector3(-42.0f, -1.0f, -37.0f), Vector3(84.0f, 43.0f, 74.0f));
	d.start_items.push_back("magnet");
	d.legal_note = "Сама прогулка по крыше часто не наказуема, но взлом двери чердака — уже правонарушение. Если выход на кровлю не закрыт, штрафуют управляющую компанию. А падение с 12 этажа не прощает ничего."_u;

	k.objective_photo("city", "Сфоткать панораму делового центра (запад)"_u);
	k.objective_photo("sunrise", "Сфоткать рассвет (восток)"_u);
	k.objective_action("sticker", "Оставить стикер команды на крыше"_u);
	k.objective_item("badge", "Найти значок руфера"_u, true);

	b.slab(-200.0f, -200.0f, 200.0f, 200.0f, 0.0f, 0.5f, "ground");
	b.box(Vector3(0.0f, 0.02f, -46.0f), Vector3(400.0f, 0.06f, 14.0f), "asphalt");
	b.box(Vector3(0.0f, 0.03f, -21.0f), Vector3(3.0f, 0.06f, 28.0f), "asphalt");
	b.box(Vector3(-24.0f, 0.03f, 10.0f), Vector3(30.0f, 0.06f, 3.0f), "asphalt");
	b.box(Vector3(-11.0f, 0.03f, 10.0f), Vector3(3.0f, 0.06f, 6.0f), "asphalt");
	b.box(Vector3(24.0f, 0.03f, -12.0f), Vector3(18.0f, 0.06f, 30.0f), "asphalt");

	b.box(Vector3(-XW - 0.1f, ROOF * 0.5f - 0.15f, 0.0f), Vector3(0.2f, ROOF - 0.3f, 2.0f * ZW + 0.4f), "facade");
	b.box(Vector3(XW + 0.1f, ROOF * 0.5f - 0.15f, 0.0f), Vector3(0.2f, ROOF - 0.3f, 2.0f * ZW + 0.4f), "facade");
	b.box(Vector3(0.0f, ROOF * 0.5f - 0.15f, -ZW - 0.1f), Vector3(2.0f * XW, ROOF - 0.3f, 0.2f), "facade");
	b.wall(Vector3(-XW, 0, ZW + 0.1f), Vector3(XW, 0, ZW + 0.1f), 0.0f, ROOF - 0.3f, 0.2f, "facade", { door_gap(XW - 6.0f, 1.4f, 2.3f) });
	b.box(Vector3(-6.0f, 3.0f, ZW + 1.2f), Vector3(3.0f, 0.2f, 2.2f), "concrete");
	b.text(Vector3(-6.0f, 2.6f, ZW + 0.22f), Vector3(0, 0, 1), "ПОДЪЕЗД 1"_u, 0.22f, Color(0.95f, 0.95f, 0.9f));
	b.visual_box(Vector3(-5.0f, 1.35f, ZW + 0.23f), Vector3(0.18f, 0.26f, 0.05f), "metal_gray");
	k.ceiling_lamp(Vector3(-6.0f, 2.85f, ZW + 1.1f), Color(1.0f, 0.85f, 0.6f), 8.0f, 1.2f, "", false);

	for (int n = 0; n <= FLOORS; n++) {
		float y = FH * float(n);
		std::vector<Rect2> holes;
		if (n >= 1) {
			holes.push_back(Rect2(4.2f, -6.8f, 5.6f, 4.3f));
		}
		if (n == FLOORS) {
			b.slab(-XW, -ZW, XW, ZW, y + 0.05f, 0.3f, "concrete_floor", holes);
		} else {
			b.slab(-XW, -ZW, XW, ZW, n == 0 ? 0.05f : y, n == 0 ? 0.2f : 0.3f, "concrete_floor", holes);
		}
	}
	b.slab(-XW - 0.2f, -ZW - 0.2f, XW + 0.2f, ZW + 0.2f, ROOF, 0.3f, "roof", { Rect2(-5.45f, 2.05f, 0.9f, 0.9f) });

	std::shared_ptr<std::vector<StairLight>> stair_lights = std::make_shared<std::vector<StairLight>>();
	for (int n = 0; n < FLOORS; n++) {
		float y = FH * float(n);
		float wh = FH - 0.3f;
		stairs(b, y);
		b.wall(Vector3(4.0f, 0, -ZW), Vector3(4.0f, 0, -1.0f), y, wh, 0.2f, "paint_beige");
		b.wall(Vector3(4.0f, 0, -1.0f), Vector3(XW, 0, -1.0f), y, wh, 0.2f, "paint_beige", { door_gap(3.0f, 1.3f, 2.2f) });
		if (n == 0) {
			b.wall(Vector3(-XW, 0, 2.0f), Vector3(XW, 0, 2.0f), y, wh, 0.2f, "paint_beige", { door_gap(4.0f, 1.6f, 2.3f) });
			b.wall(Vector3(-8.0f, 0, 2.0f), Vector3(-8.0f, 0, ZW), y, wh, 0.2f, "paint_beige");
			b.wall(Vector3(-4.0f, 0, 2.0f), Vector3(-4.0f, 0, ZW), y, wh, 0.2f, "paint_beige");
			b.wall(Vector3(-XW, 0, -1.0f), Vector3(4.0f, 0, -1.0f), y, wh, 0.2f, "paint_beige", { window_gap(3.5f, 3.0f, 0.95f, 2.2f), door_gap(5.55f, 0.9f, 2.1f) });
			b.wall(Vector3(-8.0f, 0, ZW - 0.05f), Vector3(-4.0f, 0, ZW - 0.05f), y, wh, 0.1f, "paint_beige", { door_gap(2.0f, 1.4f, 2.3f) });
			b.wall(Vector3(-4.0f, 0, -ZW), Vector3(-4.0f, 0, -1.0f), y, wh, 0.2f, "paint_beige");
			b.wall(Vector3(1.6f, 0, -ZW), Vector3(1.6f, 0, -1.0f), y, wh, 0.2f, "paint_beige");
		} else {
			b.wall(Vector3(-XW, 0, 2.0f), Vector3(XW, 0, 2.0f), y, wh, 0.2f, "paint_beige");
			b.wall(Vector3(-XW, 0, -1.0f), Vector3(4.0f, 0, -1.0f), y, wh, 0.2f, "paint_beige");
		}
		b.wall(Vector3(-XW + 0.05f, 0, -1.0f), Vector3(-XW + 0.05f, 0, 2.0f), y, wh, 0.1f, "paint_beige");
		b.wall(Vector3(XW - 0.05f, 0, -1.0f), Vector3(XW - 0.05f, 0, 2.0f), y, wh, 0.1f, "paint_beige");
		b.wall(Vector3(4.0f, 0, -ZW + 0.05f), Vector3(XW, 0, -ZW + 0.05f), y, wh, 0.1f, "paint_beige");
		b.wall(Vector3(XW - 0.05f, 0, -ZW), Vector3(XW - 0.05f, 0, -1.0f), y, wh, 0.1f, "paint_beige");
		b.wall(Vector3(-XW, 0, -0.89f), Vector3(4.0f, 0, -0.89f), y, 1.2f, 0.02f, "paint_green", n == 0 ? std::vector<Opening>{ window_gap(3.5f, 3.0f, 0.95f, 2.2f), door_gap(5.55f, 0.9f, 2.1f) } : std::vector<Opening>{}, false);

		if (n > 0) {
			for (int a = 0; a < 4; a++) {
				float x = -8.0f + float(a) * 4.6f;
				b.visual_box(Vector3(x, y + 1.05f, 1.88f), Vector3(0.95f, 2.05f, 0.06f), "door_wood");
				b.text(Vector3(x, y + 2.25f, 1.86f), Vector3(0, 0, -1), String::num_int64(n * 5 + a + 1), 0.13f, Color(0.95f, 0.9f, 0.7f));
			}
			b.visual_box(Vector3(-7.0f, y + 1.05f, -0.88f), Vector3(0.95f, 2.05f, 0.06f), "door_wood");
			b.text(Vector3(-7.0f, y + 2.25f, -0.86f), Vector3(0, 0, 1), String::num_int64(n * 5 + 5), 0.13f, Color(0.95f, 0.9f, 0.7f));
			b.text(Vector3(4.1f, y + 1.9f, -1.6f), Vector3(1, 0, 0), String::num_int64(n + 1), 0.45f, Color(0.3f, 0.3f, 0.3f));
		}
		b.visual_box(Vector3(-1.2f, y + 1.05f, -0.86f), Vector3(1.1f, 2.1f, 0.05f), "metal_gray");
		b.visual_box(Vector3(-1.2f, y + 1.05f, -0.83f), Vector3(0.02f, 2.1f, 0.01f), "black");

		Vector3 lamp_pos(7.0f, y + FH - 0.35f, -1.8f);
		b.visual_box(lamp_pos + Vector3(0, 0.08f, 0), Vector3(0.25f, 0.08f, 0.25f), "plastic_white", Basis(), false);
		OmniLight3D *l = b.omni(lamp_pos, Color(1.0f, 0.88f, 0.7f), 7.0f, 1.3f, false);
		l->set_visible(false);
		StairLight sl;
		sl.light = l;
		sl.position = Vector3(7.0f, y + 1.0f, -3.5f);
		k.probe(lamp_pos - Vector3(0, 1.6f, 0), 5.0f, 0.75f);
		sl.probe = int(d.lights.size()) - 1;
		d.lights[size_t(sl.probe)].enabled = false;
		stair_lights->push_back(sl);
	}
	k.shadow_zone(Vector3(0.0f, TECH * 0.5f, 0.0f), Vector3(2.0f * XW, TECH + TECH_H, 2.0f * ZW));

	b.box(Vector3(-7.0f, 0.5f, -5.5f), Vector3(1.6f, 1.0f, 0.7f), "wood");
	b.visual_box(Vector3(-9.3f, 1.05f, -4.0f), Vector3(0.12f, 0.4f, 0.6f), "plastic_dark");
	b.visual_box(Vector3(-9.23f, 1.05f, -4.0f), Vector3(0.01f, 0.32f, 0.52f), "tv_glow");
	b.visual_box(Vector3(-6.5f, 1.58f, -1.0f), Vector3(3.0f, 1.25f, 0.02f), "glass");
	b.box(Vector3(-6.4f, 0.45f, -2.4f), Vector3(0.5f, 0.9f, 0.5f), "wood", false);
	k.ceiling_lamp(Vector3(-6.5f, FH - 0.36f, -4.0f), Color(1.0f, 0.82f, 0.6f), 6.0f, 1.0f, "");
	k.ceiling_lamp(Vector3(-3.0f, FH - 0.36f, 0.5f), Color(1.0f, 0.9f, 0.75f), 7.0f, 1.0f, "");
	k.ceiling_lamp(Vector3(3.5f, FH - 0.36f, 0.5f), Color(1.0f, 0.9f, 0.75f), 7.0f, 0.9f, "");
	props::mailboxes(b, Vector3(-0.97f, 1.55f, 1.81f), 0.0f, 5, 2);
	b.text(Vector3(-0.95f, 1.85f, 1.86f), Vector3(0, 0, -1), "ПОЧТА"_u, 0.14f, Color(0.2f, 0.2f, 0.2f));
	k.hint(Vector3(-6.0f, 1.0f, 4.5f), Vector3(4.0f, 2.0f, 4.5f), "За стеклом консьержка. Жди, пока отвернётся к телевизору, и тихо проходи к лестнице (восточный конец холла)."_u);
	k.board(Vector3(2.5f, 1.5f, 1.88f), 0.0f, "Объявление УК"_u,
			"Уважаемые жильцы!\nВыход на кровлю и технический этаж закрыт. Ключи у консьержа и в диспетчерской.\nДверь на чердак оборудована охранной сигнализацией.\n\n[color=#a0a49f]Внизу ручкой: «Ага, а запасные в щитке на 12-м»[/color]"_u,
			false);

	Door *entrance = k.door(Vector3(-6.0f, 0.05f, ZW + 0.1f), 0.0f, Door::STYLE_METAL, 1.2f, 2.2f, "Дверь подъезда"_u);
	entrance->set_key("key_intercom");
	entrance->set_exit_button(true);

	ClimbPoint *lift_up = k.climb(Vector3(-1.2f, 1.1f, -0.6f), Vector3(1.4f, 2.0f, 0.5f), "Вызвать лифт и подняться на 12 этаж"_u, Vector3(-1.2f, FH * 11.0f + 0.15f, -0.2f), 0.0f, 4.0f);
	lift_up->set_noise(5.0f, "lift");
	lift_up->set_arrival_message("Лифт дзынькнул на двенадцатом. Надеюсь, консьержка не слышала"_u);
	ClimbPoint *lift_down = k.climb(Vector3(-1.2f, FH * 11.0f + 1.1f, -0.6f), Vector3(1.4f, 2.0f, 0.5f), "Вызвать лифт и спуститься на 1 этаж"_u, Vector3(-1.2f, 0.2f, -0.2f), 0.0f, 4.0f);
	lift_down->set_noise(5.0f, "lift");

	float top = FH * 11.0f;
	k.power_box(Vector3(2.5f, top + 1.5f, 1.8f), 0.0f, "uk", "электрощиток этажа (питание сигнализации кровли)"_u);
	b.visual_box(Vector3(2.5f, top + 1.05f, 1.75f), Vector3(0.5f, 0.04f, 0.3f), "metal_gray");
	k.pickup(Vector3(2.5f, top + 1.07f, 1.72f), "key_attic", "Ключи от чердака и люка"_u, "Связка на проволоке: дверь техэтажа и люк на кровлю"_u, false, "key");
	b.graffiti(Vector3(-4.0f, top + 1.7f, 1.88f), Vector3(0, 0, -1), "руферы, не шумите. соседи"_u, 0.18f);

	b.wall(Vector3(4.0f, 0, -ZW), Vector3(4.0f, 0, -1.0f), TECH, TECH_H, 0.2f, "concrete");
	b.wall(Vector3(4.0f, 0, -1.0f), Vector3(XW, 0, -1.0f), TECH, TECH_H, 0.2f, "concrete", { door_gap(3.0f, 1.3f, 2.2f) });
	b.wall(Vector3(4.0f, 0, -ZW + 0.05f), Vector3(XW, 0, -ZW + 0.05f), TECH, TECH_H, 0.1f, "concrete");
	b.wall(Vector3(XW - 0.05f, 0, -ZW), Vector3(XW - 0.05f, 0, -1.0f), TECH, TECH_H, 0.1f, "concrete");
	b.railing(Vector3(7.65f, 0, -2.45f), Vector3(9.8f, 0, -2.45f), TECH + 0.05f, 1.0f, "steel");
	b.wall(Vector3(-XW + 0.05f, 0, -ZW), Vector3(-XW + 0.05f, 0, ZW), TECH, TECH_H, 0.1f, "concrete");
	b.wall(Vector3(XW - 0.05f, 0, -1.0f), Vector3(XW - 0.05f, 0, ZW), TECH, TECH_H, 0.1f, "concrete");
	b.wall(Vector3(-XW, 0, ZW - 0.05f), Vector3(XW, 0, ZW - 0.05f), TECH, TECH_H, 0.1f, "concrete");
	b.wall(Vector3(-XW, 0, -ZW + 0.05f), Vector3(4.0f, 0, -ZW + 0.05f), TECH, TECH_H, 0.1f, "concrete");
	Door *attic = k.door(Vector3(7.0f, TECH + 0.05f, -1.0f), PI, Door::STYLE_METAL, 1.1f, 2.1f, "Дверь на технический этаж"_u);
	attic->set_key("key_attic");
	attic->set_reed(true, "uk", g.get_materials().get("plastic_white"));
	b.text(Vector3(7.0f, TECH + 2.25f, -1.14f), Vector3(0, 0, -1), "ВЫХОД НА КРОВЛЮ ЗАПРЕЩЁН"_u, 0.12f, Color(0.75f, 0.1f, 0.08f));
	k.hint(Vector3(7.0f, TECH + 1.0f, -2.0f), Vector3(4.0f, 2.0f, 2.0f), "Дверь на техэтаж: замок и геркон. Сначала магнит на датчик (подойди к белой коробочке над дверью), потом ключ."_u);
	k.shadow_zone(Vector3(0.0f, TECH + 1.2f, 0.0f), Vector3(2.0f * XW, TECH_H + 0.6f, 2.0f * ZW));

	for (int i = 0; i < 4; i++) {
		b.pipe(Vector3(-XW + 0.4f, TECH + 2.25f - float(i) * 0.1f, 1.0f + float(i) * 0.5f), Vector3(XW - 0.4f, TECH + 2.25f - float(i) * 0.1f, 1.0f + float(i) * 0.5f), 0.06f + float(i % 2) * 0.03f, i % 2 ? "rust" : "metal");
	}
	b.box(Vector3(-1.0f, TECH + 0.8f, -4.5f), Vector3(3.0f, 1.6f, 2.0f), "metal_gray");
	b.text(Vector3(-1.0f, TECH + 1.2f, -3.48f), Vector3(0, 0, 1), "ЛЕБЁДКА ЛИФТА"_u, 0.12f, Color(0.9f, 0.85f, 0.2f));
	b.box(Vector3(-7.5f, TECH + 0.6f, 5.0f), Vector3(2.0f, 1.2f, 1.5f), "metal");
	b.rubble(Vector3(3.0f, TECH + 0.05f, 4.0f), 1.0f, 8, "concrete");
	for (int i = 0; i < 6; i++) {
		b.visual_box(Vector3(-4.6f, TECH + 0.3f + float(i) * 0.4f, 2.5f), Vector3(0.5f, 0.04f, 0.04f), "steel");
	}
	b.visual_box(Vector3(-4.85f, TECH + 1.2f, 2.5f), Vector3(0.05f, 2.4f, 0.05f), "steel");
	b.visual_box(Vector3(-4.35f, TECH + 1.2f, 2.5f), Vector3(0.05f, 2.4f, 0.05f), "steel");
	Door *hatch = k.door(Vector3(-5.0f, ROOF - 0.02f, 2.5f), 0.0f, Door::STYLE_HATCH, 0.85f, 0.85f, "Люк на кровлю"_u);
	hatch->set_key("key_attic");
	ClimbPoint *roof_up = k.climb(Vector3(-4.6f, TECH + 0.6f, 2.5f), Vector3(0.6f, 1.0f, 0.9f), "Подняться по лестнице на крышу"_u, Vector3(-5.0f, ROOF + 0.15f, 3.6f), PI, 1.2f);
	roof_up->set_required_door(hatch, "Сначала открой люк над лестницей"_u);
	roof_up->set_noise(2.5f, "step_metal_3");
	roof_up->set_arrival_message("Крыша. Ветер и весь город под ногами"_u);
	ClimbPoint *roof_down = k.climb(Vector3(-5.0f, ROOF + 0.5f, 3.4f), Vector3(1.0f, 1.0f, 0.6f), "Спуститься в люк"_u, Vector3(-4.6f, TECH + 0.15f, 3.3f), 0.0f, 1.2f);
	roof_down->set_required_door(hatch, "Люк закрыт"_u);
	b.lamp_fixture(Vector3(0.0f, ROOF - 0.38f, 0.0f), false);

	float rp = ROOF;
	b.wall(Vector3(-XW - 0.2f, 0, -ZW - 0.2f), Vector3(XW + 0.2f, 0, -ZW - 0.2f), rp, 1.0f, 0.25f, "concrete");
	b.wall(Vector3(-XW - 0.2f, 0, ZW + 0.2f), Vector3(XW + 0.2f, 0, ZW + 0.2f), rp, 1.0f, 0.25f, "concrete");
	b.wall(Vector3(-XW - 0.2f, 0, -ZW - 0.2f), Vector3(-XW - 0.2f, 0, ZW + 0.2f), rp, 1.0f, 0.25f, "concrete");
	b.wall(Vector3(XW + 0.2f, 0, -ZW - 0.2f), Vector3(XW + 0.2f, 0, ZW + 0.2f), rp, 0.45f, 0.25f, "concrete");
	b.box(Vector3(-0.5f, rp + 1.25f, -3.5f), Vector3(4.6f, 2.5f, 4.0f), "brick");
	b.box(Vector3(-0.5f, rp + 2.6f, -3.5f), Vector3(5.0f, 0.2f, 4.4f), "roof");
	b.box(Vector3(6.0f, rp + 0.7f, 3.5f), Vector3(1.2f, 1.4f, 1.2f), "concrete");
	b.box(Vector3(-7.5f, rp + 0.7f, -4.5f), Vector3(1.2f, 1.4f, 1.2f), "concrete");
	b.pipe(Vector3(7.5f, rp, -5.5f), Vector3(7.5f, rp + 9.0f, -5.5f), 0.07f, "steel");
	for (float h : { 4.0f, 6.0f, 8.0f }) {
		b.pipe(Vector3(6.8f, rp + h, -5.5f), Vector3(8.2f, rp + h, -5.5f), 0.025f, "steel");
	}
	b.sphere(Vector3(7.5f, rp + 9.1f, -5.5f), 0.15f, "red_light");
	for (int i = 0; i < 3; i++) {
		b.visual_box(Vector3(-9.0f + float(i) * 1.4f, rp + 0.9f, 6.4f), Vector3(0.9f, 0.9f, 0.08f), "plastic_white", Basis(Vector3(1, 0, 0), -0.5f));
		b.pipe(Vector3(-9.0f + float(i) * 1.4f, rp, 6.5f), Vector3(-9.0f + float(i) * 1.4f, rp + 0.8f, 6.5f), 0.03f, "steel");
	}
	SecurityCamera *cam = k.camera(Vector3(-2.85f, rp + 2.2f, -1.45f), yaw_towards(Vector3(-2.85f, 0, -1.45f), Vector3(-5.0f, 0, 3.0f)), 30.0f, -28.0f, false, "uk", "Камера управляющей компании на кровле"_u);
	(void)cam;
	ActionPoint *sticker = k.action(Vector3(1.85f, rp + 1.3f, -2.0f), Vector3(0.4f, 0.8f, 1.2f), "Наклеить стикер команды"_u, [&g](UrbexPlayer *p) {
		g.play_sound("spray", p->get_global_position(), -6.0f);
		g.complete_objective("sticker");
	});
	sticker->set_done_prompt("Стикер на месте"_u);
	Label3D *tag = b.text(Vector3(1.86f, rp + 1.4f, -2.0f), Vector3(1, 0, 0), "URBEX TEAM", 0.22f, Color(0.95f, 0.55f, 0.15f));
	tag->set_visible(false);
	ActionPoint *sticker_ref = sticker;
	k.pickup(Vector3(7.8f, rp, -6.2f), "badge", "Значок руфера"_u, "Чей-то значок «Крыши наши» зацепился за антенну"_u, true, "key");
	k.hint(Vector3(0.0f, rp + 1.0f, 0.0f), Vector3(20.0f, 2.0f, 14.0f), "Крыша. На западе деловой центр, на востоке вот-вот встанет солнце. Подними камеру (ПКМ). На восточной стороне парапет по колено — не подходи к краю."_u);

	Vector3 city(-520.0f, 0.0f, 90.0f);
	Rng cr(5);
	float heights[] = { 374.0f, 300.0f, 268.0f, 240.0f, 210.0f, 190.0f, 160.0f, 130.0f };
	for (int i = 0; i < 8; i++) {
		Vector3 c = city + Vector3(cr.range(-50.0f, 50.0f), 0.0f, cr.range(-70.0f, 70.0f));
		float h = heights[i];
		float w = cr.range(28.0f, 42.0f);
		b.visual_box(c + Vector3(0, h * 0.5f, 0), Vector3(w, h, w), "facade", Basis(Vector3(0, 1, 0), cr.range(0.0f, 1.2f)));
		b.sphere(c + Vector3(0, h + 3.0f, 0), 3.0f, "red_light");
	}
	PhotoSpot *city_spot = k.spot(city + Vector3(0.0f, 220.0f, 0.0f), "city", "Панорама делового центра"_u, 900.0f);
	AABB roof_zone(Vector3(-XW - 0.5f, ROOF - 0.2f, -ZW - 0.5f), Vector3(2.0f * XW + 1.0f, 4.0f, 2.0f * ZW + 1.0f));
	city_spot->set_zone(roof_zone);
	PhotoSpot *sun_spot = k.spot(Vector3(1000.0f, ROOF + 1000.0f * std::tan(7.0f * DEG), 0.0f), "sunrise", "Рассвет над городом"_u, 1400.0f);
	sun_spot->set_zone(roof_zone);

	Rng br(11);
	for (int i = 0; i < 16; i++) {
		float a = float(i) / 16.0f * 2.0f * PI;
		float r = 110.0f + br.range(0.0f, 70.0f);
		Vector3 c(std::cos(a) * r, 0.0f, std::sin(a) * r);
		b.block_building(c, Vector3(br.range(16.0f, 22.0f), br.range(30.0f, 60.0f), br.range(16.0f, 40.0f)));
	}
	b.block_building(Vector3(-28.0f, 0.0f, 22.0f), Vector3(14.0f, 33.0f, 14.0f));
	b.block_building(Vector3(26.0f, 0.0f, 22.0f), Vector3(14.0f, 27.0f, 20.0f));

	b.fence(Vector3(-40.0f, 0, -35.0f), Vector3(40.0f, 0, -35.0f), 2.0f, "fence_gray", { Opening{ 40.0f, 1.6f, 0.0f, 2.0f }, Opening{ 64.0f, 5.0f, 0.0f, 2.0f } }, false);
	b.fence(Vector3(-40.0f, 0, 35.0f), Vector3(40.0f, 0, 35.0f), 2.0f, "fence_gray", {}, false);
	b.fence(Vector3(-40.0f, 0, -35.0f), Vector3(-40.0f, 0, 35.0f), 2.0f, "fence_gray", {}, false);
	b.fence(Vector3(40.0f, 0, -35.0f), Vector3(40.0f, 0, 35.0f), 2.0f, "fence_gray", {}, false);
	Door *barrier = k.door(Vector3(24.0f, 0.0f, -35.0f), 0.0f, Door::STYLE_GATE, 5.0f, 2.0f, "Автоматические ворота"_u);
	barrier->set_jammed(true, "Ворота открываются только с пульта"_u);
	b.text(Vector3(0.0f, 2.3f, -35.1f), Vector3(0, 0, -1), "ЖК «ЗАРЯ»"_u, 0.4f, Color(0.95f, 0.9f, 0.8f));

	const char *car_colors[] = { "car_red", "car_white", "car_black", "car_silver", "car_green" };
	Rng carr(8);
	for (int i = 0; i < 9; i++) {
		if (i == 4) {
			continue;
		}
		Vector3 c(18.0f + float(i % 3) * 3.0f, 0.0f, -22.0f + float(i / 3) * 6.0f);
		props::car(b, c, carr.range(-0.05f, 0.05f) + (i % 2 == 0 ? 0.0f : PI), car_colors[(i * 3) % 5], false);
	}
	props::bench(b, Vector3(-20.0f, 0.0f, -17.6f), 0.0f);
	props::bench(b, Vector3(-13.0f, 0.0f, -17.6f), 0.0f);
	props::trash_container(b, Vector3(-30.0f, 0.0f, -28.5f), 0.0f);
	props::trash_container(b, Vector3(-28.4f, 0.0f, -28.5f), 0.05f);
	props::swing(b, Vector3(-22.0f, 0.0f, -9.0f), 0.0f);
	props::slide(b, Vector3(-19.5f, 0.0f, -5.5f), PI * 0.5f);
	for (int i = 0; i < 4; i++) {
		float a = float(i) * PI * 0.5f;
		Vector3 c(-27.0f + std::sin(a) * 1.4f, 0.12f, -3.0f + std::cos(a) * 1.4f);
		b.visual_box(c, i % 2 == 0 ? Vector3(3.0f, 0.24f, 0.2f) : Vector3(0.2f, 0.24f, 3.0f), "wood");
	}
	b.visual_box(Vector3(-27.0f, 0.06f, -3.0f), Vector3(2.6f, 0.08f, 2.6f), "ground", Basis(), false);
	props::doormat(b, Vector3(-6.0f, 0.08f, ZW + 0.9f), 0.0f);
	props::flower_pot(b, Vector3(-7.7f, 0.08f, ZW + 0.55f));
	props::flower_pot(b, Vector3(-4.3f, 0.08f, ZW + 0.55f));
	facade_dressing(b);
	stair_dressing(b);
	k.beacon(Vector3(-4.8f, 3.25f, ZW + 1.9f), Color(1.0f, 0.12f, 0.08f), Color(1.0f, 0.12f, 0.08f), false, false);

	for (const Vector3 &p : { Vector3(-8.5f, ROOF, -6.2f), Vector3(9.0f, ROOF, 6.2f), Vector3(-9.0f, ROOF, 1.0f) }) {
		props::lightning_rod(b, p, 2.2f);
	}
	props::antenna(b, Vector3(3.0f, ROOF, 5.5f), 4.5f);
	props::antenna(b, Vector3(-1.5f, ROOF + 2.7f, -4.6f), 3.0f);
	for (const Vector3 &p : { Vector3(1.5f, ROOF, 6.3f), Vector3(-9.3f, ROOF, -2.0f) }) {
		b.pipe(p, p + Vector3(0.0f, 0.75f, 0.0f), 0.025f, "steel");
	}
	props::satellite_dish(b, Vector3(1.5f, ROOF + 0.75f, 6.3f), PI + 0.4f);
	props::satellite_dish(b, Vector3(-9.3f, ROOF + 0.75f, -2.0f), PI * 0.5f);
	k.steam(Vector3(6.0f, ROOF + 1.45f, 3.5f), Vector3(0.15f, 1.0f, 0.0f), 0.5f, false);
	k.steam(Vector3(-7.5f, ROOF + 1.45f, -4.5f), Vector3(0.15f, 1.0f, 0.0f), 0.4f, false);
	k.sound_loop(Vector3(6.0f, ROOF + 1.0f, 3.5f), "hum", -18.0f, 10.0f);

	props::tec_chimney(b, Vector3(320.0f, 0.0f, -210.0f), 180.0f, 8.0f);
	props::tec_chimney(b, Vector3(350.0f, 0.0f, -180.0f), 180.0f, 8.0f);
	props::tec_chimney(b, Vector3(-260.0f, 0.0f, -300.0f), 150.0f, 7.0f);
	fx::smoke_plume(b.dynamic_root, g.get_materials(), Vector3(320.0f, 181.0f, -210.0f), 5.0f, Vector3(-0.5f, 0.3f, 0.6f), Color(0.75f, 0.72f, 0.72f, 0.5f));
	fx::smoke_plume(b.dynamic_root, g.get_materials(), Vector3(350.0f, 181.0f, -180.0f), 5.0f, Vector3(-0.5f, 0.3f, 0.6f), Color(0.75f, 0.72f, 0.72f, 0.5f));
	fx::smoke_plume(b.dynamic_root, g.get_materials(), Vector3(-260.0f, 151.0f, -300.0f), 4.5f, Vector3(-0.5f, 0.3f, 0.6f), Color(0.7f, 0.68f, 0.68f, 0.45f));
	fx::birds(b.dynamic_root, g.get_materials(), Vector3(0.0f, 70.0f, 0.0f), Vector3(90.0f, 12.0f, 90.0f), Vector3(1.0f, 0.0f, 0.35f));
	fx::birds(b.dynamic_root, g.get_materials(), Vector3(-60.0f, 30.0f, 40.0f), Vector3(50.0f, 6.0f, 50.0f), Vector3(-0.4f, 0.0f, 1.0f));
	k.fog(Vector3(0.0f, 0.3f, -18.0f), Vector3(36.0f, 0.2f, 14.0f), 0.6f);
	k.fog(Vector3(0.0f, 0.3f, 24.0f), Vector3(36.0f, 0.2f, 9.0f), 0.6f);
	k.leaves(Vector3(-25.0f, 6.0f, 0.0f), Vector3(10.0f, 2.0f, 25.0f), 45);
	k.leaves(Vector3(25.0f, 6.0f, 15.0f), Vector3(10.0f, 2.0f, 15.0f), 30);
	Rng tr(21);
	for (int i = 0; i < 20; i++) {
		Vector3 p(tr.range(-37.0f, 37.0f), 0.0f, tr.range(-32.0f, 32.0f));
		if (std::fabs(p.x) < 14.0f && std::fabs(p.z) < 12.0f) {
			continue;
		}
		if (p.x > 14.0f && p.z < -8.0f) {
			continue;
		}
		if (std::fabs(p.x) < 4.0f && p.z < -10.0f) {
			continue;
		}
		if (std::fabs(p.x + 27.0f) < 3.0f && std::fabs(p.z + 3.0f) < 3.0f) {
			continue;
		}
		if (p.x < -12.0f && p.x > -25.0f && p.z > -12.0f && p.z < -3.0f) {
			continue;
		}
		if (tr.chance(0.65f)) {
			props::birch(b, p, tr.range(8.0f, 12.0f), uint32_t(i) * 11u + 5u);
		} else {
			b.tree(p, tr.range(6.0f, 9.0f));
		}
	}
	for (const Vector3 &p : { Vector3(-12.0f, 0.0f, 12.0f), Vector3(12.0f, 0.0f, 12.0f), Vector3(-2.5f, 0.0f, -14.0f), Vector3(14.0f, 0.0f, -10.0f), Vector3(-14.0f, 0.0f, -10.0f) }) {
		k.street_lamp(p, PI * 0.5f);
	}
	for (float x : { -30.0f, 0.0f, 30.0f }) {
		k.street_lamp(Vector3(x, 0.0f, -40.0f), 0.0f);
	}
	k.hint(Vector3(-6.0f, 1.0f, 10.0f), Vector3(6.0f, 2.0f, 5.0f), "Дверь на домофоне. Подожди жильца: когда он выйдет, проскочи внутрь, пока дверь не закрылась."_u);

	Guard *yard = k.guard(Guard::KIND_CHOP, "Охрана ЖК"_u, Vector3(-16.0f, 0.2f, -14.0f), false);
	yard->add_route_point(Vector3(-16.0f, 0.0f, -14.0f), 3.0f);
	yard->add_route_point(Vector3(-16.0f, 0.0f, 14.0f), 2.0f);
	yard->add_route_point(Vector3(16.0f, 0.0f, 14.0f), 3.0f);
	yard->add_route_point(Vector3(14.0f, 0.0f, -12.0f), 2.0f);
	yard->add_route_point(Vector3(0.0f, 0.0f, -30.0f), 5.0f);
	yard->set_vision(18.0f, 110.0f);

	Guard *concierge = k.guard(Guard::KIND_CONCIERGE, "Консьержка"_u, Vector3(-6.4f, 0.15f, -2.9f), false);
	concierge->set_post(Vector3(-6.4f, 0.15f, -2.9f), { yaw_towards(Vector3(-6.4f, 0, -2.9f), Vector3(-6.0f, 0, 6.0f)), yaw_towards(Vector3(-6.4f, 0, -2.9f), Vector3(-9.4f, 0, -4.0f)) }, 7.0f, true);
	concierge->set_initial_yaw(yaw_towards(Vector3(-6.4f, 0, -2.9f), Vector3(-6.0f, 0, 6.0f)));

	std::shared_ptr<Resident> resident = std::make_shared<Resident>();
	resident->node = memnew(Node3D);
	b.dynamic_root->add_child(resident->node);
	HumanoidLook look;
	look.outfit = HumanoidLook::OUTFIT_CIVIL;
	look.torso = "jacket_resident";
	look.legs = "jeans";
	look.cap = false;
	look.hair = true;
	look.bag = true;
	resident->rig.build(resident->node, g.get_materials(), look);
	resident->node->set_visible(false);

	d.gbr_search_points = { Vector3(0.0f, 0.1f, 0.5f), Vector3(7.0f, FH * 4.0f, 0.5f), Vector3(0.0f, FH * 8.0f, 0.5f), Vector3(2.0f, FH * 11.0f, 0.5f), Vector3(0.0f, TECH + 0.1f, 3.0f), Vector3(-6.0f, 0.0f, 12.0f), Vector3(0.0f, 0.0f, -20.0f) };

	UrbexGame *game = &g;
	d.tick = [game, resident, entrance, stair_lights, sticker_ref, tag](double delta) {
		float dt = float(delta);
		if (sticker_ref->is_used() && !tag->is_visible()) {
			tag->set_visible(true);
		}
		UrbexPlayer *p = game->get_player();
		if (p) {
			Vector3 pp = p->get_global_position();
			LevelData &ld = game->get_level();
			for (StairLight &sl : *stair_lights) {
				bool close_enough = std::fabs(pp.y - sl.position.y + 1.0f) < 2.2f && (Vector2(pp.x, pp.z) - Vector2(sl.position.x, sl.position.z)).length() < 4.5f;
				Vector3 v = p->get_real_velocity();
				if (close_enough && v.length() > 0.3f) {
					if (sl.timer <= 0.0f) {
						game->play_sound("click", sl.position + Vector3(0, 1.5f, 0), -12.0f);
					}
					sl.timer = 15.0f;
				}
				if (sl.timer > 0.0f) {
					sl.timer -= dt;
				}
				bool on = sl.timer > 0.0f;
				sl.light->set_visible(on);
				ld.lights[size_t(sl.probe)].enabled = on;
			}
		}

		Resident &r = *resident;
		r.phase_time += dt;
		Vector3 start(-6.0f, 0.05f, 0.8f);
		Vector3 door_in(-6.0f, 0.05f, 5.5f);
		Vector3 door_out(-6.0f, 0.05f, 9.0f);
		Vector3 away(-24.0f, 0.05f, 12.0f);
		auto walk = [&](const Vector3 &from, const Vector3 &to, float speed) {
			float len = (to - from).length();
			float t = clampf(r.phase_time * speed / std::max(0.01f, len), 0.0f, 1.0f);
			Vector3 pos = from.lerp(to, t);
			r.node->set_global_position(pos);
			r.node->set_rotation(Vector3(0.0f, yaw_towards(from, to), 0.0f));
			r.anim += dt * speed * 2.4f;
			r.rig.animate(r.anim, 1.0f);
			return t >= 1.0f;
		};
		switch (r.phase) {
			case 0:
				if (r.phase_time > r.timer) {
					r.phase = 1;
					r.phase_time = 0.0f;
					r.node->set_visible(true);
				}
				break;
			case 1:
				if (walk(start, door_in, 1.3f)) {
					r.phase = 2;
					r.phase_time = 0.0f;
					game->play_sound("beep_low", door_in, -4.0f);
					entrance->open_by_guard(door_in);
				}
				break;
			case 2:
				r.rig.animate(r.anim, 0.0f);
				if (r.phase_time > 0.8f) {
					r.phase = 3;
					r.phase_time = 0.0f;
				}
				break;
			case 3:
				if (walk(door_in, door_out, 1.3f)) {
					r.phase = 4;
					r.phase_time = 0.0f;
				}
				break;
			case 4:
				if (r.phase_time > 4.5f && entrance->is_open()) {
					entrance->close_door();
				}
				if (walk(door_out, away, 1.3f)) {
					r.phase = 0;
					r.phase_time = 0.0f;
					r.timer = 38.0f;
					r.node->set_visible(false);
					if (entrance->is_open()) {
						entrance->close_door();
					}
				}
				break;
		}
	};
}

}
