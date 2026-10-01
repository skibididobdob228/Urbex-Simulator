#include "core/sound_bank.h"

#include "core/common.h"

#include <godot_cpp/variant/packed_byte_array.hpp>

#include <algorithm>
#include <cmath>

using namespace godot;

namespace urbex {

namespace {

constexpr int RATE = 22050;
constexpr float TAU = 6.28318530718f;

int samples_for(float seconds) {
	return int(seconds * float(RATE));
}

struct OnePole {
	float a = 0.0f;
	float y = 0.0f;
	explicit OnePole(float cutoff) { a = 1.0f - std::exp(-TAU * cutoff / float(RATE)); }
	float lp(float x) {
		y += a * (x - y);
		return y;
	}
	float hp(float x) { return x - lp(x); }
};

float noise(Rng &rng) {
	return rng.randf() * 2.0f - 1.0f;
}

void normalize(std::vector<float> &s, float peak) {
	float m = 0.0001f;
	for (float v : s) {
		m = std::max(m, std::fabs(v));
	}
	float k = peak / m;
	for (float &v : s) {
		v *= k;
	}
}

std::vector<float> step_sound(uint32_t seed, float cutoff, float decay, bool crunchy) {
	Rng rng(seed * 7919 + 13);
	std::vector<float> s(samples_for(crunchy ? 0.22f : 0.13f));
	OnePole lp(cutoff);
	OnePole lp2(cutoff * 0.6f);
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float env = std::min(1.0f, t / 0.004f) * std::exp(-t * decay);
		float n = lp2.lp(lp.lp(noise(rng)));
		float thump = std::sin(TAU * 75.0f * t) * std::exp(-t * 55.0f) * 0.5f;
		float v = n * env * 1.6f + thump;
		if (crunchy && rng.randf() < 0.012f) {
			v += noise(rng) * 0.8f * std::exp(-t * 12.0f);
		}
		s[i] = v;
	}
	normalize(s, 0.85f);
	return s;
}

std::vector<float> creak_sound(uint32_t seed, float base_freq, float length) {
	Rng rng(seed);
	std::vector<float> s(samples_for(length));
	OnePole lp(1600.0f);
	OnePole hp(180.0f);
	float phase = 0.0f;
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float f = base_freq + 60.0f * std::sin(TAU * 1.1f * t) + 30.0f * std::sin(TAU * 3.7f * t);
		phase += f / RATE;
		phase -= std::floor(phase);
		float saw = phase * 2.0f - 1.0f;
		float stick = std::sin(TAU * 26.0f * t) > 0.3f ? 1.0f : 0.35f;
		float env = std::min(1.0f, t / 0.06f) * std::min(1.0f, (length - t) / 0.15f);
		float v = hp.hp(lp.lp(saw * stick + noise(rng) * 0.15f)) * env;
		s[i] = v;
	}
	normalize(s, 0.7f);
	return s;
}

std::vector<float> impact_sound(uint32_t seed, const std::vector<float> &partials, float length, float decay, float noise_amount) {
	Rng rng(seed);
	std::vector<float> s(samples_for(length));
	OnePole lp(3000.0f);
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float v = 0.0f;
		for (size_t p = 0; p < partials.size(); p++) {
			v += std::sin(TAU * partials[p] * t) * std::exp(-t * decay * (1.0f + float(p) * 0.6f)) / float(p + 1);
		}
		v += lp.lp(noise(rng)) * noise_amount * std::exp(-t * 40.0f);
		s[i] = v;
	}
	normalize(s, 0.85f);
	return s;
}

std::vector<float> blast_door_sound() {
	Rng rng(404);
	float length = 3.2f;
	std::vector<float> s(samples_for(length));
	OnePole lp(320.0f);
	OnePole lp2(900.0f);
	float phase = 0.0f;
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float grind_env = std::min(1.0f, t / 0.3f) * (t < 2.6f ? 1.0f : std::max(0.0f, 1.0f - (t - 2.6f) / 0.1f));
		phase += (48.0f + 6.0f * std::sin(TAU * 0.7f * t)) / RATE;
		phase -= std::floor(phase);
		float rumble = lp.lp((phase * 2.0f - 1.0f) * 0.8f + noise(rng) * 0.6f);
		float squeal = std::sin(TAU * (1250.0f + 180.0f * std::sin(TAU * 2.3f * t)) * t) * 0.05f * (std::sin(TAU * 0.9f * t) > 0.2f ? 1.0f : 0.0f);
		float v = (rumble * 1.4f + lp2.lp(noise(rng)) * 0.3f + squeal) * grind_env;
		if (t >= 2.6f) {
			float tt = t - 2.6f;
			v += (std::sin(TAU * 52.0f * tt) * 0.9f + std::sin(TAU * 131.0f * tt) * 0.4f) * std::exp(-tt * 7.0f);
		}
		s[i] = v;
	}
	normalize(s, 0.9f);
	return s;
}

