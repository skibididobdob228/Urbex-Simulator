#include "core/materials.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <thread>
#include <vector>

using namespace godot;

namespace urbex {

namespace {

uint32_t hash2(int x, int y, uint32_t seed) {
	uint32_t h = seed * 374761393u + uint32_t(x) * 668265263u + uint32_t(y) * 2246822519u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

float hashf(int x, int y, uint32_t seed) {
	return float(hash2(x, y, seed) & 0xFFFFFF) / float(0xFFFFFF);
}

int wrap(int v, int period) {
	return ((v % period) + period) % period;
}

float smooth(float t) {
	return t * t * (3.0f - 2.0f * t);
}

float value_noise(float x, float y, int period, uint32_t seed) {
	int x0 = int(std::floor(x));
	int y0 = int(std::floor(y));
	float fx = smooth(x - float(x0));
	float fy = smooth(y - float(y0));
	float a = hashf(wrap(x0, period), wrap(y0, period), seed);
	float b = hashf(wrap(x0 + 1, period), wrap(y0, period), seed);
	float c = hashf(wrap(x0, period), wrap(y0 + 1, period), seed);
	float d = hashf(wrap(x0 + 1, period), wrap(y0 + 1, period), seed);
	float top = a + (b - a) * fx;
	float bottom = c + (d - c) * fx;
	return top + (bottom - top) * fy;
}

float fbm(float u, float v, int period, int octaves, uint32_t seed) {
	float sum = 0.0f;
	float amp = 0.5f;
	float norm = 0.0f;
	for (int i = 0; i < octaves; i++) {
		sum += amp * value_noise(u * float(period), v * float(period), period, seed + uint32_t(i) * 31u);
		norm += amp;
		amp *= 0.5f;
		period *= 2;
	}
	return sum / norm;
}

float warped(float u, float v, int period, int octaves, uint32_t seed, float strength) {
	float wu = fbm(u + 0.17f, v + 0.31f, 2, 3, seed + 900) - 0.5f;
	float wv = fbm(u + 0.53f, v + 0.11f, 2, 3, seed + 901) - 0.5f;
	return fbm(u + wu * strength, v + wv * strength, period, octaves, seed);
}

struct Cell {
	float f1 = 10.0f;
	float f2 = 10.0f;
	float id = 0.0f;
};

Cell worley(float u, float v, int period, uint32_t seed) {
	float x = u * float(period);
	float y = v * float(period);
	int cx = int(std::floor(x));
	int cy = int(std::floor(y));
	Cell c;
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			int gx = cx + dx;
			int gy = cy + dy;
			int wx = wrap(gx, period);
			int wy = wrap(gy, period);
			float px = float(gx) + hashf(wx, wy, seed);
			float py = float(gy) + hashf(wx, wy, seed + 7);
			float d = std::sqrt((px - x) * (px - x) + (py - y) * (py - y));
			if (d < c.f1) {
				c.f2 = c.f1;
				c.f1 = d;
				c.id = hashf(wx, wy, seed + 13);
			} else if (d < c.f2) {
				c.f2 = d;
			}
		}
	}
	return c;
}

float sstep(float a, float b, float x);

float cracks(float u, float v, int period, uint32_t seed, float width, float coverage) {
	float wu = (fbm(u, v, 3, 3, seed + 50) - 0.5f) * 0.08f;
	float wv = (fbm(u, v, 3, 3, seed + 51) - 0.5f) * 0.08f;
	Cell c = worley(u + wu, v + wv, period, seed);
	float edge = c.f2 - c.f1;
	float local_width = width * (0.4f + fbm(u, v, 12, 3, seed + 33));
	float line = 1.0f - std::clamp(edge / local_width, 0.0f, 1.0f);
	float mask = fbm(u, v, 2, 3, seed + 77);
	float m = std::clamp((mask - (1.0f - coverage)) * 6.0f, 0.0f, 1.0f);
	float breakup = sstep(0.42f, 0.58f, fbm(u, v, 9, 3, seed + 88));
	return line * line * line * m * breakup;
}

float sstep(float a, float b, float x) {
	float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

struct Rgb {
	float r, g, b;
};

Rgb mix(const Rgb &a, const Rgb &b, float t) {
	return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}

Rgb scale(const Rgb &c, float k) {
	return { c.r * k, c.g * k, c.b * k };
}

struct Pixel {
	Rgb color{ 0, 0, 0 };
	float alpha = 1.0f;
	float height = 0.5f;
	float rough = 0.9f;
};

struct Canvas {
	int size = 512;
	std::vector<Pixel> px;
	bool has_alpha = false;
	bool has_height = true;
	bool has_rough = false;

	explicit Canvas(int s = 512) :
			size(s), px(size_t(s * s)) {}

	template <typename F>
	void paint(F fn) {
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				float u = (float(x) + 0.5f) / float(size);
				float v = (float(y) + 0.5f) / float(size);
				fn(x, y, u, v, px[size_t(y * size + x)]);
			}
		}
	}
};

Ref<ImageTexture> make_texture(const Canvas &c) {
	int n = c.size * c.size;
	int ch = c.has_alpha ? 4 : 3;
	PackedByteArray data;
	data.resize(n * ch);
	uint8_t *w = data.ptrw();
	for (int i = 0; i < n; i++) {
		const Pixel &p = c.px[size_t(i)];
		w[i * ch + 0] = uint8_t(std::clamp(p.color.r, 0.0f, 1.0f) * 255.0f);
		w[i * ch + 1] = uint8_t(std::clamp(p.color.g, 0.0f, 1.0f) * 255.0f);
		w[i * ch + 2] = uint8_t(std::clamp(p.color.b, 0.0f, 1.0f) * 255.0f);
		if (ch == 4) {
			w[i * ch + 3] = uint8_t(std::clamp(p.alpha, 0.0f, 1.0f) * 255.0f);
		}
	}
	Ref<Image> img = Image::create_from_data(c.size, c.size, false, ch == 4 ? Image::FORMAT_RGBA8 : Image::FORMAT_RGB8, data);
	img->generate_mipmaps();
	return ImageTexture::create_from_image(img);
}

Ref<ImageTexture> make_normal(const Canvas &c, float strength) {
	int n = c.size * c.size;
	PackedByteArray data;
	data.resize(n);
	uint8_t *w = data.ptrw();
	for (int i = 0; i < n; i++) {
		w[i] = uint8_t(std::clamp(c.px[size_t(i)].height, 0.0f, 1.0f) * 255.0f);
	}
	Ref<Image> img = Image::create_from_data(c.size, c.size, false, Image::FORMAT_L8, data);
	img->bump_map_to_normal_map(strength);
	img->generate_mipmaps(true);
	return ImageTexture::create_from_image(img);
}

