#pragma once

#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/omni_light3d.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

namespace urbex {

class MaterialLibrary;

namespace fx {

godot::GPUParticles3D *dust(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &center, const godot::Vector3 &extents, float density = 1.0f);
godot::GPUParticles3D *drips(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &top, float fall, float per_second);
godot::GPUParticles3D *splash(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos, float per_second);
godot::GPUParticles3D *steam(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos, const godot::Vector3 &dir, float strength);
godot::GPUParticles3D *smoke_plume(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos, float size, const godot::Vector3 &wind, const godot::Color &color);
godot::GPUParticles3D *fire(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos, float radius);
godot::GPUParticles3D *embers(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos, float radius);
godot::GPUParticles3D *candle_flame(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos);
godot::GPUParticles3D *leaves(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &center, const godot::Vector3 &extents, int amount);
godot::GPUParticles3D *moths(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos, int amount = 7);
godot::GPUParticles3D *ground_fog(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &center, const godot::Vector3 &extents, float density = 1.0f);
godot::GPUParticles3D *birds(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &center, const godot::Vector3 &extents, const godot::Vector3 &heading);
godot::GPUParticles3D *dust_puff(godot::Node3D *parent, const MaterialLibrary &m, const godot::Vector3 &pos, float size);
godot::Node3D *decal(godot::Node3D *parent, const MaterialLibrary &m, const char *texture, const godot::Vector3 &pos, const godot::Vector3 &normal, const godot::Vector2 &size, float angle, float opacity = 1.0f);

}

class FxFlicker : public godot::OmniLight3D {
	GDCLASS(FxFlicker, godot::OmniLight3D)

public:
	enum Mode {
		MODE_FIRE,
		MODE_FLUORESCENT,
		MODE_BROKEN,
		MODE_CANDLE,
	};

private:
	Mode mode = MODE_FIRE;
	float base_energy = 1.0f;
	float clock = 0.0f;
	float next_event = 0.0f;
	float event_timer = 0.0f;
	bool off_phase = false;
	godot::String power_group;
	godot::Ref<godot::StandardMaterial3D> glow;
	float glow_energy = 1.0f;
	uint32_t seed = 1;

	float rand01();

protected:
	static void _bind_methods() {}

public:
	void setup(Mode p_mode, float energy, const godot::String &group, uint32_t p_seed);
	void set_glow(const godot::Ref<godot::StandardMaterial3D> &material);
	void _process(double delta) override;
};

class FxBeacon : public godot::Node3D {
	GDCLASS(FxBeacon, godot::Node3D)

	godot::SpotLight3D *spot = nullptr;
	godot::OmniLight3D *glow = nullptr;
	godot::Ref<godot::StandardMaterial3D> dome;
	godot::Color color_a;
	godot::Color color_b;
	bool always_on = false;
	bool alternate = false;
	float clock = 0.0f;

protected:
	static void _bind_methods() {}

public:
	void setup(const MaterialLibrary &m, const godot::Color &a, const godot::Color &b, bool p_always, bool p_alternate);
	void _process(double delta) override;
};

class FxSparks : public godot::Node3D {
	GDCLASS(FxSparks, godot::Node3D)

	godot::GPUParticles3D *particles = nullptr;
	godot::OmniLight3D *flash = nullptr;
	godot::String power_group;
	float timer = 2.0f;
	float flash_time = 0.0f;
	uint32_t seed = 7;
	float min_interval = 2.0f;
	float max_interval = 7.0f;

protected:
	static void _bind_methods() {}

public:
	void setup(const MaterialLibrary &m, const godot::String &group, float interval_min, float interval_max, uint32_t p_seed);
	void _process(double delta) override;
};

class FxDebris : public godot::Node3D {
	GDCLASS(FxDebris, godot::Node3D)

	godot::GPUParticles3D *chunks = nullptr;
	godot::GPUParticles3D *puff = nullptr;
	float timer = 5.0f;
	uint32_t seed = 3;

protected:
	static void _bind_methods() {}

public:
	void setup(const MaterialLibrary &m, uint32_t p_seed);
	void _process(double delta) override;
};

class FxFlash : public godot::OmniLight3D {
	GDCLASS(FxFlash, godot::OmniLight3D)

	float life = 0.12f;
	float total = 0.12f;
	float energy = 6.0f;

protected:
	static void _bind_methods() {}

public:
	void setup(float p_energy, float p_range, float p_life, const godot::Color &color);
	void _process(double delta) override;
};

}