std::vector<float> shutter_sound() {
	Rng rng(77);
	std::vector<float> s(samples_for(0.16f));
	OnePole hp(2500.0f);
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float c1 = t < 0.006f ? 1.0f : 0.0f;
		float c2 = (t > 0.075f && t < 0.083f) ? 0.8f : 0.0f;
		s[i] = hp.hp(noise(rng)) * (c1 + c2) + std::sin(TAU * 3200.0f * t) * std::exp(-t * 90.0f) * 0.2f;
	}
	normalize(s, 0.8f);
	return s;
}

std::vector<float> tone(float freq, float length, float attack, float release, float volume) {
	std::vector<float> s(samples_for(length));
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float env = std::min(1.0f, t / attack) * std::min(1.0f, (length - t) / release);
		s[i] = std::sin(TAU * freq * t) * env * volume;
	}
	return s;
}

std::vector<float> sweep(float f0, float f1, float length, float volume) {
	std::vector<float> s(samples_for(length));
	float phase = 0.0f;
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float k = t / length;
		phase += (f0 + (f1 - f0) * k) / RATE;
		float env = std::min(1.0f, t / 0.01f) * std::min(1.0f, (length - t) / 0.05f);
		s[i] = (std::sin(TAU * phase) + 0.3f * std::sin(TAU * phase * 2.0f)) * env * volume;
	}
	return s;
}

std::vector<float> bell_loop() {
	Rng rng(5);
	float length = 1.5f;
	std::vector<float> s(samples_for(length));
	OnePole lp(6000.0f);
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float strike = std::fmod(t * 22.0f, 1.0f);
		float hit = std::exp(-strike * 9.0f);
		float tonev = std::sin(TAU * 1180.0f * t) + 0.6f * std::sin(TAU * 2950.0f * t) + 0.35f * std::sin(TAU * 4100.0f * t);
		s[i] = lp.lp(tonev * (0.35f + hit * 0.65f) + noise(rng) * 0.08f * hit);
	}
	normalize(s, 0.75f);
	return s;
}

std::vector<float> siren_loop() {
	float length = 3.0f;
	std::vector<float> s(samples_for(length));
	float phase = 0.0f;
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float f = 700.0f + 500.0f * (0.5f - 0.5f * std::cos(TAU * t / length));
		phase += f / RATE;
		phase -= std::floor(phase);
		float sq = phase < 0.5f ? 1.0f : -1.0f;
		s[i] = std::sin(TAU * phase) * 0.7f + sq * 0.15f;
	}
	normalize(s, 0.7f);
	return s;
}

std::vector<float> loop_crossfade(std::vector<float> s, int fade) {
	int n = int(s.size()) - fade;
	std::vector<float> out(n);
	for (int i = 0; i < n; i++) {
		out[i] = s[i];
	}
	for (int i = 0; i < fade; i++) {
		float k = float(i) / float(fade);
		out[i] = s[n + i] * (1.0f - k) + s[i] * k;
	}
	return out;
}

std::vector<float> wind_loop() {
	Rng rng(909);
	float length = 8.0f;
	int fade = samples_for(1.0f);
	std::vector<float> s(samples_for(length) + fade);
	float y1 = 0.0f;
	float y2 = 0.0f;
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float cutoff = 260.0f + 220.0f * std::sin(TAU * 0.13f * t) + 120.0f * std::sin(TAU * 0.37f * t + 1.3f);
		float a = 1.0f - std::exp(-TAU * cutoff / RATE);
		y1 += a * (noise(rng) - y1);
		y2 += a * (y1 - y2);
		float gust = 0.55f + 0.45f * std::sin(TAU * 0.11f * t + 0.5f * std::sin(TAU * 0.05f * t));
		s[i] = y2 * gust;
	}
	auto out = loop_crossfade(s, fade);
	normalize(out, 0.6f);
	return out;
}