Ref<ImageTexture> make_rough(const Canvas &c) {
	int n = c.size * c.size;
	PackedByteArray data;
	data.resize(n);
	uint8_t *w = data.ptrw();
	for (int i = 0; i < n; i++) {
		w[i] = uint8_t(std::clamp(c.px[size_t(i)].rough, 0.0f, 1.0f) * 255.0f);
	}
	Ref<Image> img = Image::create_from_data(c.size, c.size, false, Image::FORMAT_L8, data);
	img->generate_mipmaps();
	return ImageTexture::create_from_image(img);
}

Canvas concrete_wall(uint32_t seed, float base, Rgb tint) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float n = warped(u, v, 4, 5, seed, 0.4f);
		float grain = fbm(u, v, 64, 2, seed + 3);
		float k = base + (n - 0.5f) * 0.2f + (grain - 0.5f) * 0.07f;
		float panel_v = std::fmod(v * 2.0f, 1.0f);
		float panel_u = std::fmod(u * 2.0f + float(int(v * 2.0f) % 2) * 0.5f, 1.0f);
		float seam = std::min(std::min(panel_v, 1.0f - panel_v), std::min(panel_u, 1.0f - panel_u));
		float seam_line = 1.0f - sstep(0.0f, 0.004f, seam);
		k -= seam_line * 0.12f;
		Cell pores = worley(u, v, 90, seed + 9);
		float pore = (1.0f - sstep(0.03f, 0.09f, pores.f1)) * (pores.id > 0.8f ? 1.0f : 0.0f);
		k -= pore * 0.1f;
		float tie_u = std::fmod(u * 4.0f, 1.0f) - 0.5f;
		float tie_v = std::fmod(v * 2.0f, 1.0f) - 0.25f;
		float tie = 1.0f - sstep(0.012f, 0.02f, std::sqrt(tie_u * tie_u * 0.25f + tie_v * tie_v));
		float below = (tie_v > 0.0f && std::fabs(tie_u) < 0.06f) ? std::exp(-tie_v * 6.0f) * sstep(0.35f, 0.6f, fbm(u * 3.0f, v * 0.3f, 8, 3, seed + 21)) : 0.0f;
		float streak = sstep(0.55f, 0.85f, fbm(u * 6.0f, v * 0.35f, 6, 3, seed + 41)) * sstep(0.2f, 0.9f, 1.0f - v * 0.6f);
		float crack = cracks(u, v, 6, seed + 61, 0.025f, 0.35f);
		float stain = sstep(0.6f, 0.85f, fbm(u, v, 2, 4, seed + 81));
		k -= streak * 0.1f + crack * 0.25f + stain * 0.08f;
		Rgb col = scale(tint, k);
		col = mix(col, Rgb{ 0.36f, 0.2f, 0.1f }, std::clamp(below * 0.6f + tie * 0.3f, 0.0f, 0.7f));
		p.color = col;
		p.height = 0.55f + (grain - 0.5f) * 0.25f - pore * 0.3f - crack * 0.5f - seam_line * 0.3f - tie * 0.4f;
		p.rough = 0.92f - stain * 0.15f;
	});
	return c;
}

Canvas concrete_floor(uint32_t seed, float base, float wetness, float moss) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int x, int y, float u, float v, Pixel &p) {
		float n = warped(u, v, 3, 5, seed, 0.6f);
		float grain = fbm(u, v, 80, 2, seed + 5);
		float k = base + (n - 0.5f) * 0.22f + (grain - 0.5f) * 0.08f;
		float agg = hashf(x / 2, y / 2, seed + 11);
		if (agg > 0.97f) {
			k += 0.08f;
		} else if (agg < 0.03f) {
			k -= 0.07f;
		}
		float crack = cracks(u, v, 4, seed + 31, 0.03f, 0.35f);
		float dirt = sstep(0.45f, 0.8f, fbm(u, v, 3, 5, seed + 71));
		float wet = sstep(0.62f, 0.72f, warped(u, v, 2, 4, seed + 91, 0.8f)) * wetness;
		k -= crack * 0.25f + dirt * 0.12f + wet * 0.1f;
		Rgb col = scale(Rgb{ 1.0f, 0.97f, 0.92f }, k);
		float m = sstep(0.55f, 0.8f, fbm(u, v, 4, 4, seed + 131)) * moss;
		col = mix(col, Rgb{ 0.16f, 0.22f, 0.1f }, m * 0.8f);
		p.color = col;
		p.height = 0.5f + (grain - 0.5f) * 0.3f - crack * 0.6f + (agg > 0.97f ? 0.1f : 0.0f);
		p.rough = std::clamp(0.88f - wet * 0.75f - dirt * 0.05f, 0.08f, 1.0f);
	});
	return c;
}

Canvas painted_plaster(uint32_t seed, Rgb paint, float peel, float gloss) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float mottle = warped(u, v, 3, 5, seed, 0.5f);
		float fine = fbm(u, v, 48, 2, seed + 9);
		float region = sstep(0.4f, 0.7f, fbm(u, v, 2, 4, seed + 29));
		float flake_field = warped(u, v, 6, 6, seed + 23, 0.9f) * 0.75f + fbm(u, v, 24, 3, seed + 24) * 0.25f;
		float thr = 0.74f - peel * 0.55f * region;
		float peeled = flake_field > thr ? 1.0f : 0.0f;
		float rim = peeled > 0.0f ? sstep(thr, thr + 0.025f, flake_field) : 1.0f - sstep(thr - 0.012f, thr, flake_field) * 0.6f;
		Rgb base = scale(paint, 0.92f + (mottle - 0.5f) * 0.16f + (fine - 0.5f) * 0.04f);
		Rgb under = scale(Rgb{ 0.62f, 0.6f, 0.56f }, 0.85f + (fine - 0.5f) * 0.3f);
		Rgb col = peeled > 0.0f ? mix(scale(under, 0.7f), under, rim) : scale(base, 0.75f + rim * 0.25f);
		float crack = cracks(u, v, 7, seed + 41, 0.018f, 0.3f);
		float dirt = sstep(0.55f, 0.95f, fbm(u, v, 2, 4, seed + 53)) * 0.22f;
		float scuff = sstep(0.7f, 0.95f, fbm(u * 0.5f, v * 6.0f, 6, 3, seed + 57)) * 0.08f;
		col = scale(col, 1.0f - dirt - scuff - crack * 0.35f);
		p.color = col;
		p.height = (peeled > 0.0f ? 0.35f : 0.6f) + (fine - 0.5f) * 0.12f - crack * 0.3f;
		p.rough = peeled > 0.0f ? 0.95f : gloss;
	});
	return c;
}

