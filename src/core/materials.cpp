#include "core/materials.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

using namespace godot;

namespace urbex {

namespace {

constexpr int SIZE = 256;

uint32_t hash2(int x, int y, uint32_t seed) {
	uint32_t h = seed * 374761393u + uint32_t(x) * 668265263u + uint32_t(y) * 2246822519u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

float hashf(int x, int y, uint32_t seed) {
	return float(hash2(x, y, seed) & 0xFFFFFF) / float(0xFFFFFF);
}

float smooth(float t) {
	return t * t * (3.0f - 2.0f * t);
}

float value_noise(float x, float y, int period, uint32_t seed) {
	int x0 = int(std::floor(x));
	int y0 = int(std::floor(y));
	float fx = smooth(x - float(x0));
	float fy = smooth(y - float(y0));
	auto w = [period](int v) { return ((v % period) + period) % period; };
	float a = hashf(w(x0), w(y0), seed);
	float b = hashf(w(x0 + 1), w(y0), seed);
	float c = hashf(w(x0), w(y0 + 1), seed);
	float d = hashf(w(x0 + 1), w(y0 + 1), seed);
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

struct Canvas {
	std::vector<Rgb> color = std::vector<Rgb>(SIZE * SIZE, { 0, 0, 0 });
	std::vector<float> height = std::vector<float>(SIZE * SIZE, 0.5f);
	bool has_height = false;

	void paint(const std::function<void(int, int, float, float, Rgb &, float &)> &fn) {
		for (int y = 0; y < SIZE; y++) {
			for (int x = 0; x < SIZE; x++) {
				float u = (float(x) + 0.5f) / float(SIZE);
				float v = (float(y) + 0.5f) / float(SIZE);
				fn(x, y, u, v, color[y * SIZE + x], height[y * SIZE + x]);
			}
		}
	}

	Ref<ImageTexture> to_texture() const {
		PackedByteArray data;
		data.resize(SIZE * SIZE * 3);
		uint8_t *w = data.ptrw();
		for (int i = 0; i < SIZE * SIZE; i++) {
			w[i * 3 + 0] = uint8_t(std::clamp(color[i].r, 0.0f, 1.0f) * 255.0f);
			w[i * 3 + 1] = uint8_t(std::clamp(color[i].g, 0.0f, 1.0f) * 255.0f);
			w[i * 3 + 2] = uint8_t(std::clamp(color[i].b, 0.0f, 1.0f) * 255.0f);
		}
		Ref<Image> img = Image::create_from_data(SIZE, SIZE, false, Image::FORMAT_RGB8, data);
		img->generate_mipmaps();
		return ImageTexture::create_from_image(img);
	}

	Ref<ImageTexture> to_normal(float strength) const {
		PackedByteArray data;
		data.resize(SIZE * SIZE * 3);
		uint8_t *w = data.ptrw();
		for (int i = 0; i < SIZE * SIZE; i++) {
			uint8_t h = uint8_t(std::clamp(height[i], 0.0f, 1.0f) * 255.0f);
			w[i * 3 + 0] = h;
			w[i * 3 + 1] = h;
			w[i * 3 + 2] = h;
		}
		Ref<Image> img = Image::create_from_data(SIZE, SIZE, false, Image::FORMAT_RGB8, data);
		img->bump_map_to_normal_map(strength);
		img->generate_mipmaps(true);
		return ImageTexture::create_from_image(img);
	}
};

Canvas make_concrete(uint32_t seed, float base, float stain_amount) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float n = fbm(u, v, 4, 5, seed);
		float fine = fbm(u, v, 32, 2, seed + 7);
		float stain = fbm(u, v, 2, 4, seed + 101);
		float k = base + (n - 0.5f) * 0.22f + (fine - 0.5f) * 0.08f;
		if (hashf(x, y, seed + 3) > 0.985f) {
			k -= 0.12f;
		}
		k -= sstep(0.55f, 0.8f, stain) * stain_amount;
		float crack_n = fbm(u, v, 3, 4, seed + 55);
		float crack = 1.0f - sstep(0.0f, 0.012f, std::fabs(crack_n - 0.5f));
		k -= crack * 0.18f;
		out = { k * 0.99f, k * 0.97f, k * 0.94f };
		h = 0.5f + (fine - 0.5f) * 0.6f - crack * 0.4f;
	});
	return c;
}

Canvas make_plaster(uint32_t seed, Rgb paint, Rgb under, float peel) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float n = fbm(u, v, 4, 4, seed);
		float fine = fbm(u, v, 24, 2, seed + 9);
		float p = fbm(u, v, 3, 5, seed + 21);
		float drip = fbm(u * 6.0f, v * 0.5f, 4, 3, seed + 33);
		float peeled = sstep(1.0f - peel, 1.0f - peel + 0.04f, p);
		Rgb base = scale(paint, 0.9f + (n - 0.5f) * 0.25f + (fine - 0.5f) * 0.06f);
		Rgb conc = scale(under, 0.85f + (fine - 0.5f) * 0.3f);
		out = mix(base, conc, peeled);
		float dirt = sstep(0.6f, 0.9f, drip) * 0.25f;
		out = scale(out, 1.0f - dirt);
		h = 0.6f - peeled * 0.25f + (fine - 0.5f) * 0.2f;
		(void)x;
		(void)y;
	});
	return c;
}