std::vector<float> hum_loop() {
	std::vector<float> s(samples_for(1.0f));
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float v = std::sin(TAU * 50.0f * t) * 0.5f + std::sin(TAU * 100.0f * t) * 0.35f + std::sin(TAU * 150.0f * t) * 0.2f + std::sin(TAU * 250.0f * t) * 0.08f;
		s[i] = std::clamp(v * 1.4f, -0.8f, 0.8f);
	}
	normalize(s, 0.5f);
	return s;
}

std::vector<float> drip_sound() {
	std::vector<float> s(samples_for(0.22f));
	float phase = 0.0f;
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float f = 700.0f + 1400.0f * std::min(1.0f, t / 0.05f);
		phase += f / RATE;
		s[i] = std::sin(TAU * phase) * std::exp(-t * 28.0f) * std::min(1.0f, t / 0.002f);
	}
	normalize(s, 0.6f);
	return s;
}

std::vector<float> radio_sound() {
	Rng rng(1234);
	std::vector<float> s(samples_for(0.45f));
	OnePole hp(400.0f);
	OnePole lp(2800.0f);
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float env = std::min(1.0f, t / 0.01f) * (t < 0.36f ? 1.0f : std::max(0.0f, 1.0f - (t - 0.36f) / 0.03f));
		float v = lp.lp(hp.hp(noise(rng))) * env * 0.8f;
		if (t > 0.4f && t < 0.405f) {
			v += 0.6f;
		}
		s[i] = v;
	}
	normalize(s, 0.6f);
	return s;
}

std::vector<float> rattle_sound(uint32_t seed, float length, float density) {
	Rng rng(seed);
	std::vector<float> s(samples_for(length));
	std::vector<float> partials = { 310.0f, 520.0f, 870.0f, 1330.0f };
	std::vector<float> excitation(s.size(), 0.0f);
	for (size_t i = 0; i < s.size(); i++) {
		if (rng.randf() < density / RATE) {
			excitation[i] = rng.range(0.4f, 1.0f);
		}
	}
	excitation[0] = 1.0f;
	for (size_t p = 0; p < partials.size(); p++) {
		float r = std::exp(-1.0f / (0.05f * RATE));
		float w = TAU * partials[p] / RATE;
		float y1 = 0.0f;
		float y2 = 0.0f;
		float c = 2.0f * r * std::cos(w);
		for (size_t i = 0; i < s.size(); i++) {
			float y = excitation[i] + c * y1 - r * r * y2;
			y2 = y1;
			y1 = y;
			s[i] += y * 0.05f / float(p + 1);
		}
	}
	normalize(s, 0.75f);
	return s;
}

std::vector<float> lift_sound() {
	Rng rng(77);
	float length = 3.0f;
	std::vector<float> s(samples_for(length));
	OnePole lp(400.0f);
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float motor = (std::sin(TAU * 92.0f * t) * 0.4f + lp.lp(noise(rng)) * 0.5f) * std::min(1.0f, t / 0.4f) * (t < 2.3f ? 1.0f : std::max(0.0f, 1.0f - (t - 2.3f) / 0.2f));
		float ding = 0.0f;
		if (t > 2.4f) {
			float tt = t - 2.4f;
			ding = (std::sin(TAU * 880.0f * tt) + 0.5f * std::sin(TAU * 1320.0f * tt)) * std::exp(-tt * 5.0f) * 0.6f;
		}
		s[i] = motor + ding;
	}
	normalize(s, 0.75f);
	return s;
}

std::vector<float> heartbeat_loop() {
	std::vector<float> s(samples_for(0.9f));
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float v = 0.0f;
		for (float start : { 0.0f, 0.22f }) {
			if (t >= start) {
				float tt = t - start;
				v += std::sin(TAU * 52.0f * tt) * std::exp(-tt * 22.0f) * (start == 0.0f ? 1.0f : 0.7f);
			}
		}
		s[i] = v;
	}
	normalize(s, 0.8f);
	return s;
}