Canvas wall_tile(uint32_t seed) {
	Canvas c;
	c.has_rough = true;
	const float tiles = 8.0f;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float tu = u * tiles;
		float tv = v * tiles;
		int ix = int(tu);
		int iy = int(tv);
		float fx = tu - float(ix);
		float fy = tv - float(iy);
		float edge = std::min(std::min(fx, 1.0f - fx), std::min(fy, 1.0f - fy));
		bool grout = edge < 0.035f;
		float bevel = sstep(0.035f, 0.09f, edge);
		float tint = hashf(ix, iy, seed) * 0.06f;
		bool missing = hashf(ix, iy, seed + 5) > 0.95f;
		bool cracked = hashf(ix, iy, seed + 6) > 0.85f;
		float dirt = sstep(0.45f, 0.85f, fbm(u, v, 3, 4, seed + 11));
		if (missing) {
			float comb = 0.5f + 0.5f * std::sin((tu + tv) * 40.0f);
			float k = 0.42f + comb * 0.06f + (fbm(u, v, 32, 2, seed + 2) - 0.5f) * 0.1f;
			p.color = { k, k * 0.97f, k * 0.9f };
			p.height = 0.2f + comb * 0.05f;
			p.rough = 0.95f;
		} else if (grout) {
			p.color = scale(Rgb{ 0.5f, 0.48f, 0.43f }, 1.0f - dirt * 0.4f);
			p.height = 0.3f;
			p.rough = 0.95f;
		} else {
			Rgb col{ 0.86f - tint, 0.88f - tint, 0.86f - tint * 0.4f };
			float crack = 0.0f;
			if (cracked) {
				float a = hashf(ix, iy, seed + 8) * 3.14159f;
				float d = std::fabs((fx - 0.5f) * std::cos(a) + (fy - 0.5f) * std::sin(a) + (fbm(fx, fy, 4, 2, seed + uint32_t(ix)) - 0.5f) * 0.2f);
				crack = 1.0f - sstep(0.0f, 0.012f, d);
			}
			col = scale(col, 1.0f - dirt * 0.25f - crack * 0.5f);
			p.color = col;
			p.height = 0.5f + bevel * 0.3f - crack * 0.2f;
			p.rough = 0.12f + dirt * 0.3f;
		}
	});
	return c;
}

Canvas metlakh(uint32_t seed) {
	Canvas c;
	c.has_rough = true;
	const float tiles = 8.0f;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float tu = u * tiles;
		float tv = v * tiles;
		int ix = int(tu);
		int iy = int(tv);
		float fx = tu - float(ix);
		float fy = tv - float(iy);
		float edge = std::min(std::min(fx, 1.0f - fx), std::min(fy, 1.0f - fy));
		bool grout = edge < 0.04f;
		Rgb terracotta{ 0.52f, 0.24f, 0.16f };
		Rgb beige{ 0.78f, 0.7f, 0.52f };
		Rgb dark{ 0.2f, 0.18f, 0.16f };
		Rgb col = ((ix + iy) % 2 == 0) ? terracotta : beige;
		float cx = std::fabs(fx - 0.5f);
		float cy = std::fabs(fy - 0.5f);
		if (cx + cy < 0.16f) {
			col = dark;
		}
		col = scale(col, 0.9f + hashf(ix, iy, seed) * 0.15f);
		float wear = sstep(0.4f, 0.8f, fbm(u, v, 2, 4, seed + 5));
		float dirt = sstep(0.5f, 0.9f, fbm(u, v, 6, 3, seed + 9));
		col = mix(col, Rgb{ 0.35f, 0.32f, 0.28f }, wear * 0.35f + dirt * 0.2f);
		if (grout) {
			col = { 0.3f, 0.28f, 0.25f };
		}
		bool chipped = hashf(ix, iy, seed + 3) > 0.93f && fbm(fx, fy, 3, 2, seed + uint32_t(ix) * 7u) > 0.55f;
		if (chipped && !grout) {
			col = { 0.38f, 0.37f, 0.34f };
		}
		p.color = col;
		p.height = grout ? 0.3f : (chipped ? 0.35f : 0.55f);
		p.rough = grout ? 0.95f : 0.45f + wear * 0.35f;
	});
	return c;
}

Canvas linoleum(uint32_t seed) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float tv = v * 16.0f;
		int iy = int(tv);
		float tu = u * 4.0f + float(iy % 2) * 0.5f;
		int ix = int(tu);
		float plank = hashf(ix, iy, seed);
		float grain = fbm(u * 2.0f, v * 30.0f, 4, 3, seed + 3);
		Rgb col = scale(Rgb{ 0.42f, 0.24f, 0.15f }, 0.85f + plank * 0.2f + (grain - 0.5f) * 0.2f);
		float fx = tu - float(ix);
		float fy = tv - float(iy);
		if (fx < 0.01f || fy < 0.04f) {
			col = scale(col, 0.75f);
		}
		float fade = sstep(0.35f, 0.75f, fbm(u, v, 2, 4, seed + 7));
		col = mix(col, Rgb{ 0.5f, 0.4f, 0.3f }, fade * 0.4f);
		float scuff = sstep(0.75f, 0.95f, fbm(u * 3.0f, v * 0.6f, 8, 3, seed + 11));
		col = scale(col, 1.0f - scuff * 0.2f);
		Cell tear = worley(u, v, 5, seed + 13);
		bool torn = tear.id > 0.88f && tear.f1 < 0.3f;
		if (torn) {
			col = { 0.32f, 0.31f, 0.29f };
		}
		p.color = col;
		p.height = torn ? 0.3f : 0.55f + (grain - 0.5f) * 0.05f;
		p.rough = torn ? 0.95f : 0.35f + scuff * 0.3f;
	});
	return c;
}

Canvas brick(uint32_t seed, Rgb a, Rgb b, Rgb mortar, float soot) {
	Canvas c;
	const float cols = 4.0f;
	const float rows = 14.0f;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float rv = v * rows;
		int row = int(rv);
		float ru = u * cols + float(row % 2) * 0.5f;
		int col_i = int(ru);
		float fx = ru - float(col_i);
		float fy = rv - float(row);
		float edge = std::min(std::min(fx, 1.0f - fx) * 3.8f, std::min(fy, 1.0f - fy));
		bool is_mortar = fx < 0.025f || fy < 0.09f;
		float n = fbm(u, v, 24, 3, seed);
		float chip = worley(u, v, 40, seed + 4).f1;
		bool chipped = chip < 0.18f && hashf(col_i, row, seed + 9) > 0.6f && edge < 0.25f;
		float salt = sstep(0.6f, 0.85f, fbm(u, v, 3, 4, seed + 19)) * (1.0f - std::min(1.0f, edge * 6.0f));
		Rgb col;
		if (is_mortar) {
			col = scale(mortar, 0.85f + (n - 0.5f) * 0.3f);
			p.height = 0.25f;
		} else {
			float t = hashf(col_i, row, seed + 1);
			col = scale(mix(a, b, t), 0.82f + (n - 0.5f) * 0.35f + hashf(col_i, row, seed + 2) * 0.12f);
			if (chipped) {
				col = scale(col, 0.75f);
			}
			p.height = chipped ? 0.4f : 0.7f + (n - 0.5f) * 0.15f;
		}
		col = mix(col, Rgb{ 0.85f, 0.85f, 0.8f }, salt * 0.35f);
		float s = sstep(0.5f, 0.9f, fbm(u, v * 0.5f, 2, 4, seed + 77) * (1.2f - v * 0.5f)) * soot;
		p.color = scale(col, 1.0f - s * 0.55f);
		p.rough = 0.92f;
	});
	return c;
}