Canvas make_tile(uint32_t seed) {
	Canvas c;
	c.has_height = true;
	const int tile = 32;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		int tx = x / tile;
		int ty = y / tile;
		int lx = x % tile;
		int ly = y % tile;
		bool grout = lx < 2 || ly < 2;
		float tint = hashf(tx, ty, seed) * 0.08f;
		bool missing = hashf(tx, ty, seed + 5) > 0.93f;
		float dirt = fbm(u, v, 4, 4, seed + 11);
		if (missing) {
			float k = 0.35f + (fbm(u, v, 16, 3, seed + 2) - 0.5f) * 0.15f;
			out = { k, k * 0.98f, k * 0.95f };
			h = 0.2f;
		} else if (grout) {
			out = { 0.42f, 0.41f, 0.38f };
			h = 0.35f;
		} else {
			out = { 0.80f - tint, 0.84f - tint, 0.82f - tint * 0.5f };
			h = 0.7f;
		}
		out = scale(out, 1.0f - sstep(0.5f, 0.85f, dirt) * 0.35f);
	});
	return c;
}

Canvas make_brick(uint32_t seed) {
	Canvas c;
	c.has_height = true;
	const int bw = 64;
	const int bh = 21;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		int row = y / bh;
		int off = (row % 2) * (bw / 2);
		int col = ((x + off) % SIZE) / bw;
		int lx = (x + off) % bw;
		int ly = y % bh;
		bool mortar = lx < 3 || ly < 3 || y >= (SIZE / bh) * bh;
		float n = fbm(u, v, 16, 3, seed);
		if (mortar) {
			float k = 0.55f + (n - 0.5f) * 0.1f;
			out = { k, k * 0.97f, k * 0.92f };
			h = 0.3f;
		} else {
			float t = hashf(col, row, seed);
			Rgb a{ 0.50f, 0.22f, 0.15f };
			Rgb b{ 0.38f, 0.17f, 0.12f };
			out = scale(mix(a, b, t), 0.85f + (n - 0.5f) * 0.4f);
			h = 0.7f + (n - 0.5f) * 0.2f;
		}
		float soot = fbm(u, v, 2, 4, seed + 77);
		out = scale(out, 1.0f - sstep(0.55f, 0.85f, soot) * 0.45f);
	});
	return c;
}

Canvas make_metal(uint32_t seed, Rgb paint, float rust_amount) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float n = fbm(u, v, 4, 4, seed);
		float r = fbm(u, v, 3, 5, seed + 13);
		float scratch = hashf(x / 2, y / 40, seed + 3) > 0.97f ? 0.15f : 0.0f;
		Rgb base = scale(paint, 0.9f + (n - 0.5f) * 0.2f + scratch);
		float rust = sstep(1.0f - rust_amount, 1.0f - rust_amount + 0.1f, r);
		Rgb rc = scale(Rgb{ 0.42f, 0.22f, 0.10f }, 0.8f + (fbm(u, v, 16, 2, seed + 4) - 0.5f) * 0.5f);
		out = mix(base, rc, rust);
		h = 0.55f + rust * 0.15f * fbm(u, v, 32, 2, seed + 8);
	});
	return c;
}

Canvas make_asphalt(uint32_t seed) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float n = fbm(u, v, 8, 4, seed);
		float k = 0.17f + (n - 0.5f) * 0.08f;
		float g = hashf(x, y, seed + 1);
		if (g > 0.9f) {
			k += 0.08f;
		} else if (g < 0.08f) {
			k -= 0.05f;
		}
		float crack_n = fbm(u, v, 2, 5, seed + 40);
		float crack = 1.0f - sstep(0.0f, 0.01f, std::fabs(crack_n - 0.5f));
		k -= crack * 0.08f;
		out = { k, k, k * 1.02f };
		h = 0.5f + (g - 0.5f) * 0.3f - crack * 0.3f;
	});
	return c;
}

