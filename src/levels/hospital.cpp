#include "levels/kit.h"
#include "levels/levels.h"

#include "core/game.h"

using namespace godot;

namespace urbex {

namespace {

constexpr float FH = 3.6f;
constexpr float HUB_R = 10.0f;
constexpr float HUB_A = 8.660254f;
constexpr float WING_L = 30.0f;
constexpr float HW = 5.0f;
constexpr float CW = 1.4f;
constexpr float WALL = 0.3f;
constexpr float INNER = 0.2f;
constexpr int FLOORS = 3;
constexpr float ROOF = FH * FLOORS;
constexpr float GROUND_TOP = 0.05f;

float floor_top(int n) {
	return n == 0 ? GROUND_TOP : FH * float(n);
}

struct WingSpec {
	float angle = 0.0f;
	int stair_top = 2;
	bool shaft = false;
	bool end_door = false;
	int breach_side = 0;
	float breach_offset = 15.0f;
	std::vector<std::pair<int, Rect2>> holes;
	std::vector<std::pair<int, float>> missing_partitions;
};

Vector3 wing_local(float x, float y, float z) {
	return Vector3(x, y, HUB_A + z);
}

void build_stairs(LevelBuilder &b, int n) {
	float z0 = HUB_A;
	float yb = floor_top(n);
	float ym = FH * float(n) + FH * 0.5f;
	float yt = FH * float(n + 1);
	b.ramp(Vector3(4.07f, yb, z0 + 1.5f), Vector3(4.07f, ym, z0 + 4.5f), 1.55f, "concrete_floor");
	b.box(Vector3(3.2f, ym - 0.125f, z0 + 5.25f), Vector3(3.4f, 0.25f, 1.5f), "concrete_floor");
	b.ramp(Vector3(2.42f, ym, z0 + 4.5f), Vector3(2.42f, yt, z0 + 1.5f), 1.55f, "concrete_floor");
	b.box(Vector3(3.25f, FH * float(n) + (FH - 0.3f) * 0.5f, z0 + 3.0f), Vector3(0.08f, FH - 0.3f, 3.0f), "concrete");
}

void build_wing(Kit &k, const WingSpec &spec) {
	LevelBuilder &b = k.b;
	b.push_yaw(Vector3(), spec.angle);
	const float z0 = HUB_A;
	const float z1 = HUB_A + WING_L;

	for (int n = 0; n <= FLOORS; n++) {
		std::vector<Rect2> holes;
		if (n >= 1 && n <= spec.stair_top) {
			holes.push_back(Rect2(CW, z0 + 1.5f, HW - CW, 4.5f));
		}
		if (spec.shaft && n < FLOORS) {
			holes.push_back(Rect2(-4.6f, z0 + 18.5f, 2.8f, 3.0f));
		}
		for (const auto &h : spec.holes) {
			if (h.first == n) {
				holes.push_back(h.second);
			}
		}
		float top = floor_top(n);
		float thick = n == 0 ? 0.25f : 0.3f;
		b.slab(-HW - 0.15f, z0, HW + 0.15f, z1 + 0.15f, top, thick, n == FLOORS ? "roof" : "concrete_floor", holes);
	}

	for (int n = 0; n < FLOORS; n++) {
		float y0 = FH * float(n);
		for (int side : { -1, 1 }) {
			std::vector<Opening> ext;
			for (int i = 0; i < 6; i++) {
				float off = 2.5f + 5.0f * float(i);
				if (n == 0 && spec.breach_side == side && std::fabs(off - spec.breach_offset) < 0.1f) {
					ext.push_back(Opening{ off, 2.4f, 0.0f, 2.6f });
				} else {
					ext.push_back(window_gap(off, 1.6f, 0.9f, 2.5f));
				}
			}
			b.wall(Vector3(float(side) * HW, 0, z0), Vector3(float(side) * HW, 0, z1), y0, FH, WALL, "concrete", ext);
		}
		std::vector<Opening> end;
		if (n == 0 && spec.end_door) {
			end.push_back(Opening{ HW, 1.8f, 0.0f, 2.4f });
		} else {
			end.push_back(window_gap(HW, 1.6f, 0.9f, 2.5f));
		}
		b.wall(Vector3(-HW, 0, z1), Vector3(HW, 0, z1), y0, FH, WALL, "concrete", end);
		b.wall(Vector3(-HW, 0, z0), Vector3(HW, 0, z0), y0, FH - 0.3f, INNER, "concrete", { Opening{ HW, CW * 2.0f, 0.0f, 3.0f } });

		std::vector<Opening> right = { door_gap(0.75f, 1.2f, 2.2f) };
		for (float off : { 10.0f, 15.0f, 20.0f, 25.0f, 28.75f }) {
			right.push_back(door_gap(off, 1.0f, 2.1f));
		}
		std::vector<Opening> left;
		for (int i = 0; i < 6; i++) {
			float off = 2.5f + 5.0f * float(i);
			if (spec.shaft && (i == 3 || i == 4)) {
				continue;
			}
			left.push_back(door_gap(off, 1.0f, 2.1f));
		}
		if (spec.shaft) {
			left.push_back(door_gap(16.2f, 1.0f, 2.1f));
		}
		if (b.rng.chance(0.4f)) {
			left.push_back(Opening{ 5.0f + float(b.rng.rangei(0, 4)) * 5.0f, 2.2f, 0.0f, 2.6f });
		}
		b.wall(Vector3(CW, 0, z0), Vector3(CW, 0, z1), y0, FH - 0.3f, INNER, n == 0 ? "paint_blue" : "plaster", right);
		b.wall(Vector3(-CW, 0, z0), Vector3(-CW, 0, z1), y0, FH - 0.3f, INNER, n == 0 ? "paint_blue" : "plaster", left);

		for (float zp : { 7.5f, 12.5f, 17.5f, 22.5f, 27.5f }) {
			bool missing = false;
			for (const auto &mp : spec.missing_partitions) {
				if (mp.first == n && std::fabs(mp.second - zp) < 0.1f) {
					missing = true;
				}
			}
			if (!missing) {
				b.wall(Vector3(CW, 0, z0 + zp), Vector3(HW, 0, z0 + zp), y0, FH - 0.3f, INNER, "plaster");
			}
		}
		for (float zp : { 5.0f, 10.0f, 15.0f, 20.0f, 25.0f }) {
			if (spec.shaft && (zp == 20.0f)) {
				continue;
			}
			bool missing = false;
			for (const auto &mp : spec.missing_partitions) {
				if (mp.first == n && std::fabs(mp.second + zp) < 0.1f) {
					missing = true;
				}
			}
			if (!missing) {
				b.wall(Vector3(-HW, 0, z0 + zp), Vector3(-CW, 0, z0 + zp), y0, FH - 0.3f, INNER, "plaster");
			}
		}

		if (n < spec.stair_top) {
			build_stairs(b, n);
		}

		if (spec.shaft) {
			float sh = n == FLOORS - 1 ? FH - 0.3f : FH;
			b.wall(Vector3(-1.8f, 0, z0 + 18.5f), Vector3(-1.8f, 0, z0 + 21.5f), y0, sh, INNER, "concrete", { door_gap(1.5f, 1.2f, 2.1f) });
			b.wall(Vector3(-4.6f, 0, z0 + 18.5f), Vector3(-1.8f, 0, z0 + 18.5f), y0, sh, INNER, "concrete");
			b.wall(Vector3(-4.6f, 0, z0 + 21.5f), Vector3(-1.8f, 0, z0 + 21.5f), y0, sh, INNER, "concrete");
			b.wall(Vector3(-4.6f, 0, z0 + 18.5f), Vector3(-4.6f, 0, z0 + 21.5f), y0, sh, INNER, "concrete");
		}

		for (int r = 0; r < 4; r++) {
			float zz = z0 + b.rng.range(2.0f, WING_L - 2.0f);
			float xx = b.rng.chance(0.5f) ? b.rng.range(-4.3f, -2.0f) : b.rng.range(2.0f, 4.3f);
			if (spec.shaft && xx < 0 && zz > z0 + 17.5f && zz < z0 + 22.5f) {
				continue;
			}
			if (xx > 0 && zz < z0 + 7.0f) {
				continue;
			}
			b.rubble(Vector3(xx, floor_top(n), zz), b.rng.range(0.6f, 1.3f), b.rng.rangei(6, 14), "concrete");
		}
	}

	{
		float y = floor_top(spec.stair_top);
		b.railing(Vector3(CW + 0.1f, 0, z0 + 6.0f), Vector3(HW - 0.2f, 0, z0 + 6.0f), y, 1.0f, "rust");
		b.railing(Vector3(3.35f, 0, z0 + 1.5f), Vector3(HW - 0.2f, 0, z0 + 1.5f), y, 1.0f, "rust");
	}

	if (spec.stair_top == FLOORS) {
		float y = ROOF;
		float h = 2.6f;
		b.wall(Vector3(HW, 0, z0), Vector3(HW, 0, z0 + 6.15f), y, h, WALL, "brick");
		b.wall(Vector3(CW, 0, z0), Vector3(CW, 0, z0 + 6.15f), y, h, INNER, "brick", { door_gap(0.75f, 1.2f, 2.15f) });
		b.wall(Vector3(CW, 0, z0 + 6.15f), Vector3(HW, 0, z0 + 6.15f), y, h, INNER, "brick");
		b.wall(Vector3(CW, 0, z0), Vector3(HW, 0, z0), y, h, INNER, "brick");
		b.box(Vector3((CW + HW) * 0.5f, y + h + 0.1f, z0 + 3.07f), Vector3(HW - CW + 0.3f, 0.2f, 6.4f), "roof");
	}

	for (int side : { -1, 1 }) {
		b.wall(Vector3(float(side) * HW, 0, z0), Vector3(float(side) * HW, 0, z1), ROOF, 0.7f, WALL, "concrete");
	}
	b.wall(Vector3(-HW, 0, z1), Vector3(HW, 0, z1), ROOF, 0.7f, WALL, "concrete");

	k.shadow_zone(Vector3(0, ROOF * 0.5f - 0.2f, z0 + WING_L * 0.5f), Vector3(HW * 2.0f, ROOF, WING_L));
	b.pop();
}

void build_hub(Kit &k) {
	LevelBuilder &b = k.b;
	for (int n = 0; n <= FLOORS; n++) {
		b.hex_slab(Vector3(), HUB_R, floor_top(n), n == 0 ? 0.25f : 0.3f, n == FLOORS ? "roof" : "concrete_floor");
	}
	for (int i = 0; i < 6; i++) {
		float a = float(i) * PI / 3.0f;
		b.box(Vector3(std::sin(a) * 5.0f, (ROOF - 0.3f) * 0.5f, std::cos(a) * 5.0f), Vector3(0.5f, ROOF - 0.3f, 0.5f), "concrete");
	}
	for (float deg : { 90.0f, 210.0f, 330.0f }) {
		b.push_yaw(Vector3(), deg * DEG);
		for (int n = 0; n < FLOORS; n++) {
			std::vector<Opening> o;
			if (deg == 210.0f && n == 0) {
				o = { window_gap(1.6f, 1.2f), Opening{ 5.0f, 2.4f, 0.0f, 2.8f }, window_gap(8.4f, 1.2f) };
			} else if (deg == 90.0f && n == 1) {
				o = {};
			} else {
				o = { window_gap(2.5f, 1.6f, 0.9f, 2.5f), window_gap(7.5f, 1.6f, 0.9f, 2.5f) };
			}
			b.wall(Vector3(-HUB_R * 0.5f - 0.1f, 0, HUB_A), Vector3(HUB_R * 0.5f + 0.1f, 0, HUB_A), FH * float(n), FH, WALL, "concrete", o);
		}
		b.wall(Vector3(-HUB_R * 0.5f - 0.1f, 0, HUB_A), Vector3(HUB_R * 0.5f + 0.1f, 0, HUB_A), ROOF, 0.7f, WALL, "concrete");
		b.pop();
	}
	k.shadow_zone(Vector3(0, ROOF * 0.5f - 0.2f, 0), Vector3(HUB_A * 2.0f, ROOF, HUB_A * 2.0f));

	b.box(Vector3(-3.0f, GROUND_TOP + 0.55f, 4.2f), Vector3(3.2f, 1.1f, 0.6f), "plaster");
	b.box(Vector3(-3.0f, GROUND_TOP + 1.12f, 4.2f), Vector3(3.4f, 0.05f, 0.8f), "wood", false);
	for (int i = 0; i < 5; i++) {
		float a = b.rng.range(0.0f, 2.0f * PI);
		float r = b.rng.range(1.5f, 7.0f);
		b.rubble(Vector3(std::sin(a) * r, GROUND_TOP, std::cos(a) * r), 1.0f, 10, "concrete");
	}
	props::puddle(b, Vector3(2.0f, GROUND_TOP, -2.0f), 2.2f, 5u);
	props::puddle(b, Vector3(1.0f, GROUND_TOP, -1.2f), 1.2f, 9u);
}

void build_booth(Kit &k, const Vector3 &c) {
	LevelBuilder &b = k.b;
	float w = 4.0f;
	float d = 3.0f;
	float h = 2.8f;
	b.box(c + Vector3(0, 0.05f, 0), Vector3(w, 0.1f, d), "concrete_floor");
	b.wall(c + Vector3(-w * 0.5f, 0, -d * 0.5f), c + Vector3(w * 0.5f, 0, -d * 0.5f), 0.0f, h, 0.2f, "brick", { window_gap(2.0f, 2.4f, 0.9f, 2.2f) });
	b.wall(c + Vector3(-w * 0.5f, 0, d * 0.5f), c + Vector3(w * 0.5f, 0, d * 0.5f), 0.0f, h, 0.2f, "brick", { window_gap(2.0f, 1.6f, 0.9f, 2.2f) });
	b.wall(c + Vector3(-w * 0.5f, 0, -d * 0.5f), c + Vector3(-w * 0.5f, 0, d * 0.5f), 0.0f, h, 0.2f, "brick", { door_gap(1.5f, 0.9f, 2.1f) });
	b.wall(c + Vector3(w * 0.5f, 0, -d * 0.5f), c + Vector3(w * 0.5f, 0, d * 0.5f), 0.0f, h, 0.2f, "brick", { window_gap(1.5f, 1.2f, 0.9f, 2.2f) });
	b.box(c + Vector3(0, h + 0.1f, 0), Vector3(w + 0.4f, 0.2f, d + 0.4f), "roof");
	b.box(c + Vector3(0.6f, 0.4f, -0.9f), Vector3(1.6f, 0.8f, 0.7f), "wood");
	b.visual_box(c + Vector3(1.2f, 1.0f, -1.0f), Vector3(0.45f, 0.35f, 0.3f), "plastic_dark");
	b.visual_box(c + Vector3(1.2f, 1.0f, -0.85f), Vector3(0.36f, 0.26f, 0.01f), "tv_glow");
	OmniLight3D *lamp = b.omni(c + Vector3(0, h - 0.3f, 0), Color(1.0f, 0.82f, 0.55f), 9.0f, 1.6f, true);
	k.g.register_light(lamp, "booth");
	b.lamp_fixture(c + Vector3(0, h - 0.1f, 0), true);
	k.probe(c + Vector3(0, 1.0f, 0), 7.0f, 0.85f, "booth");
	k.shadow_zone(c + Vector3(0, 1.4f, 0), Vector3(w, h, d));
}

void wing_windows(LevelBuilder &b, uint32_t seed) {
	Rng r(seed);
	for (int n = 0; n < FLOORS; n++) {
		for (int side : { -1, 1 }) {
			for (int i = 0; i < 6; i++) {
				if (!r.chance(0.35f)) {
					continue;
				}
				float off = 2.5f + 5.0f * float(i);
				props::window_frame(b, Vector3(float(side) * HW, FH * float(n) + 1.7f, HUB_A + off), PI * 0.5f, 1.55f, 1.55f, r.range(0.4f, 0.95f));
			}
		}
	}
}

void wing_dressing(Kit &k, float angle, uint32_t seed) {
	LevelBuilder &b = k.b;
	b.push_yaw(Vector3(), angle);
	const float A = HUB_A;
	wing_windows(b, seed);
	for (int n = 0; n < FLOORS; n++) {
		float y = floor_top(n);
		k.dust(Vector3(0.0f, y + 1.5f, A + 15.0f), Vector3(1.3f, 1.3f, 14.0f), 0.8f);
		k.dust(Vector3(3.2f, y + 1.5f, A + 20.0f), Vector3(1.6f, 1.3f, 9.0f), 0.5f);
		k.tags(Vector3(CW - 0.12f, y + 1.45f, A + 5.0f + float(n) * 7.5f), Vector3(-1, 0, 0), int(seed) + n * 3, 0.3f);
		k.tags(Vector3(CW - 0.12f, y + 1.7f, A + 22.5f - float(n) * 5.0f), Vector3(-1, 0, 0), int(seed) + n * 3 + 1, 0.25f);
		props::hanging_cable(b, Vector3(-0.8f, FH * float(n + 1) - 0.32f, A + 8.0f + float(n) * 4.0f), Vector3(0.6f, FH * float(n + 1) - 0.32f, A + 11.0f + float(n) * 4.0f), 0.6f + float(n) * 0.2f);
		props::radiator(b, Vector3(HW - 0.22f, y, A + 10.0f), -PI * 0.5f, 8);
		props::radiator(b, Vector3(-HW + 0.22f, y, A + 12.5f), PI * 0.5f, 10);
		k.decal("decal_streak", Vector3(float(n % 2 == 0 ? 1 : -1) * (HW + 0.16f), y + 0.8f, A + 7.5f + float(n) * 5.0f), Vector3(float(n % 2 == 0 ? 1 : -1), 0, 0), Vector2(1.4f, 2.6f), 0.0f, 0.8f);
		k.decal("decal_water", Vector3(0.0f, y + 0.02f, A + 18.0f - float(n) * 3.0f), Vector3(0, 1, 0), Vector2(2.2f, 2.2f), float(n), 0.7f);
	}
	k.debris(Vector3(0.0f, FH - 0.35f, A + 13.0f + float(seed % 7)));
	k.debris(Vector3(0.4f, 2.0f * FH - 0.35f, A + 24.0f));
	b.pop();
}

void hospital_dressing(Kit &k) {
	LevelBuilder &b = k.b;
	const float A = HUB_A;

	k.dust(Vector3(0.0f, GROUND_TOP + 1.7f, 0.0f), Vector3(6.5f, 1.6f, 6.5f), 0.7f);
	k.dust(Vector3(0.0f, FH + 1.7f, 0.0f), Vector3(6.5f, 1.6f, 6.5f), 0.5f);
	k.drips(Vector3(2.2f, FH - 0.2f, -2.1f), FH - 0.25f, 1.6f, false);
	k.drips(Vector3(1.4f, FH - 0.2f, -1.4f), FH - 0.25f, 0.7f, false);
	k.debris(Vector3(-2.0f, FH - 0.35f, 2.0f));
	k.decal("decal_water", Vector3(2.0f, FH - 0.16f, -2.0f), Vector3(0, -1, 0), Vector2(3.0f, 3.0f), 0.4f, 0.9f);
	props::stretcher(b, Vector3(-5.5f, GROUND_TOP, -1.5f), 0.9f);
	props::wheelchair(b, Vector3(-1.2f, GROUND_TOP, 5.2f), 2.3f, true);
	props::chair(b, Vector3(-3.6f, GROUND_TOP, 3.2f), 0.4f, false);
	props::chair(b, Vector3(-2.0f, GROUND_TOP, 3.0f), 2.2f, true);
	props::cabinet(b, Vector3(-5.4f, GROUND_TOP, 4.4f), PI * 0.5f + 0.5f, true);
	props::wall_clock(b, Vector3(-3.0f, 2.6f, 4.55f), PI);
	props::bottles(b, Vector3(4.0f, GROUND_TOP, 2.5f), 5, 17);
	for (int i = 0; i < 6; i++) {
		float a = float(i) * PI / 3.0f;
		Vector3 c(std::sin(a) * 5.0f, 0.0f, std::cos(a) * 5.0f);
		if (i % 2 == 0) {
			props::hanging_cable(b, c + Vector3(0.0f, ROOF - 0.32f, 0.0f), c * 0.3f + Vector3(0.0f, ROOF - 0.32f, 0.0f), 1.4f);
		}
	}
	for (float deg : { 90.0f, 210.0f, 330.0f }) {
		b.push_yaw(Vector3(), deg * DEG);
		for (int n = 0; n < FLOORS; n++) {
			if ((deg == 210.0f && n == 0) || (deg == 90.0f && n == 1)) {
				continue;
			}
			for (float x : { -2.5f, 2.5f }) {
				if (std::fmod(deg + float(n) * 50.0f + x, 3.0f) < 1.4f) {
					props::window_frame(b, Vector3(x, FH * float(n) + 1.7f, HUB_A), 0.0f, 1.55f, 1.55f, 0.6f);
				}
			}
		}
		b.pop();
	}

	wing_dressing(k, 270.0f * DEG, 3);
	wing_dressing(k, 30.0f * DEG, 7);
	wing_dressing(k, 150.0f * DEG, 12);

	b.push_yaw(Vector3(), 270.0f * DEG);
	k.drips(Vector3(-3.2f, ROOF - 0.2f, A + 20.3f), ROOF - 0.2f + 3.0f, 2.2f, false);
	k.drips(Vector3(-2.4f, ROOF - 0.2f, A + 19.4f), ROOF - 0.2f + 3.0f, 0.8f, false);
	k.fog(Vector3(-3.2f, -2.75f, A + 20.0f), Vector3(1.2f, 0.15f, 1.3f), 3.0f);
	k.sound_loop(Vector3(-3.2f, -2.5f, A + 20.0f), "drip", -6.0f, 16.0f);
	for (int n = 0; n < FLOORS; n++) {
		props::rebar(b, Vector3(-1.8f, floor_top(n + 1) - 0.15f, A + 20.0f), Vector3(-1, 0, 0), 5, 31u + uint32_t(n));
	}
	props::rebar(b, Vector3(0.0f, 2.0f * FH - 0.15f, A + 25.0f), Vector3(0, 0, 1), 7, 55u);
	props::rebar(b, Vector3(0.0f, 2.0f * FH - 0.15f, A + 27.0f), Vector3(0, 0, -1), 7, 56u);
	props::cabinet(b, Vector3(4.55f, GROUND_TOP, A + 9.5f), -PI * 0.5f, false);
	props::cabinet(b, Vector3(4.55f, GROUND_TOP, A + 10.4f), -PI * 0.5f, true);
	props::table(b, Vector3(3.0f, GROUND_TOP, A + 11.0f), 0.2f, 1.2f, 0.7f);
	props::chair(b, Vector3(2.6f, GROUND_TOP, A + 12.0f), 3.0f, true);
	props::hospital_bed(b, Vector3(3.3f, FH, A + 20.5f), PI * 0.5f + 0.15f, true);
	props::wheelchair(b, Vector3(3.0f, FH, A + 23.0f), 1.0f, false);
	b.pop();

	b.push_yaw(Vector3(), 30.0f * DEG);
	for (int i = 0; i < 4; i++) {
		props::hospital_bed(b, Vector3(3.25f, GROUND_TOP, A + 9.0f + float(i) * 5.0f), PI * 0.5f + (i % 2 == 0 ? 0.05f : -0.12f), i != 2);
	}
	props::wheelchair(b, Vector3(0.75f, GROUND_TOP, A + 16.5f), 0.4f, true);
	k.fire_barrel(Vector3(-3.9f, GROUND_TOP, A + 18.6f));
	props::mattress(b, Vector3(-2.9f, GROUND_TOP, A + 16.4f), PI * 0.5f + 0.1f);
	props::cardboard(b, Vector3(-4.0f, GROUND_TOP, A + 15.9f), 0.3f);
	props::bottles(b, Vector3(-2.6f, GROUND_TOP, A + 18.9f), 7, 5);
	props::chair(b, Vector3(-2.7f, GROUND_TOP, A + 19.3f), 2.5f, false);
	props::kettle(b, Vector3(-3.2f, GROUND_TOP, A + 19.4f));
	k.decal("decal_soot", Vector3(-4.85f, 1.9f, A + 18.6f), Vector3(1, 0, 0), Vector2(1.6f, 2.4f), 0.0f, 0.9f);
	k.decal("decal_soot", Vector3(-3.9f, FH - 0.16f, A + 18.6f), Vector3(0, -1, 0), Vector2(2.4f, 2.4f), 0.0f, 0.9f);
	b.graffiti(Vector3(-3.2f, 1.7f, A + 15.12f), Vector3(0, 0, 1), "НЕ ВХОДИТЬ ЖИВУ ТУТ"_u, 0.22f);
	props::rebar(b, Vector3(-3.2f, FH - 0.15f, A + 11.0f), Vector3(0, 0, 1), 6, 77u);
	props::rebar(b, Vector3(1.6f, 2.0f * FH - 0.15f, A + 18.0f), Vector3(0, 0, 1), 9, 78u);
	props::rebar(b, Vector3(1.6f, 2.0f * FH - 0.15f, A + 23.0f), Vector3(0, 0, -1), 9, 79u);
	k.drips(Vector3(1.0f, ROOF - 0.2f, A + 20.5f), ROOF - 0.2f - FH, 1.2f, true);
	k.drips(Vector3(2.8f, ROOF - 0.2f, A + 19.2f), ROOF - 0.2f - FH, 0.6f, false);
	props::cabinet(b, Vector3(-4.55f, FH, A + 21.0f), PI * 0.5f, true);
	props::table(b, Vector3(-3.3f, FH, A + 23.4f), 0.0f, 1.4f, 0.8f);
	b.pop();

	b.push_yaw(Vector3(), 150.0f * DEG);
	for (int i = 0; i < 3; i++) {
		props::hospital_bed(b, Vector3(-3.25f, GROUND_TOP, A + 4.0f + float(i) * 10.0f), PI * 0.5f + float(i) * 0.08f, i == 1);
	}
	props::stretcher(b, Vector3(3.3f, GROUND_TOP, A + 21.0f), 1.3f);
	k.candles(Vector3(-3.2f, 2.0f * FH, A + 22.6f), 1.0f);
	b.graffiti(Vector3(-3.2f, 2.0f * FH + 1.6f, A + 20.12f), Vector3(0, 0, 1), "НИМОСТОР"_u, 0.5f)->set_modulate(Color(0.6f, 0.05f, 0.05f));
	b.graffiti(Vector3(-3.2f, 2.0f * FH + 1.0f, A + 20.12f), Vector3(0, 0, 1), "он смотрит"_u, 0.18f);
	props::rebar(b, Vector3(0.0f, FH - 0.15f, A + 20.0f), Vector3(0, 0, 1), 5, 91u);
	props::rebar(b, Vector3(3.2f, 2.0f * FH - 0.15f, A + 16.0f), Vector3(0, 0, -1), 6, 92u);
	k.drips(Vector3(0.2f, 2.0f * FH - 0.2f, A + 21.2f), 2.0f * FH - 0.2f - FH, 0.9f, true);
	props::lockers(b, Vector3(4.7f, GROUND_TOP, A + 9.5f), -PI * 0.5f, 3);
	b.pop();

	b.push_yaw(Vector3(), 330.0f * DEG);
	props::pallets(b, Vector3(-3.0f, 0.0f, HUB_A + 4.5f), 0.3f, 4);
	props::barrel(b, Vector3(1.5f, 0.0f, HUB_A + 3.2f), "rust", false);
	props::barrel(b, Vector3(2.2f, 0.0f, HUB_A + 3.6f), "metal_blue", false);
	b.pop();

	props::car(b, Vector3(-71.0f, 0.05f, -16.0f), 0.0f, "car_white", false);
	props::car(b, Vector3(-75.5f, 0.05f, 40.0f), PI + 0.03f, "car_red", false);
	props::car(b, Vector3(40.0f, 0.0f, -49.0f), 1.3f, "car_silver", false);
	props::trash_container(b, Vector3(29.0f, 0.0f, -52.5f), 0.0f);
	props::lightning_rod(b, Vector3(0.0f, ROOF, 0.0f), 2.5f);
	k.fog(Vector3(-82.0f, 0.3f, 0.0f), Vector3(8.0f, 0.2f, 70.0f), 0.8f);
	k.fog(Vector3(0.0f, 0.25f, 55.0f), Vector3(50.0f, 0.15f, 5.0f), 0.8f);
	k.leaves(Vector3(-55.0f, 6.0f, 0.0f), Vector3(4.0f, 2.0f, 50.0f), 60);
	k.leaves(Vector3(0.0f, 6.0f, 55.0f), Vector3(50.0f, 2.0f, 4.0f), 60);

	Vector3 booth(34.0f, 0.0f, -54.0f);
	k.flicker(booth + Vector3(-2.35f, 2.6f, 0.9f), Color(0.85f, 0.95f, 1.0f), 7.0f, 1.3f, FxFlicker::MODE_FLUORESCENT, "booth", true);
	k.sparks(booth + Vector3(2.25f, 2.25f, 0.6f), "booth", 3.0f, 9.0f);
	props::hanging_cable(b, booth + Vector3(2.2f, 2.3f, 0.6f), Vector3(45.0f, 6.3f, -67.5f), 1.2f);
	k.beacon(booth + Vector3(1.6f, 3.05f, -1.4f), Color(1.0f, 0.5f, 0.05f), Color(1.0f, 0.5f, 0.05f), false, false);

	props::tec_chimney(b, Vector3(190.0f, 0.0f, -150.0f), 140.0f, 7.0f);
	fx::smoke_plume(b.dynamic_root, k.g.get_materials(), Vector3(190.0f, 141.0f, -150.0f), 4.0f, Vector3(0.6f, 0.25f, 0.3f), Color(0.55f, 0.55f, 0.58f, 0.45f));
	props::tec_chimney(b, Vector3(215.0f, 0.0f, -130.0f), 120.0f, 6.0f);
	fx::smoke_plume(b.dynamic_root, k.g.get_materials(), Vector3(215.0f, 121.0f, -130.0f), 3.5f, Vector3(0.6f, 0.25f, 0.3f), Color(0.55f, 0.55f, 0.58f, 0.4f));
}

}

void build_hospital(LevelBuilder &b, LevelData &d, UrbexGame &g) {
	Kit k(b, d, g);
	d.id = "hospital";
	d.title = "ХЗБ: заброшенная больница"_u;
	d.subtitle = "Ночь. Периметр под охраной ЧОП"_u;
	d.briefing = "[b]Объект:[/b] недостроенный больничный комплекс, три корпуса звездой. В паблике его зовут «Амбрелла».\n\n"_u
				 "[b]Охрана:[/b] вахтёр на КПП у ворот, обходчик с фонарём по периметру и ещё один внутри. На воротах камера, на главном входе свежий ИК-барьер, в коридорах пара датчиков движения. Питание всей этой электроники идёт от щитка на задней стене КПП.\n\n"_u
				 "[b]Нужно снять:[/b] граффити «AMBRELLA» в центральном корпусе на втором этаже, шахту лифта в западном корпусе и панораму с крыши. На крышу ведёт лестница западного корпуса.\n\n"_u
				 "[b]Вход:[/b] дыра в заборе прямо перед тобой, через забор можно перелезть на севере. Восточный забор новый, на нём вибродатчик.\n\n"_u
				 "Выход — обратно к дороге на западе."_u;
	d.exit_hint = "к дороге за западным забором"_u;
	d.spawn_position = Vector3(-75.0f, 0.1f, 22.0f);
	d.spawn_yaw = -PI * 0.5f;
	d.exit_zone = AABB(Vector3(-95.0f, -1.0f, -70.0f), Vector3(31.5f, 8.0f, 140.0f));
	d.ambient_exposure = 0.24f;
	d.indoor_exposure = 0.08f;
	d.kill_height = -12.0f;
	d.gbr_delay = 55.0f;
	d.gbr_spawn = Vector3(23.0f, 0.1f, -58.0f);
	d.gbr_van = true;
	d.gbr_van_position = Vector3(19.0f, 0.05f, -68.0f);
	d.gbr_van_yaw = -PI * 0.5f + 0.15f;
	d.ambience = Ambience::NightOutdoor;
	d.nav_bounds = AABB(Vector3(-66.0f, -5.0f, -66.0f), Vector3(132.0f, 22.0f, 132.0f));
	d.legal_note = "Ховринская больница строилась с 1980 года, была заморожена в 1992-м и снесена в 2018-м. Уровень сделан по мотивам, планировка упрощена."_u;

	k.objective_photo("graffiti", "Сфоткать граффити «AMBRELLA» (центр, 2 этаж)"_u);
	k.objective_photo("shaft", "Сфоткать шахту лифта (западный корпус)"_u);
	k.objective_photo("roof", "Сфоткать панораму с крыши: телебашня"_u);
	k.objective_photo("flooded", "Затопленный подвал в шахте лифта"_u, true);
	k.objective_item("medcard", "Найти старую медкарту"_u, true);

	const float A = HUB_A;
	const float shaft_x0 = -(A + 21.5f);
	b.slab(-220.0f, -220.0f, 220.0f, 220.0f, 0.0f, 0.5f, "ground", { Rect2(shaft_x0, -4.6f, 3.0f, 2.8f) });

	b.box(Vector3(-73.0f, 0.02f, 0.0f), Vector3(14.0f, 0.06f, 440.0f), "asphalt");
	b.box(Vector3(0.0f, 0.02f, -73.0f), Vector3(440.0f, 0.06f, 14.0f), "asphalt");

	build_hub(k);

	WingSpec wa;
	wa.angle = 270.0f * DEG;
	wa.stair_top = FLOORS;
	wa.shaft = true;
	wa.breach_side = 1;
	wa.breach_offset = 17.5f;
	wa.holes.push_back({ 2, Rect2(-1.2f, A + 25.0f, 2.4f, 2.0f) });
	wa.missing_partitions.push_back({ 1, 17.5f });
	build_wing(k, wa);

	WingSpec wb;
	wb.angle = 30.0f * DEG;
	wb.holes.push_back({ 1, Rect2(-4.6f, A + 11.0f, 2.8f, 3.0f) });
	wb.holes.push_back({ 2, Rect2(-1.4f, A + 18.0f, 6.0f, 5.0f) });
	wb.missing_partitions.push_back({ 0, -10.0f });
	build_wing(k, wb);

	WingSpec wc;
	wc.angle = 150.0f * DEG;
	wc.end_door = true;
	wc.holes.push_back({ 1, Rect2(-1.3f, A + 20.0f, 2.6f, 2.5f) });
	wc.holes.push_back({ 2, Rect2(1.6f, A + 13.0f, 3.2f, 3.0f) });
	build_wing(k, wc);

	b.push_yaw(Vector3(), wa.angle);
	b.wall(Vector3(-4.8f, 0, A + 18.5f), Vector3(-1.6f, 0, A + 18.5f), -3.6f, 3.6f, 0.2f, "concrete");
	b.wall(Vector3(-4.8f, 0, A + 21.5f), Vector3(-1.6f, 0, A + 21.5f), -3.6f, 3.6f, 0.2f, "concrete");
	b.wall(Vector3(-4.7f, 0, A + 18.5f), Vector3(-4.7f, 0, A + 21.5f), -3.6f, 3.6f, 0.2f, "concrete");
	b.wall(Vector3(-1.7f, 0, A + 18.5f), Vector3(-1.7f, 0, A + 21.5f), -3.6f, 3.6f, 0.2f, "concrete");
	b.box(Vector3(-3.2f, -3.75f, A + 20.0f), Vector3(3.2f, 0.3f, 3.2f), "concrete_floor");
	b.visual_box(Vector3(-3.2f, -3.0f, A + 20.0f), Vector3(2.8f, 0.02f, 2.9f), "water", Basis(), false);
	for (int i = 0; i < 9; i++) {
		b.visual_box(Vector3(-1.86f, -3.2f + float(i) * 0.4f, A + 19.0f), Vector3(0.06f, 0.03f, 0.4f), "rust");
	}
	ClimbPoint *pit_ladder = k.climb(Vector3(-2.0f, -2.8f, A + 19.0f), Vector3(0.8f, 1.6f, 1.0f), "Вылезти из шахты по скобам"_u, b.g(Vector3(-0.6f, 0.15f, A + 16.2f)), b.gyaw(0.0f), 1.6f);
	pit_ladder->set_noise(4.0f, "step_metal_0");
	k.spot(Vector3(-3.2f, 3.0f, A + 20.0f), "shaft", "Шахта лифта"_u, 10.0f);
	k.spot(Vector3(-3.2f, -2.9f, A + 20.0f), "flooded", "Затопленный подвал"_u, 12.0f);
	k.hint(Vector3(-0.2f, 1.0f, A + 16.0f), Vector3(2.5f, 2.0f, 3.0f), "Открытая шахта лифта за дверным проёмом. Не подходи к краю."_u);
	k.pir(Vector3(CW - 0.15f, 2.9f, A + 9.0f), Vector3(-CW, 0.6f, A + 16.0f), 9.0f, "booth", false, "Датчик движения в западном корпусе"_u);
	k.pir(Vector3(-CW + 0.15f, 3.6f + 2.9f, A + 4.0f), Vector3(0, 4.2f, A + 12.0f), 8.0f, "", true, "Старый датчик"_u);
	NoteBoard *note = k.board(Vector3(CW + 0.12f, 7.2f + 1.5f, A + 2.5f), -PI * 0.5f, "Записка руфера"_u,
			"Крыша — по этой лестнице, ещё один пролёт вверх, дальше дверь в будке.\n\nСнимай телебашню на северо-западе. Внизу над шахтой лифта не ходи, там пол на честном слове.\n\nИ не свети с крыши фонарём во двор: обходчик замечает свет издалека."_u,
			false);
	(void)note;
	k.noisy(Vector3(3.2f, 0.25f, A + 17.5f), Vector3(2.6f, 0.5f, 3.0f));
	Door *roof_door = k.door(Vector3(CW, ROOF, A + 0.75f), -PI * 0.5f, Door::STYLE_METAL, 1.0f, 2.05f, "Выход на кровлю"_u);
	(void)roof_door;
	b.pop();

	b.push_yaw(Vector3(), 90.0f * DEG);
	b.graffiti(Vector3(0, FH + 1.9f, HUB_A - 0.17f), Vector3(0, 0, -1), "AMBRELLA", 1.0f)->set_modulate(Color(0.85f, 0.1f, 0.08f));
	b.graffiti(Vector3(0, FH + 1.1f, HUB_A - 0.17f), Vector3(0, 0, -1), "1980 · 1992 · 2018"_u, 0.35f);
	k.spot(Vector3(0, FH + 1.7f, HUB_A - 0.45f), "graffiti", "Граффити «AMBRELLA»"_u, 14.0f);
	b.graffiti(Vector3(0.0f, 1.7f, HUB_A - 0.17f), Vector3(0, 0, -1), "ХЗБ"_u, 0.7f);
	b.graffiti(Vector3(0.0f, 2 * FH + 1.6f, HUB_A - 0.17f), Vector3(0, 0, -1), "НИМОСТОР"_u, 0.6f);
	b.pop();

	b.push_yaw(Vector3(), 210.0f * DEG);
	k.beam(Vector3(-1.4f, 1.1f, HUB_A - 0.6f), Vector3(1.4f, 1.1f, HUB_A - 0.6f), GROUND_TOP, "booth", "ИК-барьер на главном входе"_u);
	k.noisy(Vector3(0, 0.25f, HUB_A - 2.0f), Vector3(3.0f, 0.5f, 2.0f));
	k.hint(Vector3(0, 1.0f, HUB_A + 3.0f), Vector3(6.0f, 2.0f, 5.0f), "На входе свежий ИК-барьер: красная нить на высоте груди. Пригнись (C) и проползи под ней."_u);
	b.graffiti(Vector3(-3.5f, 1.8f, HUB_A + 0.17f), Vector3(0, 0, 1), "Сталкеры были здесь"_u, 0.35f);
	b.pop();

	b.push_yaw(Vector3(), 330.0f * DEG);
	k.camera(Vector3(3.0f, 3.2f, HUB_A + 0.3f), PI, 0.0f, -20.0f, true, "", "Муляж камеры"_u);
	k.board(Vector3(-2.0f, 1.6f, HUB_A - 0.17f), 0.0f, "Объявление"_u,
			"Территория охраняется ЧОП «Рубеж».\nВедётся видеонаблюдение.\nПроход запрещён.\n\n[color=#a0a49f]Ниже кто-то дописал маркером: «камера на углу — муляж, не крутится»[/color]"_u,
			false);
	b.pop();

	b.push_yaw(Vector3(), 30.0f * DEG);
	k.pickup(Vector3(-3.0f, FH, A + 22.0f), "medcard", "Медкарта 1984 года"_u, "Пожелтевшая карта из регистратуры, которой так и не было"_u, true, "paper");
	k.pir(Vector3(-CW + 0.15f, 2.9f, A + 6.0f), Vector3(0, 0.6f, A + 14.0f), 8.0f, "", true, "Старый датчик"_u);
	k.pir(Vector3(CW - 0.15f, 2.9f, A + 20.0f), Vector3(0, 0.6f, A + 26.0f), 8.0f, "", true, "Старый датчик"_u);
	b.graffiti(Vector3(-CW - 0.12f, 1.6f, A + 12.0f), Vector3(-1, 0, 0), "Цой жив"_u, 0.5f);
	b.graffiti(Vector3(CW + 0.12f, FH + 1.6f, A + 6.0f), Vector3(1, 0, 0), "не ходи на 3 этаж"_u, 0.35f);
	b.pop();

	b.push_yaw(Vector3(), 150.0f * DEG);
	k.pickup(Vector3(3.0f, GROUND_TOP, A + 15.0f), "batteries", "Батарейки"_u, "Свежие батарейки. Фонарь снова как новый"_u, false, "batteries");
	k.pickup(Vector3(3.2f, 2 * FH, A + 24.0f), "helmet", "Каска строителя"_u, "Оранжевая каска с выцарапанным «1985»"_u, true, "helmet");
	k.pir(Vector3(-CW + 0.15f, 2.9f, A + 10.0f), Vector3(0.0f, 0.6f, A + 1.0f), 9.0f, "booth", false, "Датчик движения в южном корпусе"_u);
	k.noisy(Vector3(0, 0.25f, A + WING_L - 1.5f), Vector3(2.6f, 0.5f, 2.5f));
	b.graffiti(Vector3(CW + 0.12f, 1.5f, A + 22.0f), Vector3(1, 0, 0), "Лёха + Катя 2009"_u, 0.35f);
	b.graffiti(Vector3(0.0f, 2.0f, A + WING_L - 0.17f), Vector3(0, 0, -1), "ВЫХОД"_u, 0.6f);
	b.pop();

	const float F = 62.0f;
	b.fence(Vector3(-F, 0, -F), Vector3(-F, 0, F), 2.6f, "fence", { Opening{ 82.0f, 1.3f, 0.0f, 2.6f } });
	b.fence(Vector3(-F, 0, F), Vector3(F, 0, F), 2.6f, "fence");
	b.fence(Vector3(F, 0, F), Vector3(F, 0, -F), 2.6f, "fence_gray");
	b.fence(Vector3(F, 0, -F), Vector3(-F, 0, -F), 2.6f, "fence", { Opening{ 39.0f, 6.0f, 0.0f, 2.6f } });
	Door *gate = k.door(Vector3(23.0f, 0.0f, -F), 0.0f, Door::STYLE_GATE, 6.0f, 2.4f, "Ворота"_u);
	gate->set_jammed(true, "Ворота закрыты на цепь с замком"_u);

	k.hint(Vector3(-66.0f, 1.0f, 20.0f), Vector3(6.0f, 2.0f, 6.0f), "Дыра в заборе. Пригнись: по периметру ходит обходчик с фонарём."_u);
	ClimbPoint *north_in = k.climb(Vector3(-40.0f, 1.2f, F + 0.4f), Vector3(1.6f, 2.4f, 0.6f), "Перелезть через забор"_u, Vector3(-40.0f, 0.2f, F - 1.2f), PI, 1.2f);
	north_in->set_noise(7.0f, "fence");
	ClimbPoint *north_out = k.climb(Vector3(-40.0f, 1.2f, F - 0.4f), Vector3(1.6f, 2.4f, 0.6f), "Перелезть через забор"_u, Vector3(-40.0f, 0.2f, F + 1.2f), 0.0f, 1.2f);
	north_out->set_noise(7.0f, "fence");
	ClimbPoint *east_in = k.climb(Vector3(F + 0.4f, 1.2f, 0.0f), Vector3(0.6f, 2.4f, 1.6f), "Перелезть через новый забор"_u, Vector3(F - 1.2f, 0.2f, 0.0f), PI * 0.5f, 1.2f);
	east_in->set_noise(7.0f, "fence");
	east_in->set_vibration_sensor("Вибродатчик «Шорох» на восточном заборе"_u);
	ClimbPoint *east_out = k.climb(Vector3(F - 0.4f, 1.2f, 0.0f), Vector3(0.6f, 2.4f, 1.6f), "Перелезть через новый забор"_u, Vector3(F + 1.2f, 0.2f, 0.0f), -PI * 0.5f, 1.2f);
	east_out->set_noise(7.0f, "fence");
	east_out->set_vibration_sensor("Вибродатчик «Шорох» на восточном заборе"_u);
	b.visual_box(Vector3(F + 0.06f, 1.4f, 0.8f), Vector3(0.05f, 0.12f, 0.08f), "plastic_white");

	Vector3 booth(34.0f, 0.0f, -54.0f);
	build_booth(k, booth);
	k.power_box(booth + Vector3(2.12f, 1.3f, 0.6f), -PI * 0.5f, "booth", "щиток КПП (ИК-барьер, датчики, камера, свет)"_u);
	k.camera(booth + Vector3(-2.1f, 2.6f, -1.6f), yaw_towards(booth + Vector3(-2.1f, 0.0f, -1.6f), Vector3(23.0f, 0.0f, -62.0f)), 35.0f, -18.0f, false, "booth", "Камера на воротах"_u);
	SpotLight3D *flood = b.spot(booth + Vector3(-1.5f, 3.2f, -1.0f), Vector3(23.0f, 0.0f, -60.0f), Color(1.0f, 0.95f, 0.85f), 30.0f, 3.0f, 35.0f, true);
	g.register_light(flood, "booth");
	k.probe(Vector3(23.0f, 1.0f, -57.0f), 10.0f, 0.95f, "booth");

	for (float z : { -40.0f, -5.0f, 30.0f }) {
		k.street_lamp(Vector3(-67.5f, 0.0f, z), PI * 0.5f);
	}
	for (float x : { -30.0f, 10.0f, 45.0f }) {
		k.street_lamp(Vector3(x, 0.0f, -67.5f), 0.0f);
	}
	k.board(Vector3(-74.0f, 0.0f, 25.5f), 0.0f, "Ховринская больница"_u,
			"Строительство больничного комплекса на 1300 мест началось в 1980 году. Работы много раз останавливали и окончательно заморозили в 1992-м.\n\nЗдание стоит на болотистом месте у реки Лихоборки, подвалы со временем затопило грунтовыми водами. Форма — три луча-корпуса звездой, похожая на знак биологической опасности, поэтому сталкеры прозвали его «Амбреллой». Другое название — «Нимостор» — пошло из городских легенд.\n\nВ 2009 году территорию обнесли забором с колючей проволокой, с 2011-го здесь постоянная охрана. В 2018 году комплекс снесли.\n\n[color=#a0a49f]Уровень — вольная реконструкция по мотивам.[/color]"_u,
			true);

	Rng tr(99);
	for (int i = 0; i < 46; i++) {
		Vector3 p(tr.range(-58.0f, 58.0f), 0.0f, tr.range(-58.0f, 58.0f));
		if (p.length() < 46.0f) {
			continue;
		}
		if (std::fabs(p.x - 23.0f) < 6.0f && p.z < -48.0f) {
			continue;
		}
		if ((p - booth).length() < 6.0f) {
			continue;
		}
		if (tr.chance(0.6f)) {
			props::birch(b, p, tr.range(8.0f, 13.0f), uint32_t(i) * 13u + 7u);
		} else {
			b.tree(p, tr.range(7.0f, 12.0f));
		}
	}
	for (int i = 0; i < 14; i++) {
		float a = float(i) / 14.0f * 2.0f * PI;
		float r = 150.0f + tr.range(-20.0f, 40.0f);
		Vector3 c(std::cos(a) * r, 0.0f, std::sin(a) * r);
		b.block_building(c, Vector3(tr.range(14.0f, 22.0f), tr.range(27.0f, 50.0f), tr.range(30.0f, 70.0f)));
	}

	Vector3 tower(-210.0f, 0.0f, 300.0f);
	b.cylinder(tower, 14.0f, 40.0f, "concrete", false, 10);
	b.cylinder(tower + Vector3(0, 40.0f, 0), 6.5f, 230.0f, "concrete", false, 12);
	b.cylinder(tower + Vector3(0, 270.0f, 0), 9.0f, 18.0f, "metal_gray", false, 12);
	b.cylinder(tower + Vector3(0, 288.0f, 0), 3.5f, 60.0f, "plaster", false, 8);
	b.cylinder(tower + Vector3(0, 348.0f, 0), 1.4f, 70.0f, "red_paint", false, 6);
	for (float y : { 120.0f, 200.0f, 279.0f, 340.0f, 418.0f }) {
		b.sphere(tower + Vector3(0, y, 0), 2.6f, "red_light");
	}
	PhotoSpot *roof_spot = k.spot(tower + Vector3(0, 279.0f, 0), "roof", "Панорама с крыши: телебашня"_u, 700.0f);
	roof_spot->set_zone(AABB(Vector3(-50.0f, ROOF - 0.2f, -50.0f), Vector3(100.0f, 4.0f, 100.0f)));
	d.hints.push_back(HintZone{ AABB(Vector3(-50.0f, ROOF - 0.2f, -50.0f), Vector3(100.0f, 4.0f, 100.0f)), "Крыша. Подними камеру (ПКМ) и поймай в кадр телебашню на северо-западе. К краю не подходи."_u, false });

	Guard *perimeter = k.guard(Guard::KIND_CHOP, "Обходчик ЧОП"_u, Vector3(-54.0f, 0.2f, -40.0f), true);
	perimeter->add_route_point(Vector3(-54.0f, 0.0f, -54.0f), 3.0f);
	perimeter->add_route_point(Vector3(-54.0f, 0.0f, 54.0f), 2.0f);
	perimeter->add_route_point(Vector3(50.0f, 0.0f, 54.0f), 3.0f);
	perimeter->add_route_point(Vector3(54.0f, 0.0f, -20.0f), 2.0f);
	perimeter->add_route_point(Vector3(38.0f, 0.0f, -46.0f), 4.0f);
	perimeter->add_route_point(Vector3(0.0f, 0.0f, -50.0f), 2.0f);

	b.push_yaw(Vector3(), wa.angle);
	Vector3 wa_corr0 = b.g(Vector3(0.0f, GROUND_TOP, A + 12.0f));
	Vector3 wa_corr1 = b.g(Vector3(0.0f, FH, A + 10.0f));
	Vector3 wa_corr2 = b.g(Vector3(0.0f, 2 * FH, A + 8.0f));
	b.pop();
	b.push_yaw(Vector3(), wb.angle);
	Vector3 wb_corr0 = b.g(Vector3(0.0f, GROUND_TOP, A + 14.0f));
	Vector3 wb_corr1 = b.g(Vector3(0.0f, FH, A + 9.0f));
	b.pop();
	b.push_yaw(Vector3(), wc.angle);
	Vector3 wc_corr0 = b.g(Vector3(0.0f, GROUND_TOP, A + 14.0f));
	b.pop();

	Guard *inside = k.guard(Guard::KIND_CHOP, "Охранник ЧОП"_u, Vector3(0.0f, 0.3f, -2.0f), true);
	inside->add_route_point(Vector3(0.0f, GROUND_TOP, -2.0f), 4.0f);
	inside->add_route_point(wa_corr0, 2.0f);
	inside->add_route_point(wa_corr1, 2.0f);
	inside->add_route_point(Vector3(1.5f, FH, 1.5f), 4.0f);
	inside->add_route_point(wb_corr1, 3.0f);
	inside->add_route_point(Vector3(-1.5f, FH, -1.0f), 2.0f);
	inside->add_route_point(wc_corr0, 3.0f);

	Guard *watchman = k.guard(Guard::KIND_WATCHMAN, "Вахтёр на КПП"_u, booth + Vector3(0.4f, 0.15f, -0.2f), false);
	watchman->set_post(booth + Vector3(0.4f, 0.15f, -0.2f), { yaw_towards(booth, Vector3(23.0f, 0, -62.0f)), yaw_towards(booth, Vector3(0, 0, 0)) }, 7.0f, true);
	watchman->set_vision(13.0f, 100.0f);

	hospital_dressing(k);

	d.gbr_search_points = { Vector3(0.0f, GROUND_TOP, 0.0f), wa_corr0, wb_corr0, wc_corr0, Vector3(0.0f, FH, 0.0f), wa_corr2, Vector3(-50.0f, 0.0f, 0.0f), Vector3(30.0f, 0.0f, 40.0f) };
}

}