Canvas painted_metal(uint32_t seed, Rgb paint, float rust_amount, float gloss) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int x, int y, float u, float v, Pixel &p) {
		float n = fbm(u, v, 4, 4, seed);
		float brush = fbm(u * 0.3f, v * 20.0f, 8, 2, seed + 3);
		float rust_field = warped(u, v, 3, 6, seed + 13, 0.7f) * 0.8f + fbm(u, v, 32, 3, seed + 14) * 0.2f;
		float rthr = 0.73f - rust_amount * 0.3f;
		float rust = sstep(rthr, rthr + 0.04f, rust_field);
		float scratch = (hashf(x / 40, y / 2, seed + 3) > 0.995f) ? 1.0f : 0.0f;
		Rgb base = scale(paint, 0.88f + (n - 0.5f) * 0.18f + (brush - 0.5f) * 0.05f);
		base = mix(base, Rgb{ 0.55f, 0.55f, 0.52f }, scratch * 0.25f);
		Rgb rc = mix(Rgb{ 0.27f, 0.15f, 0.07f }, Rgb{ 0.14f, 0.08f, 0.05f }, fbm(u, v, 32, 2, seed + 4));
		float halo = sstep(rthr - 0.06f, rthr, rust_field) * (1.0f - rust);
		Rgb col = mix(base, rc, rust);
		col = mix(col, scale(base, 0.7f), halo * 0.5f);
		float streak = sstep(0.6f, 0.9f, fbm(u * 5.0f, v * 0.4f, 6, 3, seed + 31)) * rust_amount;
		col = mix(col, Rgb{ 0.35f, 0.18f, 0.08f }, streak * 0.4f);
		p.color = col;
		p.height = 0.5f + rust * 0.15f * fbm(u, v, 48, 2, seed + 8) - scratch * 0.1f;
		p.rough = rust > 0.5f ? 0.9f : gloss;
	});
	return c;
}

Canvas asphalt(uint32_t seed) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int x, int y, float u, float v, Pixel &p) {
		float n = fbm(u, v, 6, 4, seed);
		float k = 0.16f + (n - 0.5f) * 0.07f;
		float g = hashf(x, y, seed + 1);
		if (g > 0.9f) {
			k += 0.09f * hashf(x, y, seed + 2);
		} else if (g < 0.1f) {
			k -= 0.04f;
		}
		float crack = cracks(u, v, 4, seed + 40, 0.025f, 0.5f);
		float patch = sstep(0.68f, 0.7f, fbm(u, v, 2, 3, seed + 60));
		float wet = sstep(0.62f, 0.7f, warped(u, v, 2, 4, seed + 70, 0.7f));
		k = k * (1.0f - patch * 0.35f) - crack * 0.07f - wet * 0.04f;
		p.color = { k, k, k * 1.03f };
		p.height = 0.5f + (g - 0.5f) * 0.35f - crack * 0.4f;
		p.rough = std::clamp(0.92f - wet * 0.8f, 0.1f, 1.0f);
	});
	return c;
}

Canvas ground(uint32_t seed) {
	Canvas c;
	c.paint([&](int x, int y, float u, float v, Pixel &p) {
		float n = warped(u, v, 4, 5, seed, 0.6f);
		float blades = hashf(x, y, seed + 2);
		float clumps = fbm(u, v, 24, 3, seed + 4);
		Rgb dirt{ 0.22f, 0.18f, 0.13f };
		Rgb grass{ 0.25f, 0.27f, 0.12f };
		Rgb dry{ 0.38f, 0.34f, 0.18f };
		float t = sstep(0.38f, 0.62f, n);
		Rgb col = mix(dirt, mix(grass, dry, sstep(0.4f, 0.7f, clumps)), t);
		col = scale(col, 0.75f + blades * 0.45f * t + 0.15f);
		float leaf = hashf(x / 3, y / 3, seed + 9);
		if (leaf > 0.985f) {
			float lt = hashf(x / 3, y / 3, seed + 10);
			col = lt > 0.5f ? Rgb{ 0.62f, 0.32f, 0.08f } : Rgb{ 0.55f, 0.45f, 0.12f };
		}
		p.color = col;
		p.height = 0.5f + (blades - 0.5f) * 0.4f * t;
	});
	return c;
}

Canvas wood(uint32_t seed, Rgb tint, float paint_chip, int planks) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		int plank = int(u * float(planks));
		float pu = u * float(planks) - float(plank);
		float off = hashf(plank, 0, seed) * 10.0f;
		float n = fbm(u * 0.5f, v * 0.1f + off, 8, 3, seed + uint32_t(plank));
		float rings = 0.5f + 0.5f * std::sin((pu * 3.0f + n * 6.0f + v * 0.5f) * 6.28f * 2.0f);
		Cell knot = worley(u * 0.6f, v * 0.25f + off, 3, seed + 7);
		float kn = 1.0f - sstep(0.03f, 0.09f, knot.f1);
		Rgb col = scale(tint, 0.72f + rings * 0.22f + hashf(plank, 1, seed) * 0.12f - kn * 0.35f);
		bool gap = pu < 0.012f || pu > 0.988f;
		if (gap) {
			col = scale(col, 0.45f);
		}
		float chip = sstep(1.0f - paint_chip, 1.0f - paint_chip + 0.05f, fbm(u, v, 5, 4, seed + 31));
		col = mix(col, Rgb{ 0.5f, 0.44f, 0.36f }, chip);
		p.color = col;
		p.height = gap ? 0.2f : 0.5f + rings * 0.1f - kn * 0.2f;
		p.rough = chip > 0.5f ? 0.9f : 0.65f;
	});
	return c;
}

Canvas roof(uint32_t seed) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int x, int y, float u, float v, Pixel &p) {
		float n = fbm(u, v, 6, 4, seed);
		float gravel = hashf(x, y, seed + 5);
		float k = 0.1f + (n - 0.5f) * 0.05f + (gravel > 0.8f ? 0.06f * gravel : 0.0f);
		float seam_v = std::fmod(v * 4.0f, 1.0f);
		bool seam = seam_v < 0.012f;
		if (seam) {
			k = 0.06f;
		}
		Cell bubble = worley(u, v, 20, seed + 9);
		float bub = (bubble.id > 0.9f) ? 1.0f - sstep(0.1f, 0.35f, bubble.f1) : 0.0f;
		float puddle = sstep(0.66f, 0.72f, warped(u, v, 2, 4, seed + 13, 0.8f));
		p.color = { k + bub * 0.03f, k + bub * 0.03f, k * 1.05f + bub * 0.03f };
		p.height = seam ? 0.8f : 0.5f + (gravel - 0.5f) * 0.25f + bub * 0.3f;
		p.rough = std::clamp(0.9f - puddle * 0.8f, 0.08f, 1.0f);
	});
	return c;
}