Canvas make_ground(uint32_t seed) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float n = fbm(u, v, 4, 5, seed);
		float blades = hashf(x, y, seed + 2);
		Rgb dirt{ 0.24f, 0.20f, 0.15f };
		Rgb grass{ 0.24f, 0.27f, 0.13f };
		float t = sstep(0.4f, 0.65f, n);
		out = scale(mix(dirt, grass, t), 0.8f + blades * 0.35f);
		h = 0.5f + (blades - 0.5f) * 0.4f * t;
	});
	return c;
}

Canvas make_wood(uint32_t seed, Rgb tint) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float n = fbm(u, v * 0.25f, 8, 3, seed);
		float grain = 0.5f + 0.5f * std::sin((u * 40.0f + n * 8.0f) * 3.14159f);
		float plank = (x % 64) < 2 ? 0.6f : 1.0f;
		out = scale(tint, (0.75f + grain * 0.3f) * plank);
		float wear = fbm(u, v, 4, 4, seed + 9);
		out = mix(out, Rgb{ 0.35f, 0.33f, 0.3f }, sstep(0.6f, 0.8f, wear) * 0.6f);
		h = 0.5f + grain * 0.2f - (1.0f - plank) * 0.5f;
		(void)y;
	});
	return c;
}

Canvas make_roof(uint32_t seed) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float n = fbm(u, v, 8, 4, seed);
		float gravel = hashf(x, y, seed + 5);
		float k = 0.11f + (n - 0.5f) * 0.05f + (gravel > 0.8f ? 0.05f : 0.0f);
		bool seam = (y % 128) < 3;
		if (seam) {
			k = 0.07f;
		}
		out = { k, k, k * 1.05f };
		h = seam ? 0.75f : 0.5f + (gravel - 0.5f) * 0.2f;
		(void)u;
	});
	return c;
}

Canvas make_profnastil(uint32_t seed, Rgb paint) {
	Canvas c;
	c.has_height = true;
	c.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		float wave = 0.5f + 0.5f * std::sin(u * 3.14159f * 2.0f * 12.0f);
		float r = fbm(u, v, 4, 4, seed);
		Rgb base = scale(paint, 0.7f + wave * 0.35f);
		float rust = sstep(0.62f, 0.72f, r);
		out = mix(base, Rgb{ 0.40f, 0.22f, 0.11f }, rust * 0.8f);
		h = wave;
		(void)x;
		(void)y;
	});
	return c;
}

void make_facade(uint32_t seed, Canvas &albedo, Canvas &emission) {
	const int cell = 32;
	albedo.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		int cx = x / cell;
		int cy = y / cell;
		int lx = x % cell;
		int ly = y % cell;
		bool window = lx >= 8 && lx < 24 && ly >= 6 && ly < 26;
		float n = fbm(u, v, 8, 3, seed);
		if (window) {
			float lit = hashf(cx, cy, seed + 1);
			if (lit > 0.78f) {
				out = { 0.9f, 0.75f, 0.45f };
			} else if (lit > 0.75f) {
				out = { 0.55f, 0.65f, 0.85f };
			} else {
				out = { 0.04f, 0.05f, 0.07f };
			}
		} else {
			float k = 0.22f + (n - 0.5f) * 0.06f;
			bool seam = lx == 0 || ly == 0;
			out = { k * (seam ? 0.7f : 1.0f), k * (seam ? 0.7f : 1.0f), k * 1.05f };
		}
		h = 0.5f;
	});
	emission.paint([&](int x, int y, float u, float v, Rgb &out, float &h) {
		int cx = x / cell;
		int cy = y / cell;
		int lx = x % cell;
		int ly = y % cell;
		bool window = lx >= 8 && lx < 24 && ly >= 6 && ly < 26;
		out = { 0, 0, 0 };
		if (window) {
			float lit = hashf(cx, cy, seed + 1);
			float k = 0.6f + hashf(cx, cy, seed + 2) * 0.4f;
			if (lit > 0.78f) {
				out = scale(Rgb{ 1.0f, 0.72f, 0.38f }, k);
			} else if (lit > 0.75f) {
				out = scale(Rgb{ 0.45f, 0.6f, 1.0f }, k);
			}
		}
		h = 0.5f;
		(void)u;
		(void)v;
	});
}

} // namespace

