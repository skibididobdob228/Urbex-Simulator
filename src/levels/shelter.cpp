#include "levels/kit.h"
#include "levels/levels.h"

#include "actors/player.h"
#include "core/game.h"

using namespace godot;

namespace urbex {

namespace {

constexpr float UF = -6.0f;
constexpr float UC = -3.4f;
constexpr float UH = UC - UF;

void shelter_wall(LevelBuilder &b, const Vector3 &a, const Vector3 &c, float thickness, std::vector<Opening> openings = {}) {
	b.wall(a, c, UF, UH, thickness, "plaster", openings);
	std::vector<Opening> low;
	for (const Opening &o : openings) {
		low.push_back(Opening{ o.offset, o.width, o.bottom, 99.0f });
	}
	b.wall(a, c, UF, 1.5f, thickness + 0.04f, "paint_green", low, false);
}

void stencil(LevelBuilder &b, const Vector3 &pos, const Vector3 &normal, const String &text, float height = 0.28f) {
	b.text(pos, normal, text, height, Color(0.62f, 0.08f, 0.06f), true);
}

void shelter_dressing(Kit &k) {
	LevelBuilder &b = k.b;

	props::vent_duct(b, Vector3(-23.0f, UC - 0.32f, -0.6f), Vector3(13.6f, UC - 0.32f, -0.6f), 0.5f, 0.34f, UC);
	props::cable_tray(b, Vector3(-23.6f, UC - 0.12f, -1.85f), Vector3(13.6f, UC - 0.12f, -1.85f));
	b.pipe(Vector3(-23.8f, UC - 0.28f, -2.12f), Vector3(13.8f, UC - 0.28f, -2.12f), 0.06f, "metal_blue");
	b.pipe(Vector3(-23.8f, UC - 0.46f, -2.15f), Vector3(13.8f, UC - 0.46f, -2.15f), 0.04f, "rust");
	for (float x : { -21.0f, -12.5f, -0.5f, 8.5f }) {
		b.visual_box(Vector3(x, UC - 0.37f, -2.22f), Vector3(0.05f, 0.3f, 0.1f), "steel", Basis(), false);
	}
	for (float x : { -16.0f, -1.0f, 11.0f }) {
		props::valve(b, Vector3(x, UC - 0.28f, -1.98f), Vector3(0, 0, 1), 0.11f);
	}
	k.steam(Vector3(-6.4f, UC - 0.28f, -2.05f), Vector3(0.2f, -0.35f, 1.0f), 0.32f, true);
	k.drips(Vector3(-6.2f, UC - 0.35f, -1.95f), UH - 0.36f, 1.4f, true);
	k.drips(Vector3(-17.0f, UC - 0.05f, -1.5f), UH - 0.06f, 0.5f, true);
	k.drips(Vector3(7.8f, UC - 0.05f, -1.9f), UH - 0.06f, 0.4f, false);
	k.decal("decal_water", Vector3(-6.0f, UC - 0.01f, -1.4f), Vector3(0, -1, 0), Vector2(2.4f, 2.0f), 0.3f, 0.85f);
	k.decal("decal_water", Vector3(-17.0f, UC - 0.01f, -1.2f), Vector3(0, -1, 0), Vector2(1.8f, 1.6f), 1.2f, 0.7f);
	k.decal("decal_streak", Vector3(-6.4f, UF + 1.4f, -2.21f), Vector3(0, 0, 1), Vector2(1.0f, 2.2f), 0.0f, 0.7f);
	k.dust(Vector3(-5.0f, UF + 1.3f, -1.2f), Vector3(18.5f, 1.2f, 1.0f), 0.8f);
	k.dust(Vector3(-8.0f, UF + 1.3f, -6.0f), Vector3(5.8f, 1.2f, 3.3f), 0.7f);
	k.dust(Vector3(4.0f, UF + 1.3f, -6.0f), Vector3(5.8f, 1.2f, 3.3f), 0.7f);
	k.dust(Vector3(-3.0f, UF + 1.3f, 3.0f), Vector3(2.9f, 1.2f, 2.8f), 0.9f);

	props::fire_shield(b, Vector3(-21.6f, UF, -0.19f), 0.0f);
	props::extinguisher(b, Vector3(-13.3f, UF, -0.4f));
	props::extinguisher(b, Vector3(8.5f, UF, -0.4f));
	props::extinguisher(b, Vector3(13.5f, UF, -2.0f));
	props::wall_phone(b, Vector3(1.0f, UF + 1.5f, -0.24f), 0.0f);
	props::wheel_valve_door_frame(b, Vector3(6.7f, UF, 3.0f), 0.0f, 1.24f, 2.02f);
	props::wheel_valve_door_frame(b, Vector3(6.7f, UF, 0.0f), 0.0f, 1.24f, 2.02f);
	props::wheel_valve_door_frame(b, Vector3(-24.0f, UF, -1.2f), PI * 0.5f, 1.24f, 2.02f);

	props::water_tank(b, Vector3(8.6f, UF, -5.0f), PI * 0.5f);
	props::water_tank(b, Vector3(-12.75f, UF, -3.4f), 0.0f);
	props::lockers(b, Vector3(-2.37f, UF, -5.6f), PI * 0.5f, 3);
	props::table(b, Vector3(-4.6f, UF, -4.0f), 0.1f, 1.4f, 0.8f);
	props::chair(b, Vector3(-4.4f, UF, -3.2f), PI + 0.3f, false);
	props::chair(b, Vector3(-5.4f, UF, -4.9f), 0.4f, true);
	props::kettle(b, Vector3(-4.9f, UF + 0.78f, -3.9f));
	props::bottles(b, Vector3(-6.6f, UF, -3.4f), 3, 41);
	props::wall_clock(b, Vector3(-8.0f, UF + 2.2f, -9.22f), PI);

	props::crates(b, Vector3(-23.5f, UF, -7.2f), -PI * 0.5f, 6, "ГП-5"_u, 3);
	props::crates(b, Vector3(-15.0f, UF, -8.8f), PI, 4, "ИПП-8"_u, 9);
	props::lockers(b, Vector3(-14.35f, UF, -6.2f), PI * 0.5f, 2);

	props::cabinet(b, Vector3(13.6f, UF, -3.3f), PI * 0.5f, true);
	props::stretcher(b, Vector3(11.3f, UF, -5.2f), PI * 0.5f);
	props::sink(b, Vector3(12.68f, UF, 1.0f), PI * 0.5f);
	props::sink(b, Vector3(12.68f, UF, 2.4f), PI * 0.5f);
	k.flicker(Vector3(10.5f, UC - 0.08f, 2.0f), Color(0.85f, 0.95f, 1.0f), 6.0f, 0.9f, FxFlicker::MODE_FLUORESCENT, "shelter", true);
	k.drips(Vector3(9.0f, UC - 0.05f, 3.2f), UH - 0.06f, 0.8f, true);

	props::radio_station(b, Vector3(3.7f, UF + 0.8f, 4.75f), 0.0f);
	props::kettle(b, Vector3(1.3f, UF + 0.8f, 4.3f));
	props::chair(b, Vector3(2.4f, UF, 3.5f), PI, false);
	props::wall_clock(b, Vector3(4.6f, UF + 2.2f, 5.82f), 0.0f);
	props::cabinet(b, Vector3(0.65f, UF, 2.0f), -PI * 0.5f, false);

	k.sparks(Vector3(-13.65f, UF + 1.95f, 1.6f), "shelter", 3.5f, 9.0f);
	props::hanging_cable(b, Vector3(-13.7f, UF + 1.9f, 1.4f), Vector3(-12.0f, UC - 0.05f, 0.6f), 0.5f);
	props::barrel(b, Vector3(-13.3f, UF, 5.3f), "metal_blue", false);
	props::barrel(b, Vector3(-7.0f, UF, 5.3f), "rust", true);
	props::puddle(b, Vector3(-10.0f, UF, 1.6f), 0.9f, 61u);
	k.decal("decal_soot", Vector3(-11.5f, UC - 0.01f, 3.2f), Vector3(0, -1, 0), Vector2(1.6f, 1.6f), 0.0f, 0.8f);

	k.flicker(Vector3(-31.0f, UC - 0.1f, -1.2f), Color(0.85f, 0.95f, 1.0f), 6.0f, 0.8f, FxFlicker::MODE_BROKEN, "shelter", true);
	k.fog(Vector3(-32.0f, UF + 0.12f, -1.2f), Vector3(7.5f, 0.06f, 0.6f), 3.0f);
	k.drips(Vector3(-28.0f, UC - 0.05f, -1.6f), UH - 0.06f, 1.0f, false);
	k.drips(Vector3(-35.6f, UC - 0.05f, -0.9f), UH - 0.06f, 1.6f, false);
	k.drips(Vector3(-41.4f, -0.55f, -1.6f), -0.55f - UF, 0.8f, false);
	k.sound_loop(Vector3(-34.0f, UF + 1.0f, -1.2f), "drip", -8.0f, 14.0f);
	props::hanging_cable(b, Vector3(-25.0f, UC - 0.05f, -0.5f), Vector3(-30.0f, UC - 0.05f, -0.5f), 0.7f);
	props::hanging_cable(b, Vector3(-33.0f, UC - 0.05f, -1.9f), Vector3(-38.0f, UC - 0.05f, -1.7f), 1.0f);
	k.decal("decal_streak", Vector3(-30.0f, UF + 1.3f, -1.96f), Vector3(0, 0, 1), Vector2(1.2f, 2.4f), 0.0f, 0.8f);
	k.decal("decal_streak", Vector3(-36.5f, UF + 1.3f, -0.44f), Vector3(0, 0, -1), Vector2(1.2f, 2.4f), 0.0f, 0.8f);

	props::truck(b, Vector3(27.0f, 0.0f, -14.0f), 0.2f);
	props::shipping_container(b, Vector3(22.0f, 0.0f, 12.0f), 0.0f, "rust");
	props::shipping_container(b, Vector3(25.0f, 0.0f, 12.6f), 0.04f, "metal_blue");
	props::shipping_container(b, Vector3(-10.0f, 0.0f, -16.0f), PI * 0.5f, "rust");
	props::pallets(b, Vector3(22.5f, 0.0f, -8.5f), 0.4f, 5);
	props::pallets(b, Vector3(-6.0f, 0.0f, -16.4f), 1.2f, 3);
	props::barrel(b, Vector3(-7.4f, 0.0f, -14.2f), "rust", false);
	props::barrel(b, Vector3(-6.7f, 0.0f, -14.0f), "metal_blue", false);
	props::trash_container(b, Vector3(-2.0f, 0.0f, 7.5f), 0.0f);
	props::bench(b, Vector3(-8.0f, 0.0f, 7.8f), 0.0f);
	props::drain_pipe(b, Vector3(-12.2f, 9.9f, 8.85f), 9.9f);
	props::drain_pipe(b, Vector3(-41.8f, 9.9f, 8.85f), 9.9f);
	for (int i = 0; i < 6; i++) {
		props::window_frame(b, Vector3(-40.0f + float(i) * 5.0f, 5.5f, 8.94f), 0.0f, 2.4f, 3.0f, 0.55f);
	}
	props::tec_chimney(b, Vector3(-34.0f, 0.0f, 36.0f), 46.0f, 2.6f);
	fx::smoke_plume(b.dynamic_root, k.g.get_materials(), Vector3(-34.0f, 46.5f, 36.0f), 1.6f, Vector3(0.35f, 0.15f, -0.1f), Color(0.5f, 0.5f, 0.52f, 0.5f));
	props::tec_chimney(b, Vector3(150.0f, 0.0f, 110.0f), 120.0f, 6.0f);
	fx::smoke_plume(b.dynamic_root, k.g.get_materials(), Vector3(150.0f, 121.0f, 110.0f), 3.5f, Vector3(0.6f, 0.2f, -0.2f), Color(0.55f, 0.55f, 0.58f, 0.45f));
	k.beacon(Vector3(16.5f, 3.0f, -25.0f), Color(1.0f, 0.5f, 0.05f), Color(1.0f, 0.5f, 0.05f), false, false);
	k.beacon(Vector3(7.6f, 3.3f, 8.4f), Color(1.0f, 0.12f, 0.08f), Color(1.0f, 0.12f, 0.08f), false, false);
	k.flicker(Vector3(5.0f, 2.85f, 7.75f), Color(1.0f, 0.85f, 0.6f), 6.0f, 0.9f, FxFlicker::MODE_BROKEN, "plant", true);
	k.fog(Vector3(-40.0f, 0.2f, -8.0f), Vector3(8.0f, 0.12f, 18.0f), 0.7f);
	k.fog(Vector3(10.0f, 0.2f, 22.0f), Vector3(22.0f, 0.12f, 6.0f), 0.6f);
	k.leaves(Vector3(-40.0f, 6.0f, -10.0f), Vector3(8.0f, 2.0f, 16.0f), 50);
}

}

void build_shelter(LevelBuilder &b, LevelData &d, UrbexGame &g) {
	Kit k(b, d, g);
	d.id = "shelter";
	d.title = "Бомбоубежище ГО под заводом"_u;
	d.subtitle = "Защитное сооружение гражданской обороны №14"_u;
	d.briefing = "[b]Объект:[/b] убежище ГО под территорией приборостроительного завода. Числится на балансе, поэтому охраняется: вахтёр на КПП, обходчик по двору и дежурный внутри.\n\n"_u
				 "[b]Два пути внутрь.[/b] Основной — через павильон входа во дворе: две гермодвери и тамбур-шлюз, но на внешней гермодвери геркон, а сами гермы открываются громко. Второй — оголовок аварийного выхода в западном углу двора: навесной замок, у тебя есть болторез.\n\n"_u
				 "[b]Нужно:[/b] снять гермодверь, ФВУ в фильтровентиляционной камере и плакат ГО в отсеке, забрать противогаз ГП-5.\n\n"_u
				 "[b]Подсказка:[/b] свет и датчики внутри питаются от главного щита в дизельной. Обесточишь — станет темно, но дежурный пойдёт проверять щиток.\n\n"_u
				 "Уходить — через пролом в бетонном заборе на юге."_u;
	d.exit_hint = "через пролом в южном заборе"_u;
	d.spawn_position = Vector3(-20.0f, 0.1f, -37.0f);
	d.spawn_yaw = PI;
	d.exit_zone = AABB(Vector3(-70.0f, -1.0f, -60.0f), Vector3(120.0f, 8.0f, 28.5f));
	d.ambient_exposure = 0.22f;
	d.indoor_exposure = 0.05f;
	d.kill_height = -14.0f;
	d.gbr_delay = 60.0f;
	d.gbr_spawn = Vector3(11.0f, 0.1f, -27.0f);
	d.gbr_van = true;
	d.gbr_van_position = Vector3(14.0f, 0.05f, -36.0f);
	d.gbr_van_yaw = PI * 0.5f;
	d.ambience = Ambience::Underground;
	d.nav_bounds = AABB(Vector3(-52.0f, -7.0f, -32.0f), Vector3(90.0f, 11.0f, 64.0f));
	d.start_items.push_back("boltcutter");
	d.legal_note = "Защитные сооружения ГО до сих пор стоят на балансе предприятий и городов. За проникновение на охраняемый объект — ст. 20.17 КоАП РФ, а если объект режимный — ответственность намного строже."_u;

	k.objective_photo("blast_door", "Сфоткать гермодверь"_u);
	k.objective_photo("fvu", "Сфоткать ФВУ в фильтровентиляционной камере"_u);
	k.objective_photo("poster", "Сфоткать плакат ГО в отсеке"_u);
	k.objective_item("gasmask", "Забрать противогаз ГП-5"_u);
	k.objective_item("journal", "Найти журнал дежурств"_u, true);
	k.objective_action("crank", "Покрутить ручной привод вентилятора"_u, true);

	b.slab(-140.0f, -140.0f, 140.0f, 140.0f, 0.0f, 0.5f, "ground", { Rect2(2.3f, 9.4f, 1.7f, 4.6f) });
	b.box(Vector3(0.0f, 0.02f, -42.0f), Vector3(280.0f, 0.06f, 10.0f), "asphalt");
	b.box(Vector3(10.0f, 0.02f, -10.0f), Vector3(8.0f, 0.05f, 40.0f), "asphalt");

	b.slab(-42.6f, -9.7f, 14.3f, 16.7f, UF, 0.3f, "concrete_floor");
	b.slab(-42.6f, -9.7f, 14.3f, 9.2f, UC + 0.3f, 0.3f, "concrete", { Rect2(-42.4f, -2.4f, 2.4f, 2.4f) });

	b.wall(Vector3(2.2f, 0, 9.2f), Vector3(2.2f, 0, 16.6f), UF, -UF - 0.5f, 0.3f, "concrete");
	b.wall(Vector3(7.85f, 0, 3.0f), Vector3(7.85f, 0, 16.6f), UF, -UF - 0.5f, 0.3f, "concrete");
	b.wall(Vector3(2.2f, 0, 16.5f), Vector3(7.85f, 0, 16.5f), UF, -UF - 0.5f, 0.3f, "concrete");
	b.wall(Vector3(2.2f, 0, 9.2f), Vector3(5.4f, 0, 9.2f), UF, -UF - 0.6f, 0.3f, "concrete");
	b.box(Vector3(4.975f, UF * 0.5f - 0.25f, 12.0f), Vector3(2.05f, -UF - 0.5f, 5.2f), "concrete");
	b.ramp(Vector3(3.2f, -3.0f, 14.6f), Vector3(3.2f, 0.0f, 9.4f), 1.5f, "concrete_floor", true);
	b.box(Vector3(5.0f, -3.125f, 15.45f), Vector3(5.5f, 0.25f, 1.7f), "concrete_floor");
	b.ramp(Vector3(6.75f, UF, 9.4f), Vector3(6.75f, -3.0f, 14.6f), 1.5f, "concrete_floor", true);
	b.railing(Vector3(4.08f, 0, 9.4f), Vector3(4.08f, 0, 14.0f), 0.0f, 1.0f, "steel");
	b.railing(Vector3(2.3f, 0, 14.05f), Vector3(4.08f, 0, 14.05f), 0.0f, 1.0f, "steel");
	b.wall(Vector3(5.35f, 0, 3.0f), Vector3(5.35f, 0, 9.2f), UF, UH, 0.25f, "concrete");

	Vector3 pav(5.0f, 0.0f, 12.3f);
	b.wall(Vector3(2.0f, 0, 8.0f), Vector3(8.0f, 0, 8.0f), 0.0f, 3.0f, 0.25f, "brick", { door_gap(3.0f, 1.2f, 2.2f) });
	b.wall(Vector3(2.0f, 0, 8.0f), Vector3(2.0f, 0, 16.7f), 0.0f, 3.0f, 0.25f, "brick");
	b.wall(Vector3(8.0f, 0, 8.0f), Vector3(8.0f, 0, 16.7f), 0.0f, 3.0f, 0.25f, "brick", { window_gap(5.0f, 1.0f, 1.4f, 2.4f) });
	b.wall(Vector3(2.0f, 0, 16.7f), Vector3(8.0f, 0, 16.7f), 0.0f, 3.0f, 0.25f, "brick");
	b.box(Vector3(5.0f, 3.1f, 12.35f), Vector3(6.6f, 0.2f, 9.3f), "roof");
	b.box(Vector3(5.0f, 0.03f, 8.7f), Vector3(5.7f, 0.06f, 1.3f), "concrete_floor");
	k.shadow_zone(Vector3(5.0f, 1.5f, 12.3f), Vector3(6.0f, 3.0f, 8.7f));
	b.text(Vector3(5.0f, 2.6f, 7.86f), Vector3(0, 0, -1), "УБЕЖИЩЕ № 14"_u, 0.3f, Color(0.92f, 0.9f, 0.82f));
	Door *pav_door = k.door(Vector3(5.0f, 0.0f, 8.0f), 0.0f, Door::STYLE_METAL, 1.1f, 2.1f, "Павильон входа в убежище"_u);
	(void)pav_door;
	k.camera(Vector3(7.7f, 2.8f, 7.75f), yaw_towards(Vector3(7.7f, 0, 7.75f), Vector3(4.0f, 0, 2.0f)), 25.0f, -24.0f, false, "plant", "Камера над павильоном"_u);
	k.board(Vector3(1.75f, 1.5f, 6.5f), PI * 0.5f, "Убежище № 14"_u,
			"Защитное сооружение гражданской обороны.\nВместимость: 300 человек.\nОтветственный: начальник ГО завода.\n\nПри сигнале «Внимание всем!» включить радио и слушать сообщение штаба ГО.\n\n[color=#a0a49f]Табличка висит с восьмидесятых. Замок на павильоне новый, а на гермодвери кто-то поставил геркон.[/color]"_u,
			false);
	k.hint(Vector3(5.0f, 1.0f, 5.0f), Vector3(5.0f, 2.0f, 4.0f), "Павильон основного входа. Над дверью камера: держись в тени у стены."_u);

	b.wall(Vector3(5.35f, 0, 0.0f), Vector3(5.35f, 0, 3.0f), UF, UH, 0.25f, "concrete");
	shelter_wall(b, Vector3(5.4f, 0, 3.0f), Vector3(7.85f, 0, 3.0f), 0.5f, { door_gap(1.3f, 1.3f, 2.1f) });
	Door *blast1 = k.door(Vector3(6.7f, UF, 3.0f), PI, Door::STYLE_BLAST, 1.2f, 2.0f, "Защитно-герметическая дверь"_u);
	blast1->set_reed(true, "shelter", g.get_materials().get("plastic_white"));
	shelter_wall(b, Vector3(-24.0f, 0, 0.0f), Vector3(14.0f, 0, 0.0f), 0.3f, { door_gap(30.7f, 1.3f, 2.1f), door_gap(21.0f, 1.0f, 2.1f), door_gap(14.0f, 1.0f, 2.1f), door_gap(26.5f, 1.0f, 2.1f), door_gap(34.5f, 1.0f, 2.1f) });
	Door *blast2 = k.door(Vector3(6.7f, UF, 0.0f), PI, Door::STYLE_BLAST, 1.2f, 2.0f, "Герметическая дверь"_u);
	(void)blast2;
	stencil(b, Vector3(6.7f, UF + 2.35f, -0.17f), Vector3(0, 0, -1), "ВХОД"_u);
	k.spot(Vector3(6.7f, UF + 1.1f, -0.35f), "blast_door", "Гермодверь"_u, 10.0f);
	k.pir(Vector3(0.0f, UC - 0.15f, -2.2f), Vector3(8.0f, UF + 0.5f, -1.0f), 9.0f, "shelter", false, "Датчик движения у входа"_u);

	shelter_wall(b, Vector3(-24.0f, 0, -2.4f), Vector3(14.0f, 0, -2.4f), 0.3f, { door_gap(16.0f, 1.0f, 2.1f), door_gap(28.0f, 1.0f, 2.1f), door_gap(5.0f, 1.0f, 2.1f), door_gap(36.0f, 0.9f, 2.1f) });
	shelter_wall(b, Vector3(14.0f, 0, -9.4f), Vector3(14.0f, 0, 0.0f), 0.3f);
	shelter_wall(b, Vector3(-24.0f, 0, -9.4f), Vector3(14.0f, 0, -9.4f), 0.3f);
	shelter_wall(b, Vector3(-24.0f, 0, -9.4f), Vector3(-24.0f, 0, 0.0f), 0.3f, { door_gap(8.2f, 1.3f, 2.1f) });
	shelter_wall(b, Vector3(-14.0f, 0, -9.4f), Vector3(-14.0f, 0, -2.4f), 0.2f);
	shelter_wall(b, Vector3(-2.0f, 0, -9.4f), Vector3(-2.0f, 0, -2.4f), 0.2f);
	shelter_wall(b, Vector3(10.0f, 0, -9.4f), Vector3(10.0f, 0, -2.4f), 0.2f);
	shelter_wall(b, Vector3(10.0f, 0, -6.0f), Vector3(14.0f, 0, -6.0f), 0.2f);

	shelter_wall(b, Vector3(-14.0f, 0, 6.0f), Vector3(5.3f, 0, 6.0f), 0.3f);
	shelter_wall(b, Vector3(-14.0f, 0, 0.0f), Vector3(-14.0f, 0, 6.0f), 0.3f);
	shelter_wall(b, Vector3(-6.0f, 0, 0.0f), Vector3(-6.0f, 0, 6.0f), 0.2f);
	shelter_wall(b, Vector3(0.2f, 0, 0.0f), Vector3(0.2f, 0, 6.0f), 0.2f);
	shelter_wall(b, Vector3(5.3f, 0, 3.0f), Vector3(5.3f, 0, 6.0f), 0.2f);
	b.wall(Vector3(8.0f, 0, 0.0f), Vector3(8.0f, 0, 4.0f), UF, UH, 0.2f, "tile");
	b.wall(Vector3(13.0f, 0, 0.0f), Vector3(13.0f, 0, 4.0f), UF, UH, 0.2f, "tile");
	b.wall(Vector3(8.0f, 0, 4.0f), Vector3(13.0f, 0, 4.0f), UF, UH, 0.2f, "tile");
	b.box(Vector3(10.5f, UF + 0.02f, 2.0f), Vector3(5.0f, 0.04f, 4.0f), "tile", false);

	Door *fvk_door = k.door(Vector3(-3.0f, UF, 0.0f), 0.0f, Door::STYLE_METAL, 0.95f, 2.05f, "Фильтровентиляционная камера"_u);
	Door *des_door = k.door(Vector3(-10.0f, UF, 0.0f), 0.0f, Door::STYLE_METAL, 0.95f, 2.05f, "Дизельная электростанция"_u);
	Door *pu_door = k.door(Vector3(2.5f, UF, 0.0f), 0.0f, Door::STYLE_WOOD, 0.95f, 2.05f, "Пункт управления"_u);
	Door *wc_door = k.door(Vector3(10.5f, UF, 0.0f), 0.0f, Door::STYLE_WOOD, 0.95f, 2.05f, "Санузел"_u);
	Door *otsek1 = k.door(Vector3(-8.0f, UF, -2.4f), 0.0f, Door::STYLE_METAL, 0.95f, 2.05f, "Отсек № 1"_u);
	Door *otsek2 = k.door(Vector3(4.0f, UF, -2.4f), 0.0f, Door::STYLE_METAL, 0.95f, 2.05f, "Отсек № 2"_u);
	Door *sklad = k.door(Vector3(-19.0f, UF, -2.4f), 0.0f, Door::STYLE_WOOD, 0.95f, 2.05f, "Склад"_u);
	(void)fvk_door;
	(void)des_door;
	(void)pu_door;
	(void)wc_door;
	(void)otsek1;
	(void)otsek2;
	(void)sklad;
	stencil(b, Vector3(-3.0f, UF + 2.35f, -0.17f), Vector3(0, 0, -1), "ФВК"_u);
	stencil(b, Vector3(-10.0f, UF + 2.35f, -0.17f), Vector3(0, 0, -1), "ДЭС"_u);
	stencil(b, Vector3(2.5f, UF + 2.35f, -0.17f), Vector3(0, 0, -1), "ПУ"_u);
	stencil(b, Vector3(-8.0f, UF + 2.35f, -2.23f), Vector3(0, 0, 1), "ОТСЕК 1"_u);
	stencil(b, Vector3(4.0f, UF + 2.35f, -2.23f), Vector3(0, 0, 1), "ОТСЕК 2"_u);
	stencil(b, Vector3(-19.0f, UF + 2.35f, -2.23f), Vector3(0, 0, 1), "СКЛАД"_u);
	stencil(b, Vector3(-23.83f, UF + 2.35f, -1.2f), Vector3(1, 0, 0), "АВАРИЙНЫЙ ВЫХОД"_u, 0.22f);
	stencil(b, Vector3(-18.0f, UF + 1.9f, -0.17f), Vector3(0, 0, -1), "НЕ КУРИТЬ!"_u, 0.25f);

	for (float x : { -20.0f, -14.0f, -8.0f, -2.0f, 4.0f, 10.0f }) {
		k.ceiling_lamp(Vector3(x, UC - 0.5f, -1.2f), Color(1.0f, 0.8f, 0.55f), 7.0f, 0.9f, "shelter");
		b.pipe(Vector3(x, UC - 0.44f, -1.2f), Vector3(x, UC, -1.2f), 0.012f, "black");
	}
	for (const Vector3 &p : { Vector3(-8.0f, UC - 0.06f, -6.0f), Vector3(4.0f, UC - 0.06f, -6.0f), Vector3(-3.0f, UC - 0.06f, 3.0f), Vector3(-10.0f, UC - 0.06f, 3.0f), Vector3(6.6f, UC - 0.06f, 1.5f), Vector3(6.6f, UC - 0.06f, 6.0f) }) {
		k.ceiling_lamp(p, Color(1.0f, 0.8f, 0.55f), 7.0f, 0.8f, "shelter");
	}
	for (const Vector3 &p : { Vector3(-19.0f, UC - 0.06f, -6.0f), Vector3(2.7f, UC - 0.06f, 3.0f) }) {
		b.lamp_fixture(p + Vector3(0, 0.06f, 0), false);
	}

	k.pir(Vector3(-23.6f, UC - 0.15f, -0.3f), Vector3(-10.0f, UF + 0.6f, -1.2f), 9.0f, "shelter", true, "Старый датчик"_u);
	k.pir(Vector3(9.6f, UC - 0.15f, -2.8f), Vector3(2.0f, UF + 0.6f, -8.0f), 9.0f, "shelter", false, "Датчик движения во втором отсеке"_u);

	for (int i = 0; i < 4; i++) {
		b.bunk_bed(Vector3(-12.0f + float(i) * 2.6f, UF, -8.6f), 0.0f, 2.0f);
		b.bunk_bed(Vector3(0.0f + float(i) * 2.4f, UF, -8.6f), 0.0f, 2.0f);
	}
	b.bunk_bed(Vector3(-13.2f, UF, -5.0f), PI * 0.5f, 1.9f);
	for (int i = 0; i < 6; i++) {
		b.sphere(Vector3(-11.8f + float(i) * 1.2f, UF + 0.55f, -8.5f), 0.09f, "rubber");
	}
	b.poster(Vector3(-5.0f, UF + 1.75f, -9.24f), Vector3(0, 0, 1), 1.3f, 1.7f, "ГРАЖДАНСКАЯ ОБОРОНА"_u,
			"Правила поведения в убежище: соблюдать тишину, не курить, не зажигать свечи и керосиновые лампы без разрешения. Выполнять указания коменданта убежища. При неисправности фильтровентиляции надеть противогаз."_u,
			Color(0.6f, 0.08f, 0.06f));
	k.spot(Vector3(-5.0f, UF + 1.75f, -8.9f), "poster", "Плакат ГО «Правила поведения в убежище»"_u, 8.0f);
	b.poster(Vector3(-11.0f, UF + 1.75f, -9.24f), Vector3(0, 0, 1), 1.0f, 1.3f, "СРЕДСТВА ЗАЩИТЫ"_u, "Противогаз ГП-5. Порядок надевания: задержать дыхание, закрыть глаза, надеть шлем-маску, сделать резкий выдох."_u, Color(0.6f, 0.08f, 0.06f));
	b.poster(Vector3(7.0f, UF + 1.75f, -9.24f), Vector3(0, 0, 1), 1.0f, 1.3f, "СИГНАЛЫ ГО"_u, "«Внимание всем!» — сирены и прерывистые гудки. Включи радио и телевизор, слушай сообщение."_u, Color(0.6f, 0.08f, 0.06f));
	k.pickup(Vector3(4.6f, UF + 1.36f, -8.6f), "gasmask", "Противогаз ГП-5"_u, "Резиновая шлем-маска, фильтрующе-поглощающая коробка и брезентовая сумка"_u, false, "gasmask");
	k.hint(Vector3(-8.0f, UF + 1.0f, -5.0f), Vector3(10.0f, 2.0f, 6.0f), "Отсек для укрываемых: двухъярусные нары, на стенах плакаты ГО."_u);

	b.box(Vector3(-3.2f, UF + 0.6f, 3.6f), Vector3(2.0f, 1.2f, 1.0f), "metal");
	b.cylinder(Vector3(-1.4f, UF, 3.6f), 0.45f, 1.3f, "metal", true, 14);
	for (int i = 0; i < 3; i++) {
		b.cylinder(Vector3(-5.5f + float(i) * 0.75f, UF, 1.3f), 0.33f, 1.05f, "metal_gray", true, 12);
		b.text(Vector3(-5.5f + float(i) * 0.75f, UF + 0.75f, 0.96f), Vector3(0, 0, -1), "ФП-100"_u, 0.09f, Color(0.9f, 0.9f, 0.85f));
	}
	b.pipe(Vector3(-1.4f, UF + 1.3f, 3.6f), Vector3(-1.4f, UC - 0.3f, 3.6f), 0.18f, "metal");
	b.pipe(Vector3(-1.4f, UC - 0.3f, 3.6f), Vector3(-5.8f, UC - 0.3f, 3.6f), 0.18f, "metal");
	b.pipe(Vector3(-5.5f, UF + 1.05f, 1.3f), Vector3(-5.5f, UC - 0.3f, 1.3f), 0.1f, "metal");
	b.text(Vector3(-3.2f, UF + 0.9f, 3.09f), Vector3(0, 0, -1), "ФВК-1"_u, 0.18f, Color(0.9f, 0.9f, 0.85f));
	k.spot(Vector3(-3.0f, UF + 0.9f, 3.0f), "fvu", "ФВУ: фильтровентиляционный агрегат"_u, 7.0f);
	ActionPoint *crank = k.action(Vector3(-1.4f, UF + 0.8f, 3.05f), Vector3(0.6f, 0.6f, 0.4f), "Покрутить ручной привод вентилятора (шумно)"_u, [&g](UrbexPlayer *p) {
		g.play_sound("creak_low", p->get_global_position(), 2.0f, 0.7f);
		p->make_noise(8.0f);
		g.complete_objective("crank");
		g.notify("Вентилятор тяжело провернулся. Режим II — фильтровентиляция: воздух идёт через фильтры-поглотители"_u, Color(0.8f, 0.85f, 0.95f));
	});
	crank->set_done_prompt("Ручной привод вентилятора"_u);
	k.board(Vector3(-5.85f, UF + 1.6f, 4.6f), -PI * 0.5f, "Режимы вентиляции убежища"_u,
			"[b]Режим I — чистая вентиляция.[/b] Наружный воздух очищается от пыли и подаётся в убежище.\n\n[b]Режим II — фильтровентиляция.[/b] Воздух проходит через фильтры-поглотители ФП-100 и очищается от отравляющих веществ, радиоактивной пыли и бактериальных аэрозолей.\n\n[b]Режим III — полная изоляция.[/b] Убежище герметизируется, воздух регенерируется внутри.\n\n[color=#a0a49f]Комплект ФВК-1 рассчитан на убежище вместимостью до 150 человек. Вентилятор электроручной: если нет электричества, его крутят вручную.[/color]"_u,
			false);

	b.box(Vector3(-10.5f, UF + 0.7f, 3.2f), Vector3(2.8f, 1.4f, 1.2f), "metal");
	b.cylinder(Vector3(-12.6f, UF, 4.8f), 0.5f, 1.0f, "rust", true, 12);
	b.pipe(Vector3(-11.5f, UF + 1.4f, 3.2f), Vector3(-11.5f, UC, 3.2f), 0.09f, "rust");
	b.text(Vector3(-10.5f, UF + 1.0f, 2.59f), Vector3(0, 0, -1), "ДГ-50"_u, 0.18f, Color(0.9f, 0.9f, 0.85f));
	k.power_box(Vector3(-13.82f, UF + 1.4f, 1.6f), -PI * 0.5f, "shelter", "главный щит убежища (свет, датчики, геркон)"_u);

	b.box(Vector3(2.6f, UF + 0.4f, 4.6f), Vector3(3.0f, 0.8f, 1.0f), "wood");
	b.visual_box(Vector3(1.8f, UF + 0.95f, 4.6f), Vector3(0.5f, 0.3f, 0.4f), "metal_gray");
	b.visual_box(Vector3(3.2f, UF + 0.86f, 4.5f), Vector3(0.25f, 0.12f, 0.2f), "plastic_dark");
	b.poster(Vector3(2.7f, UF + 1.7f, 5.84f), Vector3(0, 0, -1), 1.6f, 1.0f, "СХЕМА ОПОВЕЩЕНИЯ"_u, "Штаб ГО завода — коммутатор — дежурный по убежищу."_u, Color(0.6f, 0.08f, 0.06f));
	k.pickup(Vector3(2.0f, UF + 0.81f, 4.4f), "journal", "Журнал дежурств"_u, "Журнал дежурств по убежищу. Последняя запись: «Проверка ФВУ — норма. 14.06.1983»"_u, true, "paper");

	for (int i = 0; i < 3; i++) {
		b.cylinder(Vector3(-22.6f + float(i) * 1.4f, UF, -8.4f), 0.55f, 1.6f, "metal_gray", true, 14);
	}
	for (int i = 0; i < 5; i++) {
		b.box(Vector3(-16.0f - float(i % 3) * 1.1f, UF + 0.35f + float(i / 3) * 0.7f, -4.0f), Vector3(0.9f, 0.7f, 0.7f), "wood", i < 3);
	}
	k.pickup(Vector3(-21.0f, UF, -4.0f), "batteries", "Батарейки"_u, "Пачка батареек со склада. Фонарь снова свежий"_u, false, "batteries");

	b.box(Vector3(12.0f, UF + 0.4f, -4.0f), Vector3(1.8f, 0.8f, 0.7f), "plastic_white");
	b.text(Vector3(12.0f, UF + 1.6f, -5.83f), Vector3(0, 0, 1), "МЕДПУНКТ"_u, 0.22f, Color(0.62f, 0.08f, 0.06f));

	Door *emergency = k.door(Vector3(-24.0f, UF, -1.2f), PI * 0.5f, Door::STYLE_BLAST, 1.2f, 2.0f, "Дверь аварийного выхода"_u);
	emergency->open_instantly();
	b.wall(Vector3(-40.2f, 0, -2.1f), Vector3(-24.0f, 0, -2.1f), UF, UH, 0.25f, "concrete");
	b.wall(Vector3(-40.2f, 0, -0.3f), Vector3(-24.0f, 0, -0.3f), UF, UH, 0.25f, "concrete");
	b.wall(Vector3(-42.5f, 0, -2.5f), Vector3(-40.2f, 0, -2.5f), UF, -UF - 0.5f, 0.25f, "concrete");
	b.wall(Vector3(-42.5f, 0, 0.1f), Vector3(-40.2f, 0, 0.1f), UF, -UF - 0.5f, 0.25f, "concrete");
	b.wall(Vector3(-42.5f, 0, -2.5f), Vector3(-42.5f, 0, 0.1f), UF, -UF - 0.5f, 0.25f, "concrete");
	b.wall(Vector3(-40.2f, 0, -2.5f), Vector3(-40.2f, 0, -2.0f), UF, -UF - 0.5f, 0.25f, "concrete");
	b.wall(Vector3(-40.2f, 0, -0.4f), Vector3(-40.2f, 0, 0.1f), UF, -UF - 0.5f, 0.25f, "concrete");
	b.wall(Vector3(-40.2f, 0, -2.0f), Vector3(-40.2f, 0, -0.4f), UF + 2.2f, -UF - 2.7f, 0.25f, "concrete");
	for (int i = 0; i < 14; i++) {
		b.visual_box(Vector3(-42.3f, UF + 0.4f + float(i) * 0.4f, -1.2f), Vector3(0.05f, 0.03f, 0.45f), "rust");
	}
	k.spot(Vector3(-24.0f, UF + 1.1f, -1.2f), "blast_door", "Гермодверь аварийного выхода"_u, 10.0f);
	k.noisy(Vector3(-32.0f, UF + 0.2f, -1.2f), Vector3(3.0f, 0.4f, 1.6f));
	for (int i = 0; i < 5; i++) {
		props::puddle(b, Vector3(-36.0f + float(i) * 2.6f, UF, -1.2f + b.rng.range(-0.3f, 0.3f)), b.rng.range(0.6f, 1.1f), 200u + uint32_t(i) * 17u);
	}

	Vector3 og(-41.2f, 0.0f, -1.2f);
	b.wall(og + Vector3(-1.4f, 0, -1.4f), og + Vector3(1.4f, 0, -1.4f), 0.0f, 2.2f, 0.3f, "concrete");
	b.wall(og + Vector3(-1.4f, 0, 1.4f), og + Vector3(1.4f, 0, 1.4f), 0.0f, 2.2f, 0.3f, "concrete");
	b.wall(og + Vector3(-1.4f, 0, -1.4f), og + Vector3(-1.4f, 0, 1.4f), 0.0f, 2.2f, 0.3f, "concrete");
	b.wall(og + Vector3(1.4f, 0, -1.4f), og + Vector3(1.4f, 0, 1.4f), 0.0f, 2.2f, 0.3f, "concrete", { door_gap(1.4f, 1.0f, 2.0f) });
	b.box(og + Vector3(0, 2.3f, 0), Vector3(3.2f, 0.2f, 3.2f), "concrete");
	b.box(og + Vector3(0, 0.05f, 0), Vector3(2.6f, 0.1f, 2.6f), "concrete_floor");
	b.visual_box(og + Vector3(-0.6f, 0.11f, 0), Vector3(0.8f, 0.02f, 0.8f), "black");
	b.text(og + Vector3(1.56f, 2.08f, 0.0f), Vector3(1, 0, 0), "АВ. ВЫХОД"_u, 0.16f, Color(0.92f, 0.9f, 0.82f));
	Door *og_door = k.door(og + Vector3(1.4f, 0.0f, 0.0f), -PI * 0.5f, Door::STYLE_METAL, 0.9f, 1.95f, "Дверь оголовка аварийного выхода"_u);
	og_door->set_padlock(true);
	ClimbPoint *down = k.climb(og + Vector3(-0.6f, 0.4f, 0.0f), Vector3(0.9f, 0.6f, 0.9f), "Спуститься по скобам в шахту аварийного выхода"_u, Vector3(-40.6f, UF + 0.15f, -1.2f), -PI * 0.5f, 2.2f);
	down->set_noise(3.0f, "step_metal_1");
	down->set_arrival_message("Шесть метров по ржавым скобам. Внизу сыро и пахнет плесенью"_u);
	ClimbPoint *up = k.climb(Vector3(-41.9f, UF + 1.0f, -1.2f), Vector3(0.6f, 1.6f, 1.4f), "Подняться по скобам к оголовку"_u, og + Vector3(0.4f, 0.15f, 0.0f), -PI * 0.5f, 2.2f);
	up->set_noise(3.0f, "step_metal_2");
	k.hint(og + Vector3(3.5f, 1.0f, 0.0f), Vector3(4.0f, 2.0f, 5.0f), "Оголовок аварийного выхода. Навесной замок можно перекусить болторезом, но это громко."_u);
	k.shadow_zone(Vector3(-14.0f, -3.5f, 3.5f), Vector3(57.0f, 6.0f, 26.5f));

	b.fence(Vector3(-50.0f, 0, -30.0f), Vector3(35.0f, 0, -30.0f), 2.6f, "fence_concrete", { Opening{ 30.0f, 1.3f, 0.0f, 2.6f }, Opening{ 61.0f, 6.0f, 0.0f, 2.6f } });
	b.fence(Vector3(-50.0f, 0, 30.0f), Vector3(35.0f, 0, 30.0f), 2.6f, "fence_concrete");
	b.fence(Vector3(-50.0f, 0, -30.0f), Vector3(-50.0f, 0, 30.0f), 2.6f, "fence_concrete");
	b.fence(Vector3(35.0f, 0, -30.0f), Vector3(35.0f, 0, 30.0f), 2.6f, "fence_concrete");
	b.rubble(Vector3(-20.0f, 0.0f, -30.5f), 1.2f, 14, "concrete");
	Door *gate = k.door(Vector3(11.0f, 0.0f, -30.0f), 0.0f, Door::STYLE_GATE, 6.0f, 2.4f, "Ворота завода"_u);
	gate->set_jammed(true, "Ворота закрыты изнутри"_u);
	k.hint(Vector3(-20.0f, 1.0f, -33.0f), Vector3(5.0f, 2.0f, 5.0f), "Пролом в бетонном заборе. Во дворе ходит обходчик, у ворот сидит вахтёр."_u);

	b.box(Vector3(-27.0f, 5.0f, 18.0f), Vector3(30.0f, 10.0f, 18.0f), "brick");
	b.box(Vector3(-27.0f, 10.2f, 18.0f), Vector3(30.6f, 0.4f, 18.6f), "roof");
	for (int i = 0; i < 6; i++) {
		b.visual_box(Vector3(-40.0f + float(i) * 5.0f, 5.5f, 8.98f), Vector3(2.4f, 3.0f, 0.02f), "black");
	}
	b.text(Vector3(-27.0f, 8.4f, 8.95f), Vector3(0, 0, -1), "ЦЕХ № 3"_u, 1.0f, Color(0.75f, 0.72f, 0.65f));
	b.box(Vector3(18.0f, 0.4f, -8.0f), Vector3(1.2f, 0.8f, 1.2f), "wood");
	b.box(Vector3(19.4f, 0.4f, -8.2f), Vector3(1.2f, 0.8f, 1.2f), "wood");

	Vector3 booth(16.5f, 0.0f, -25.0f);
	b.box(booth + Vector3(0, 0.05f, 0), Vector3(3.5f, 0.1f, 3.0f), "concrete_floor");
	b.wall(booth + Vector3(-1.75f, 0, -1.5f), booth + Vector3(1.75f, 0, -1.5f), 0.0f, 2.7f, 0.2f, "brick", { window_gap(1.75f, 2.2f, 0.9f, 2.2f) });
	b.wall(booth + Vector3(-1.75f, 0, 1.5f), booth + Vector3(1.75f, 0, 1.5f), 0.0f, 2.7f, 0.2f, "brick", { door_gap(1.2f, 0.9f, 2.1f) });
	b.wall(booth + Vector3(-1.75f, 0, -1.5f), booth + Vector3(-1.75f, 0, 1.5f), 0.0f, 2.7f, 0.2f, "brick", { window_gap(1.5f, 1.6f, 0.9f, 2.2f) });
	b.wall(booth + Vector3(1.75f, 0, -1.5f), booth + Vector3(1.75f, 0, 1.5f), 0.0f, 2.7f, 0.2f, "brick");
	b.box(booth + Vector3(0, 2.8f, 0), Vector3(3.9f, 0.2f, 3.4f), "roof");
	b.box(booth + Vector3(0.6f, 0.4f, -0.9f), Vector3(1.6f, 0.8f, 0.6f), "wood");
	b.visual_box(booth + Vector3(1.1f, 1.0f, -0.95f), Vector3(0.4f, 0.32f, 0.28f), "plastic_dark");
	b.visual_box(booth + Vector3(1.1f, 1.0f, -0.8f), Vector3(0.32f, 0.24f, 0.01f), "tv_glow");
	OmniLight3D *booth_lamp = b.omni(booth + Vector3(0, 2.4f, 0), Color(1.0f, 0.82f, 0.55f), 8.0f, 1.5f, true);
	g.register_light(booth_lamp, "plant");
	k.probe(booth + Vector3(0, 1.0f, 0), 6.0f, 0.85f, "plant");
	k.shadow_zone(booth + Vector3(0, 1.3f, 0), Vector3(3.5f, 2.7f, 3.0f));

	for (const Vector3 &p : { Vector3(2.0f, 0.0f, 4.0f), Vector3(14.0f, 0.0f, -18.0f), Vector3(-15.0f, 0.0f, 5.0f) }) {
		k.street_lamp(p, PI, "plant");
	}
	for (float x : { -40.0f, -10.0f, 20.0f }) {
		k.street_lamp(Vector3(x, 0.0f, -37.5f), 0.0f);
	}
	Rng tr(7);
	for (int i = 0; i < 26; i++) {
		Vector3 p(tr.range(-48.0f, -30.0f), 0.0f, tr.range(-28.0f, 28.0f));
		if ((p - og).length() < 4.0f || p.z > 8.0f) {
			continue;
		}
		if (tr.chance(0.65f)) {
			props::birch(b, p, tr.range(7.0f, 11.0f), uint32_t(i) * 7u + 3u);
		} else {
			b.tree(p, tr.range(6.0f, 10.0f));
		}
	}
	for (int i = 0; i < 10; i++) {
		float a = float(i) / 10.0f * 2.0f * PI;
		float r = 120.0f + tr.range(0.0f, 40.0f);
		b.block_building(Vector3(std::cos(a) * r, 0.0f, std::sin(a) * r), Vector3(tr.range(14.0f, 20.0f), tr.range(24.0f, 45.0f), tr.range(30.0f, 60.0f)));
	}

	Guard *yard = k.guard(Guard::KIND_CHOP, "Обходчик ЧОП"_u, Vector3(20.0f, 0.2f, -5.0f), true);
	yard->add_route_point(Vector3(20.0f, 0.0f, -5.0f), 2.0f);
	yard->add_route_point(Vector3(5.0f, 0.0f, 3.5f), 4.0f);
	yard->add_route_point(Vector3(-20.0f, 0.0f, 3.0f), 2.0f);
	yard->add_route_point(Vector3(-28.0f, 0.0f, -22.0f), 3.0f);
	yard->add_route_point(Vector3(0.0f, 0.0f, -24.0f), 2.0f);
	yard->add_route_point(Vector3(28.0f, 0.0f, 20.0f), 2.0f);

	Guard *duty = k.guard(Guard::KIND_CHOP, "Дежурный по убежищу"_u, Vector3(8.0f, UF + 0.2f, -1.2f), true);
	duty->add_route_point(Vector3(9.0f, UF, -1.2f), 4.0f);
	duty->add_route_point(Vector3(4.0f, UF, -6.0f), 3.0f);
	duty->add_route_point(Vector3(-8.0f, UF, -1.2f), 1.0f);
	duty->add_route_point(Vector3(-8.0f, UF, -6.0f), 3.0f);
	duty->add_route_point(Vector3(-20.0f, UF, -1.2f), 2.0f);
	duty->add_route_point(Vector3(-3.0f, UF, 3.0f), 3.0f);
	duty->set_vision(14.0f, 105.0f);

	Guard *watchman = k.guard(Guard::KIND_WATCHMAN, "Вахтёр на КПП"_u, booth + Vector3(0.5f, 0.15f, -0.3f), false);
	watchman->set_post(booth + Vector3(0.5f, 0.15f, -0.3f), { yaw_towards(booth, Vector3(11.0f, 0, -32.0f)), yaw_towards(booth, Vector3(0.0f, 0, 5.0f)) }, 8.0f, true);
	watchman->set_vision(12.0f, 100.0f);

	shelter_dressing(k);

	d.gbr_search_points = { Vector3(5.0f, 0.0f, 3.0f), Vector3(6.6f, UF, 6.0f), Vector3(0.0f, UF, -1.2f), Vector3(-8.0f, UF, -6.0f), Vector3(4.0f, UF, -6.0f), Vector3(-19.0f, UF, -6.0f), Vector3(-30.0f, UF, -1.2f), Vector3(-3.0f, UF, 3.0f), Vector3(-36.0f, 0.0f, -6.0f) };
}

}