Canvas profnastil(uint32_t seed, Rgb paint) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float ph = std::fmod(u * 16.0f, 1.0f);
		float wave = ph < 0.35f ? 1.0f : (ph < 0.5f ? 1.0f - (ph - 0.35f) / 0.15f : (ph < 0.85f ? 0.0f : (ph - 0.85f) / 0.15f));
		float r = warped(u, v, 3, 4, seed, 0.5f);
		Rgb base = scale(paint, 0.72f + wave * 0.3f);
		float rust = sstep(0.62f, 0.7f, r + (1.0f - wave) * 0.05f);
		float streak = sstep(0.6f, 0.85f, fbm(u * 8.0f, v * 0.4f, 6, 3, seed + 21));
		Rgb col = mix(base, Rgb{ 0.42f, 0.22f, 0.1f }, rust * 0.85f);
		col = mix(col, Rgb{ 0.35f, 0.2f, 0.1f }, streak * 0.3f);
		p.color = col;
		p.height = wave;
		p.rough = rust > 0.5f ? 0.9f : 0.5f;
	});
	return c;
}

Canvas fence_romashka(uint32_t seed) {
	Canvas c;
	c.has_rough = true;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float tu = u * 4.0f;
		float tv = v * 3.0f;
		float fx = tu - std::floor(tu) - 0.5f;
		float fy = tv - std::floor(tv) - 0.5f;
		float diamond = std::fabs(fx) + std::fabs(fy) * 1.2f;
		float ridge = 1.0f - sstep(0.3f, 0.36f, diamond);
		float inner = 1.0f - sstep(0.12f, 0.16f, diamond);
		float n = warped(u, v, 4, 5, seed, 0.4f);
		float k = 0.55f + (n - 0.5f) * 0.18f + ridge * 0.05f - inner * 0.06f;
		float streak = sstep(0.55f, 0.85f, fbm(u * 6.0f, v * 0.3f, 6, 3, seed + 41));
		float moss = sstep(0.65f, 0.85f, fbm(u, v, 3, 4, seed + 51)) * sstep(0.5f, 1.0f, v);
		Rgb col = scale(Rgb{ 1.0f, 0.98f, 0.94f }, k - streak * 0.12f);
		col = mix(col, Rgb{ 0.2f, 0.26f, 0.12f }, moss * 0.6f);
		float crack = cracks(u, v, 5, seed + 61, 0.025f, 0.3f);
		col = scale(col, 1.0f - crack * 0.3f);
		p.color = col;
		p.height = 0.4f + ridge * 0.35f - inner * 0.2f - crack * 0.3f;
		p.rough = 0.92f;
	});
	return c;
}

void facade(uint32_t seed, Canvas &albedo, Canvas &emission) {
	const float cells = 8.0f;
	auto window_at = [&](float u, float v, int &cx, int &cy, float &lx, float &ly) {
		float tu = u * cells;
		float tv = v * cells;
		cx = int(tu);
		cy = int(tv);
		lx = tu - float(cx);
		ly = tv - float(cy);
	};
	albedo.has_rough = true;
	albedo.paint([&](int, int, float u, float v, Pixel &p) {
		int cx, cy;
		float lx, ly;
		window_at(u, v, cx, cy, lx, ly);
		float n = fbm(u, v, 8, 3, seed);
		bool panel_seam = lx < 0.012f || ly < 0.012f;
		bool in_window = lx > 0.25f && lx < 0.75f && ly > 0.22f && ly < 0.72f;
		bool frame = in_window && (lx < 0.28f || lx > 0.72f || ly < 0.25f || ly > 0.69f || std::fabs(lx - 0.5f) < 0.012f || std::fabs(ly - 0.4f) < 0.012f);
		float lit = hashf(cx, cy, seed + 1);
		Rgb panel = scale(Rgb{ 0.62f, 0.6f, 0.56f }, 0.36f + (n - 0.5f) * 0.08f);
		if (hashf(cx / 2, cy, seed + 4) > 0.8f) {
			panel = scale(Rgb{ 0.55f, 0.5f, 0.44f }, 0.4f);
		}
		Rgb col = panel;
		float rough = 0.9f;
		if (panel_seam) {
			col = scale(panel, 0.6f);
		}
		if (ly > 0.72f && ly < 0.76f && lx > 0.22f && lx < 0.78f) {
			col = scale(panel, 1.3f);
		}
		if (in_window) {
			if (frame) {
				col = hashf(cx, cy, seed + 3) > 0.3f ? Rgb{ 0.75f, 0.74f, 0.7f } : Rgb{ 0.35f, 0.25f, 0.18f };
			} else if (lit > 0.76f) {
				float curtain = sstep(0.0f, 0.15f, std::fabs(lx - 0.5f) - 0.12f);
				Rgb warm{ 0.95f, 0.78f, 0.48f };
				Rgb curt = hashf(cx, cy, seed + 6) > 0.5f ? Rgb{ 0.8f, 0.45f, 0.3f } : Rgb{ 0.75f, 0.7f, 0.5f };
				col = mix(warm, curt, curtain * 0.7f);
				rough = 0.2f;
			} else if (lit > 0.73f) {
				col = { 0.45f, 0.55f, 0.85f };
				rough = 0.2f;
			} else {
				float refl = 0.05f + 0.06f * (1.0f - ly);
				col = { refl, refl * 1.1f, refl * 1.4f };
				rough = 0.08f;
			}
		}
		p.color = col;
		p.height = panel_seam ? 0.3f : (in_window && !frame ? 0.35f : 0.55f);
		p.rough = rough;
	});
	emission.has_height = false;
	emission.paint([&](int, int, float u, float v, Pixel &p) {
		int cx, cy;
		float lx, ly;
		window_at(u, v, cx, cy, lx, ly);
		bool in_window = lx > 0.28f && lx < 0.72f && ly > 0.25f && ly < 0.69f && std::fabs(lx - 0.5f) > 0.012f && std::fabs(ly - 0.4f) > 0.012f;
		p.color = { 0, 0, 0 };
		if (in_window) {
			float lit = hashf(cx, cy, seed + 1);
			float k = 0.55f + hashf(cx, cy, seed + 2) * 0.45f;
			if (lit > 0.76f) {
				float curtain = sstep(0.0f, 0.15f, std::fabs(lx - 0.5f) - 0.12f);
				p.color = scale(Rgb{ 1.0f, 0.7f, 0.36f }, k * (1.0f - curtain * 0.6f));
			} else if (lit > 0.73f) {
				p.color = scale(Rgb{ 0.4f, 0.55f, 1.0f }, k * 0.8f);
			}
		}
	});
}

