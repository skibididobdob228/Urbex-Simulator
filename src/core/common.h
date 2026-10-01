#pragma once

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <cstdint>

namespace urbex {

namespace layer {
constexpr uint32_t WORLD = 1u << 0;
constexpr uint32_t PLAYER = 1u << 1;
constexpr uint32_t DOORS = 1u << 2;
constexpr uint32_t GUARDS = 1u << 3;
constexpr uint32_t INTERACT = 1u << 4;
} // namespace layer

constexpr float PI = 3.14159265358979f;
constexpr float DEG = PI / 180.0f;

struct Rng {
	uint64_t state;

	explicit Rng(uint64_t seed = 0x9E3779B97F4A7C15ull) :
			state(seed ? seed : 1) {}

	uint64_t next() {
		state ^= state << 13;
		state ^= state >> 7;
		state ^= state << 17;
		return state;
	}

	float randf() { return float(next() >> 40) / float(1ull << 24); }
	float range(float a, float b) { return a + (b - a) * randf(); }
	int rangei(int a, int b) { return a + int(next() % uint64_t(b - a + 1)); }
	bool chance(float p) { return randf() < p; }
};

inline float approach(float from, float to, float delta) {
	if (std::fabs(to - from) <= delta) {
		return to;
	}
	return from + (to > from ? delta : -delta);
}

inline float clampf(float v, float lo, float hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

inline float lerpf(float a, float b, float t) {
	return a + (b - a) * t;
}

inline float wrap_angle(float a) {
	while (a > PI) {
		a -= 2.0f * PI;
	}
	while (a < -PI) {
		a += 2.0f * PI;
	}
	return a;
}

inline float approach_angle(float from, float to, float delta) {
	float diff = wrap_angle(to - from);
	if (std::fabs(diff) <= delta) {
		return to;
	}
	return wrap_angle(from + (diff > 0 ? delta : -delta));
}

inline float yaw_towards(const godot::Vector3 &from, const godot::Vector3 &to) {
	godot::Vector3 d = to - from;
	return std::atan2(-d.x, -d.z);
}

inline godot::Vector3 yaw_forward(float yaw) {
	return godot::Vector3(-std::sin(yaw), 0.0f, -std::cos(yaw));
}

inline godot::Vector3 flat(const godot::Vector3 &v) {
	return godot::Vector3(v.x, 0.0f, v.z);
}

inline godot::String operator""_u(const char *text, size_t length) {
	return godot::String::utf8(text, int(length));
}

godot::String item_display_name(const godot::String &id);

} // namespace urbex