void MaterialLibrary::build() {
	if (built) {
		return;
	}
	built = true;

	auto store = [this](const std::string &name, const Canvas &c, float normal_strength) {
		textures[name] = c.to_texture();
		if (c.has_height && normal_strength > 0.0f) {
			normals[name] = c.to_normal(normal_strength);
		}
	};

	store("concrete", make_concrete(11, 0.52f, 0.18f), 3.0f);
	store("concrete_floor", make_concrete(23, 0.40f, 0.25f), 3.0f);
	store("plaster", make_plaster(31, Rgb{ 0.78f, 0.77f, 0.72f }, Rgb{ 0.5f, 0.49f, 0.46f }, 0.3f), 2.0f);
	store("paint_green", make_plaster(41, Rgb{ 0.27f, 0.42f, 0.33f }, Rgb{ 0.55f, 0.54f, 0.5f }, 0.22f), 2.0f);
	store("paint_blue", make_plaster(43, Rgb{ 0.33f, 0.47f, 0.55f }, Rgb{ 0.55f, 0.54f, 0.5f }, 0.3f), 2.0f);
	store("paint_beige", make_plaster(47, Rgb{ 0.72f, 0.62f, 0.45f }, Rgb{ 0.5f, 0.48f, 0.44f }, 0.08f), 1.5f);
	store("tile", make_tile(53), 4.0f);
	store("brick", make_brick(61), 4.0f);
	store("metal", make_metal(71, Rgb{ 0.30f, 0.36f, 0.31f }, 0.3f), 1.5f);
	store("metal_gray", make_metal(73, Rgb{ 0.42f, 0.43f, 0.44f }, 0.25f), 1.5f);
	store("rust", make_metal(79, Rgb{ 0.38f, 0.2f, 0.1f }, 0.6f), 2.0f);
	store("asphalt", make_asphalt(83), 2.0f);
	store("ground", make_ground(89), 2.0f);
	store("wood", make_wood(97, Rgb{ 0.42f, 0.28f, 0.16f }), 2.0f);
	store("wood_painted", make_wood(101, Rgb{ 0.45f, 0.22f, 0.14f }), 1.5f);
	store("roof", make_roof(103), 2.0f);
	store("fence", make_profnastil(107, Rgb{ 0.18f, 0.35f, 0.24f }), 3.0f);
	store("fence_gray", make_profnastil(109, Rgb{ 0.45f, 0.46f, 0.47f }), 3.0f);

	Canvas facade;
	Canvas facade_emission;
	make_facade(113, facade, facade_emission);
	textures["facade"] = facade.to_texture();
	textures["facade_emission"] = facade_emission.to_texture();

	add_surface("concrete", "concrete", 3.0f, 0.95f, 0.8f, true);
	add_surface("concrete_floor", "concrete_floor", 3.0f, 0.9f, 0.8f, true);
	add_surface("plaster", "plaster", 2.5f, 0.92f, 0.5f, true);
	add_surface("paint_green", "paint_green", 2.5f, 0.7f, 0.5f, true);
	add_surface("paint_blue", "paint_blue", 2.5f, 0.75f, 0.5f, true);
	add_surface("paint_beige", "paint_beige", 2.5f, 0.8f, 0.4f, true);
	add_surface("tile", "tile", 1.2f, 0.35f, 0.6f, true);
	add_surface("brick", "brick", 2.0f, 0.95f, 0.8f, true);
	add_surface("metal", "metal", 2.0f, 0.55f, 0.4f, true);
	add_surface("metal_gray", "metal_gray", 2.0f, 0.5f, 0.4f, true);
	add_surface("rust", "rust", 1.5f, 0.8f, 0.6f, true);
	add_surface("asphalt", "asphalt", 4.0f, 0.9f, 0.6f, true);
	add_surface("ground", "ground", 4.0f, 1.0f, 0.6f, true);
	add_surface("wood", "wood", 2.0f, 0.85f, 0.5f, true);
	add_surface("roof", "roof", 4.0f, 0.95f, 0.5f, true);
	add_surface("fence", "fence", 2.0f, 0.6f, 0.7f, true);
	add_surface("fence_gray", "fence_gray", 2.0f, 0.6f, 0.7f, true);

	add_surface("door_wood", "wood_painted", 1.2f, 0.75f, 0.4f, false);
	add_surface("door_metal", "metal", 1.5f, 0.5f, 0.4f, false);
	add_surface("door_gray", "metal_gray", 1.5f, 0.5f, 0.4f, false);

	{
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, textures["facade"]);
		m->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
		m->set_texture(BaseMaterial3D::TEXTURE_EMISSION, textures["facade_emission"]);
		m->set_emission(Color(0, 0, 0));
		m->set_emission_energy_multiplier(0.9f);
		m->set_flag(BaseMaterial3D::FLAG_UV1_USE_TRIPLANAR, true);
		m->set_flag(BaseMaterial3D::FLAG_UV1_USE_WORLD_TRIPLANAR, true);
		m->set_uv1_scale(Vector3(1.0f / 24.0f, 1.0f / 24.0f, 1.0f / 24.0f));
		m->set_roughness(0.8f);
		materials["facade"] = m;
	}

	add_flat("black", 0.04f, 0.04f, 0.045f, 0.7f);
	add_flat("uniform", 0.07f, 0.075f, 0.08f, 0.85f);
	add_flat("pants", 0.13f, 0.13f, 0.15f, 0.9f);
	add_flat("skin", 0.82f, 0.62f, 0.5f, 0.7f);
	add_flat("armor", 0.27f, 0.30f, 0.22f, 0.8f);
	add_flat("cardigan", 0.42f, 0.24f, 0.36f, 0.9f);
	add_flat("hair_gray", 0.62f, 0.62f, 0.6f, 0.9f);
	add_flat("jacket_resident", 0.18f, 0.24f, 0.34f, 0.85f);
	add_flat("plastic_white", 0.86f, 0.86f, 0.82f, 0.4f);
	add_flat("plastic_dark", 0.06f, 0.06f, 0.07f, 0.35f);
	add_flat("rubber", 0.11f, 0.13f, 0.11f, 0.8f);
	add_flat("canvas", 0.38f, 0.37f, 0.27f, 0.95f);
	add_flat("red_paint", 0.6f, 0.08f, 0.06f, 0.6f);
	add_flat("yellow_paint", 0.85f, 0.68f, 0.1f, 0.6f);
	add_flat("paper", 0.86f, 0.82f, 0.7f, 0.9f);
	add_flat("poster_paper", 0.82f, 0.76f, 0.6f, 0.95f);
	add_flat("steel", 0.55f, 0.56f, 0.58f, 0.35f, 0.8f);
	add_flat("tree_bark", 0.12f, 0.1f, 0.08f, 1.0f);
	add_flat("tree_crown", 0.06f, 0.09f, 0.05f, 1.0f);
	add_flat("glass_booth", 0.3f, 0.4f, 0.45f, 0.1f, 0.2f);

	add_emissive("lamp_warm", 1.0f, 0.78f, 0.45f, 4.0f, false);
	add_emissive("lamp_cold", 0.8f, 0.9f, 1.0f, 3.0f, false);
	add_emissive("led_off", 0.05f, 0.05f, 0.05f, 0.0f, true);
	add_emissive("tv_glow", 0.5f, 0.65f, 1.0f, 2.0f, false);
	add_emissive("red_light", 1.0f, 0.1f, 0.05f, 6.0f, true);
	add_emissive("sun_disc", 1.0f, 0.6f, 0.3f, 8.0f, true);

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
		m->set_albedo(Color(0.04f, 0.07f, 0.06f, 0.88f));
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_roughness(0.05f);
		m->set_metallic(0.3f);
		m->set_specular(0.8f);
		materials["water"] = m;
	}
	{
		Ref<StandardMaterial3D> m;
		m.instantiate();
		m->set_albedo(Color(0.55f, 0.65f, 0.7f, 0.25f));
		m->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
		m->set_roughness(0.05f);
		m->set_metallic(0.4f);
		m->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
		materials["glass"] = m;
	}
}

void MaterialLibrary::add_surface(const std::string &name, const std::string &tex, float meters, float roughness, float normal_strength, bool world_space) {
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, textures[tex]);
	auto it = normals.find(tex);
	if (it != normals.end() && normal_strength > 0.0f) {
		m->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
		m->set_texture(BaseMaterial3D::TEXTURE_NORMAL, it->second);
		m->set_normal_scale(normal_strength);
	}
	m->set_roughness(roughness);
	m->set_flag(BaseMaterial3D::FLAG_UV1_USE_TRIPLANAR, true);
	m->set_flag(BaseMaterial3D::FLAG_UV1_USE_WORLD_TRIPLANAR, world_space);
	m->set_uv1_triplanar_blend_sharpness(8.0f);
	float s = 1.0f / meters;
	m->set_uv1_scale(Vector3(s, s, s));
	materials[name] = m;
}

void MaterialLibrary::add_flat(const std::string &name, float r, float g, float b, float roughness, float metallic) {
	Ref<StandardMaterial3D> m;
	m.instantiate();
	m->set_albedo(Color(r, g, b));
	m->set_roughness(roughness);
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

} // namespace urbex