std::vector<float> switch_sound() {
	Rng rng(31);
	std::vector<float> s(samples_for(0.35f));
	OnePole lp(1200.0f);
	for (size_t i = 0; i < s.size(); i++) {
		float t = float(i) / RATE;
		float v = std::sin(TAU * 90.0f * t) * std::exp(-t * 30.0f);
		v += lp.lp(noise(rng)) * std::exp(-t * 80.0f);
		if (t > 0.05f) {
			float tt = t - 0.05f;
			v += std::sin(TAU * 1900.0f * tt) * std::exp(-tt * 60.0f) * 0.3f;
		}
		s[i] = v;
	}
	normalize(s, 0.8f);
	return s;
}

} // namespace

void SoundBank::store(const std::string &name, const std::vector<float> &samples, bool loop) {
	PackedByteArray data;
	data.resize(int64_t(samples.size() * 2));
	uint8_t *w = data.ptrw();
	for (size_t i = 0; i < samples.size(); i++) {
		int v = int(std::clamp(samples[i], -1.0f, 1.0f) * 32767.0f);
		int16_t sv = int16_t(v);
		w[i * 2 + 0] = uint8_t(sv & 0xFF);
		w[i * 2 + 1] = uint8_t((sv >> 8) & 0xFF);
	}
	Ref<AudioStreamWAV> wav;
	wav.instantiate();
	wav->set_format(AudioStreamWAV::FORMAT_16_BITS);
	wav->set_mix_rate(RATE);
	wav->set_stereo(false);
	wav->set_data(data);
	if (loop) {
		wav->set_loop_mode(AudioStreamWAV::LOOP_FORWARD);
		wav->set_loop_begin(0);
		wav->set_loop_end(int(samples.size()));
	}
	sounds[name] = wav;
}

void SoundBank::build() {
	if (built) {
		return;
	}
	built = true;

	for (int i = 0; i < 4; i++) {
		store("step_" + std::to_string(i), step_sound(uint32_t(i + 1), 900.0f + float(i) * 120.0f, 38.0f, false));
		store("step_glass_" + std::to_string(i), step_sound(uint32_t(i + 11), 2600.0f, 26.0f, true));
		store("step_metal_" + std::to_string(i), impact_sound(uint32_t(i + 21), { 420.0f + float(i) * 35.0f, 980.0f, 1710.0f }, 0.25f, 18.0f, 0.8f));
		store("drip_" + std::to_string(i), drip_sound());
	}
	variant_counts["step"] = 4;
	variant_counts["step_glass"] = 4;
	variant_counts["step_metal"] = 4;

	store("creak", creak_sound(3, 210.0f, 0.9f));
	store("creak_low", creak_sound(9, 140.0f, 1.3f));
	store("metal_door", impact_sound(41, { 110.0f, 237.0f, 410.0f, 690.0f }, 1.3f, 3.5f, 1.0f));
	store("blast_door", blast_door_sound());
	store("shutter", shutter_sound());
	store("beep", tone(2600.0f, 0.14f, 0.005f, 0.03f, 0.7f));
	store("beep_low", tone(1400.0f, 0.2f, 0.005f, 0.05f, 0.7f));
	store("pickup", sweep(600.0f, 1250.0f, 0.18f, 0.6f));
	store("bell", bell_loop(), true);
	store("siren", siren_loop(), true);
	store("wind", wind_loop(), true);
	store("hum", hum_loop(), true);
	store("drip", drip_sound());
	store("radio", radio_sound());
	store("fence", rattle_sound(51, 1.0f, 18.0f));
	store("cut", impact_sound(61, { 2100.0f, 3300.0f, 5200.0f }, 0.45f, 9.0f, 1.4f));
	store("lift", lift_sound());
	store("heartbeat", heartbeat_loop(), true);
	store("switch", switch_sound());
	store("land", impact_sound(71, { 60.0f, 95.0f }, 0.35f, 12.0f, 1.5f));
	store("click", impact_sound(81, { 3000.0f }, 0.05f, 80.0f, 0.5f));
	store("spray", step_sound(91, 5200.0f, 3.0f, false));
}

Ref<AudioStreamWAV> SoundBank::get(const std::string &name) const {
	auto it = sounds.find(name);
	return it != sounds.end() ? it->second : Ref<AudioStreamWAV>();
}

Ref<AudioStreamWAV> SoundBank::variant(const std::string &base, int index) const {
	auto it = variant_counts.find(base);
	if (it == variant_counts.end()) {
		return get(base);
	}
	int n = it->second;
	return get(base + "_" + std::to_string(((index % n) + n) % n));
}

} // namespace urbex