Canvas soft_blob(int size, float hardness) {
	Canvas c(size);
	c.has_alpha = true;
	c.has_height = false;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float d = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f)) * 2.0f;
		float a = std::clamp(1.0f - d, 0.0f, 1.0f);
		a = std::pow(a, 1.0f + (1.0f - hardness) * 2.0f);
		p.color = { 1, 1, 1 };
		p.alpha = a;
	});
	return c;
}

Canvas smoke_puff(uint32_t seed) {
	Canvas c(128);
	c.has_alpha = true;
	c.has_height = false;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float d = std::sqrt((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f)) * 2.0f;
		float n = warped(u, v, 4, 4, seed, 0.6f);
		float a = std::clamp(1.0f - d * (0.8f + n * 0.6f), 0.0f, 1.0f);
		p.color = { 0.9f + n * 0.1f, 0.9f + n * 0.1f, 0.9f + n * 0.1f };
		p.alpha = a * a;
	});
	return c;
}

Canvas leaf(uint32_t seed) {
	Canvas c(64);
	c.has_alpha = true;
	c.has_height = false;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float x = (u - 0.5f) * 2.0f;
		float y = (v - 0.5f) * 2.0f;
		float w = 0.55f * (1.0f - y * y);
		bool inside = std::fabs(x) < w && std::fabs(y) < 0.95f;
		bool vein = std::fabs(x) < 0.04f;
		float n = fbm(u, v, 6, 2, seed);
		p.color = mix(Rgb{ 0.85f, 0.45f, 0.1f }, Rgb{ 0.6f, 0.2f, 0.05f }, n);
		if (vein) {
			p.color = scale(p.color, 0.7f);
		}
		p.alpha = inside ? 1.0f : 0.0f;
	});
	return c;
}

Canvas stain_decal(uint32_t seed, Rgb color, bool streak) {
	Canvas c(256);
	c.has_alpha = true;
	c.has_height = false;
	c.paint([&](int, int, float u, float v, Pixel &p) {
		float x = (u - 0.5f) * 2.0f;
		float y = (v - 0.5f) * 2.0f;
		float n = warped(u, v, 4, 5, seed, 0.6f);
		float d;
		if (streak) {
			float spread = 0.25f + v * 0.5f;
			d = std::fabs(x) / spread + std::max(0.0f, -y) * 0.2f + std::max(0.0f, y - 0.8f) * 4.0f;
		} else {
			d = std::sqrt(x * x + y * y);
		}
		float a = std::clamp((1.0f - d) * 1.6f + (n - 0.5f) * 1.2f, 0.0f, 1.0f);
		float ring = streak ? 0.0f : sstep(0.0f, 0.15f, a) * (1.0f - sstep(0.15f, 0.45f, a)) * 0.6f;
		p.color = scale(color, 0.8f + n * 0.4f);
		p.alpha = std::clamp(a * 0.55f + ring, 0.0f, 0.85f);
	});
	return c;
}

}

void MaterialLibrary::build() {
	if (built) {
		return;
	}
	built = true;

	std::vector<std::function<Canvas()>> makers;
	std::vector<std::string> names;
	std::vector<float> strengths;
	auto job = [&](const std::string &name, float normal_strength, std::function<Canvas()> fn) {
		names.push_back(name);
		strengths.push_back(normal_strength);
		makers.push_back(std::move(fn));
	};

	job("concrete", 3.0f, [] { return concrete_wall(11, 0.55f, Rgb{ 0.99f, 0.97f, 0.93f }); });
	job("concrete_dark", 3.0f, [] { return concrete_wall(13, 0.42f, Rgb{ 0.95f, 0.95f, 0.92f }); });
	job("concrete_floor", 3.0f, [] { return concrete_floor(23, 0.42f, 0.6f, 0.0f); });
	job("concrete_wet", 3.0f, [] { return concrete_floor(29, 0.33f, 1.0f, 0.6f); });
	job("plaster", 2.0f, [] { return painted_plaster(31, Rgb{ 0.8f, 0.79f, 0.74f }, 0.25f, 0.85f); });
	job("paint_green", 2.0f, [] { return painted_plaster(41, Rgb{ 0.25f, 0.42f, 0.32f }, 0.18f, 0.45f); });
	job("paint_blue", 2.0f, [] { return painted_plaster(43, Rgb{ 0.32f, 0.46f, 0.55f }, 0.3f, 0.5f); });
	job("paint_beige", 1.6f, [] { return painted_plaster(47, Rgb{ 0.74f, 0.62f, 0.44f }, 0.06f, 0.5f); });
	job("tile", 4.0f, [] { return wall_tile(53); });
	job("tile_floor", 3.0f, [] { return metlakh(57); });
	job("linoleum", 2.0f, [] { return linoleum(59); });
	job("brick", 4.0f, [] { return brick(61, Rgb{ 0.5f, 0.22f, 0.14f }, Rgb{ 0.38f, 0.16f, 0.11f }, Rgb{ 0.58f, 0.56f, 0.52f }, 0.6f); });
	job("brick_white", 4.0f, [] { return brick(63, Rgb{ 0.78f, 0.76f, 0.7f }, Rgb{ 0.7f, 0.68f, 0.62f }, Rgb{ 0.55f, 0.53f, 0.5f }, 0.4f); });
	job("metal", 1.5f, [] { return painted_metal(71, Rgb{ 0.28f, 0.35f, 0.29f }, 0.3f, 0.45f); });
	job("metal_gray", 1.5f, [] { return painted_metal(73, Rgb{ 0.42f, 0.44f, 0.44f }, 0.22f, 0.4f); });
	job("metal_blue", 1.5f, [] { return painted_metal(75, Rgb{ 0.2f, 0.3f, 0.45f }, 0.25f, 0.4f); });
	job("rust", 2.0f, [] { return painted_metal(79, Rgb{ 0.36f, 0.19f, 0.09f }, 0.75f, 0.85f); });
	job("asphalt", 2.0f, [] { return asphalt(83); });
	job("ground", 2.0f, [] { return ground(89); });
	job("wood", 2.0f, [] { return wood(97, Rgb{ 0.5f, 0.36f, 0.22f }, 0.0f, 5); });
	job("wood_painted", 1.5f, [] { return wood(101, Rgb{ 0.45f, 0.22f, 0.14f }, 0.25f, 1); });
	job("roof", 2.0f, [] { return roof(103); });
	job("fence", 3.0f, [] { return profnastil(107, Rgb{ 0.18f, 0.35f, 0.24f }); });
	job("fence_gray", 3.0f, [] { return profnastil(109, Rgb{ 0.45f, 0.46f, 0.47f }); });
	job("fence_concrete", 5.0f, [] { return fence_romashka(111); });
	job("fx_soft", 0.0f, [] { return soft_blob(64, 0.3f); });
	job("fx_hard", 0.0f, [] { return soft_blob(32, 0.9f); });
	job("fx_smoke", 0.0f, [] { return smoke_puff(5); });
	job("fx_leaf", 0.0f, [] { return leaf(7); });
	job("decal_water", 0.0f, [] { return stain_decal(9, Rgb{ 0.28f, 0.24f, 0.18f }, false); });
	job("decal_streak", 0.0f, [] { return stain_decal(19, Rgb{ 0.32f, 0.18f, 0.08f }, true); });
	job("decal_soot", 0.0f, [] { return stain_decal(29, Rgb{ 0.05f, 0.05f, 0.05f }, true); });

	std::vector<Canvas> results(makers.size());
	unsigned threads = std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
	std::vector<std::thread> pool;
	for (unsigned t = 0; t < threads; t++) {
		pool.emplace_back([&, t]() {
			for (size_t i = t; i < makers.size(); i += threads) {
				results[i] = makers[i]();
			}
		});
	}
	Canvas fac;
	Canvas fac_emission;
	facade(113, fac, fac_emission);
	for (std::thread &th : pool) {
		th.join();
	}

	for (size_t i = 0; i < results.size(); i++) {
		textures[names[i]] = make_texture(results[i]);
		if (results[i].has_height && strengths[i] > 0.0f) {
			normals[names[i]] = make_normal(results[i], strengths[i]);
		}
		if (results[i].has_rough) {
			roughness[names[i]] = make_rough(results[i]);
		}
	}
	textures["facade"] = make_texture(fac);
	textures["facade_emission"] = make_texture(fac_emission);
	normals["facade"] = make_normal(fac, 2.0f);
	roughness["facade"] = make_rough(fac);

	add_surface("concrete", "concrete", 3.0f, 0.95f, 0.8f, true);
	add_surface("concrete_dark", "concrete_dark", 3.0f, 0.95f, 0.8f, true);
	add_surface("concrete_floor", "concrete_floor", 3.0f, 1.0f, 0.8f, true);
	add_surface("concrete_wet", "concrete_wet", 3.0f, 1.0f, 0.8f, true);
	add_surface("plaster", "plaster", 2.5f, 1.0f, 0.5f, true);
	add_surface("paint_green", "paint_green", 2.5f, 1.0f, 0.5f, true);
	add_surface("paint_blue", "paint_blue", 2.5f, 1.0f, 0.5f, true);
	add_surface("paint_beige", "paint_beige", 2.5f, 1.0f, 0.4f, true);
	add_surface("tile", "tile", 1.2f, 1.0f, 0.7f, true);
	add_surface("tile_floor", "tile_floor", 0.8f, 1.0f, 0.6f, true);
	add_surface("linoleum", "linoleum", 4.0f, 1.0f, 0.4f, true);
	add_surface("brick", "brick", 1.04f, 0.95f, 0.9f, true);
	add_surface("brick_white", "brick_white", 1.04f, 0.95f, 0.9f, true);
	add_surface("metal", "metal", 2.0f, 1.0f, 0.4f, true);
	add_surface("metal_gray", "metal_gray", 2.0f, 1.0f, 0.4f, true);
	add_surface("metal_blue", "metal_blue", 2.0f, 1.0f, 0.4f, true);
	add_surface("rust", "rust", 1.5f, 1.0f, 0.6f, true);
	add_surface("asphalt", "asphalt", 5.0f, 1.0f, 0.6f, true);
	add_surface("ground", "ground", 5.0f, 1.0f, 0.6f, true);
	add_surface("wood", "wood", 1.6f, 1.0f, 0.5f, true);
	add_surface("roof", "roof", 5.0f, 1.0f, 0.5f, true);
	add_surface("fence", "fence", 2.0f, 1.0f, 0.7f, true);
	add_surface("fence_gray", "fence_gray", 2.0f, 1.0f, 0.7f, true);
	add_surface("fence_concrete", "fence_concrete", 4.0f, 0.95f, 1.0f, true);
	add_surface("door_wood", "wood_painted", 1.2f, 1.0f, 0.4f, false);
	add_surface("door_metal", "metal", 1.5f, 1.0f, 0.4f, false);
	add_surface("door_gray", "metal_gray", 1.5f, 1.0f, 0.4f, false);
	add_surface("wood_local", "wood", 1.2f, 1.0f, 0.4f, false);
	add_surface("rust_local", "rust", 1.0f, 1.0f, 0.5f, false);
	add_surface("metal_local", "metal", 1.0f, 1.0f, 0.4f, false);
	add_surface("metal_gray_local", "metal_gray", 1.0f, 1.0f, 0.4f, false);

	{
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, textures["facade"]);
		m->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
		m->set_texture(BaseMaterial3D::TEXTURE_NORMAL, normals["facade"]);
		m->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS, roughness["facade"]);
		m->set_roughness(1.0f);
		m->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
		m->set_texture(BaseMaterial3D::TEXTURE_EMISSION, textures["facade_emission"]);
		m->set_emission(Color(0, 0, 0));
		m->set_emission_energy_multiplier(1.1f);
		m->set_flag(BaseMaterial3D::FLAG_UV1_USE_TRIPLANAR, true);
		m->set_flag(BaseMaterial3D::FLAG_UV1_USE_WORLD_TRIPLANAR, true);
		m->set_uv1_triplanar_blend_sharpness(10.0f);
		m->set_uv1_scale(Vector3(1.0f / 24.0f, 1.0f / 24.0f, 1.0f / 24.0f));
		materials["facade"] = m;
	}

	add_flat("black", 0.04f, 0.04f, 0.045f, 0.7f);
	add_flat("uniform", 0.07f, 0.075f, 0.08f, 0.85f);
	add_flat("uniform_detail", 0.12f, 0.13f, 0.14f, 0.7f);
	add_flat("pants", 0.13f, 0.13f, 0.15f, 0.9f);
	add_flat("jeans", 0.16f, 0.2f, 0.3f, 0.9f);
	add_flat("skin", 0.82f, 0.62f, 0.5f, 0.6f);
	add_flat("armor", 0.27f, 0.30f, 0.22f, 0.8f);
	add_flat("cardigan", 0.42f, 0.24f, 0.36f, 0.9f);
	add_flat("skirt", 0.2f, 0.18f, 0.22f, 0.9f);
	add_flat("hair_gray", 0.62f, 0.62f, 0.6f, 0.9f);
	add_flat("hair_dark", 0.12f, 0.09f, 0.07f, 0.9f);
	add_flat("jacket_resident", 0.18f, 0.24f, 0.34f, 0.85f);
	add_flat("plastic_white", 0.86f, 0.86f, 0.82f, 0.4f);
	add_flat("plastic_dark", 0.06f, 0.06f, 0.07f, 0.35f);
	add_flat("plastic_red", 0.6f, 0.08f, 0.06f, 0.4f);
	add_flat("plastic_green", 0.12f, 0.35f, 0.18f, 0.5f);
	add_flat("plastic_blue", 0.12f, 0.24f, 0.5f, 0.5f);
	add_flat("rubber", 0.11f, 0.13f, 0.11f, 0.8f);
	add_flat("tire", 0.05f, 0.05f, 0.05f, 0.9f);
	add_flat("canvas", 0.38f, 0.37f, 0.27f, 0.95f);
	add_flat("cloth_stained", 0.45f, 0.4f, 0.32f, 1.0f);
	add_flat("red_paint", 0.6f, 0.08f, 0.06f, 0.6f);
	add_flat("yellow_paint", 0.85f, 0.68f, 0.1f, 0.6f);
	add_flat("white_paint", 0.85f, 0.85f, 0.82f, 0.6f);
	add_flat("orange_paint", 0.85f, 0.38f, 0.08f, 0.6f);
	add_flat("paper", 0.86f, 0.82f, 0.7f, 0.9f);
	add_flat("poster_paper", 0.82f, 0.76f, 0.6f, 0.95f);
	add_flat("steel", 0.55f, 0.56f, 0.58f, 0.35f, 0.8f);
	add_flat("chrome", 0.8f, 0.8f, 0.82f, 0.15f, 1.0f);
	add_flat("tree_bark", 0.12f, 0.1f, 0.08f, 1.0f);
	add_flat("birch_bark", 0.82f, 0.8f, 0.74f, 0.9f);
	add_flat("tree_crown", 0.06f, 0.09f, 0.05f, 1.0f);
	add_flat("tree_crown_autumn", 0.32f, 0.22f, 0.06f, 1.0f);
	add_flat("glass_booth", 0.3f, 0.4f, 0.45f, 0.1f, 0.2f);
	add_flat("glass_bottle", 0.1f, 0.3f, 0.12f, 0.1f, 0.1f);
	add_flat("car_red", 0.5f, 0.06f, 0.05f, 0.25f, 0.3f);
	add_flat("car_white", 0.8f, 0.8f, 0.78f, 0.25f, 0.3f);
	add_flat("car_black", 0.05f, 0.05f, 0.06f, 0.2f, 0.4f);
	add_flat("car_silver", 0.55f, 0.56f, 0.58f, 0.25f, 0.6f);
	add_flat("car_green", 0.12f, 0.25f, 0.15f, 0.3f, 0.3f);
	add_flat("mattress", 0.45f, 0.38f, 0.3f, 1.0f);
	add_flat("cardboard", 0.55f, 0.42f, 0.28f, 1.0f);

	add_emissive("lamp_warm", 1.0f, 0.78f, 0.45f, 4.0f, false);
	add_emissive("lamp_cold", 0.8f, 0.9f, 1.0f, 3.0f, false);
	add_emissive("led_off", 0.05f, 0.05f, 0.05f, 0.0f, true);
	add_emissive("tv_glow", 0.5f, 0.65f, 1.0f, 2.0f, false);
	add_emissive("red_light", 1.0f, 0.1f, 0.05f, 6.0f, true);
	add_emissive("blue_light", 0.1f, 0.3f, 1.0f, 6.0f, true);
	add_emissive("sun_disc", 1.0f, 0.6f, 0.3f, 8.0f, true);
	add_emissive("phone_screen", 0.75f, 0.85f, 1.0f, 1.2f, true);
	add_emissive("car_light", 1.0f, 0.95f, 0.8f, 2.0f, false);
	add_emissive("car_tail", 0.8f, 0.05f, 0.03f, 1.0f, false);
	add_emissive("fire_glow", 1.0f, 0.45f, 0.1f, 5.0f, true);
	add_emissive("candle", 1.0f, 0.75f, 0.35f, 6.0f, true);

	{
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_albedo(Color(1.0f, 0.05f, 0.05f, 0.18f));
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
		m->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
		m->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
		m->set_emission(Color(1.0f, 0.1f, 0.1f));
		materials["beam"] = m;
	}
	{
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_albedo(Color(0.14f, 0.15f, 0.14f, 0.42f));
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_roughness(0.03f);
		m->set_metallic(0.05f);
		m->set_specular(0.9f);
		m->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
		m->set_texture(BaseMaterial3D::TEXTURE_NORMAL, normals["concrete_wet"]);
		m->set_normal_scale(0.15f);
		m->set_flag(BaseMaterial3D::FLAG_UV1_USE_TRIPLANAR, true);
		m->set_flag(BaseMaterial3D::FLAG_UV1_USE_WORLD_TRIPLANAR, true);
		m->set_uv1_scale(Vector3(0.5f, 0.5f, 0.5f));
		materials["water"] = m;
	}
	{
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_albedo(Color(0.55f, 0.65f, 0.7f, 0.22f));
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_roughness(0.05f);
		m->set_metallic(0.4f);
		m->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
		materials["glass"] = m;
	}
	{
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_albedo(Color(0.2f, 0.25f, 0.28f, 0.55f));
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_roughness(0.05f);
		m->set_metallic(0.6f);
		m->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
		materials["glass_dirty"] = m;
	}
}

void MaterialLibrary::add_surface(const std::string &name, const std::string &tex, float meters, float roughness_value, float normal_strength, bool world_space) {
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, textures[tex]);
	auto it = normals.find(tex);
	if (it != normals.end() && normal_strength > 0.0f) {
		m->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
		m->set_texture(BaseMaterial3D::TEXTURE_NORMAL, it->second);
		m->set_normal_scale(normal_strength);
	}
	auto rt = roughness.find(tex);
	if (rt != roughness.end()) {
		m->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS, rt->second);
	}
	m->set_roughness(roughness_value);
	m->set_flag(BaseMaterial3D::FLAG_UV1_USE_TRIPLANAR, true);
	m->set_flag(BaseMaterial3D::FLAG_UV1_USE_WORLD_TRIPLANAR, world_space);
	m->set_uv1_triplanar_blend_sharpness(10.0f);
	float s = 1.0f / meters;
	m->set_uv1_scale(Vector3(s, s, s));
	materials[name] = m;
}

void MaterialLibrary::add_flat(const std::string &name, float r, float g, float b, float roughness_value, float metallic) {
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_albedo(Color(r, g, b));
	m->set_roughness(roughness_value);
	m->set_metallic(metallic);
	materials[name] = m;
}

void MaterialLibrary::add_emissive(const std::string &name, float r, float g, float b, float energy, bool unshaded) {
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_albedo(Color(r, g, b));
	m->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
	m->set_emission(Color(r, g, b));
	m->set_emission_energy_multiplier(energy);
	if (unshaded) {
		m->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	}
	materials[name] = m;
}

Ref<StandardMaterial3D> MaterialLibrary::get(const std::string &name) const {
	auto it = materials.find(name);
	if (it != materials.end()) {
		return it->second;
	}
	auto fallback = materials.find("concrete");
	if (fallback != materials.end()) {
		return fallback->second;
	}
	return Ref<StandardMaterial3D>();
}

Ref<ImageTexture> MaterialLibrary::texture(const std::string &name) const {
	auto it = textures.find(name);
	return it != textures.end() ? it->second : Ref<ImageTexture>();
}

}
