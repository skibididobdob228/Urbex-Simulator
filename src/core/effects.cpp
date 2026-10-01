#include "core/effects.h"

#include "actors/player.h"
#include "core/common.h"
#include "core/game.h"
#include "core/materials.h"

#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/curve_texture.hpp>
#include <godot_cpp/classes/decal.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture1_d.hpp>
#include <godot_cpp/classes/particle_process_material.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

using namespace godot;

namespace urbex {

namespace {

using PM = ParticleProcessMaterial;

Ref<StandardMaterial3D> particle_material(const MaterialLibrary &m, const char *tex, bool shaded, bool additive, const Color &tint, bool scissor = false) {
	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	mat->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, m.texture(tex));
	mat->set_albedo(tint);
	mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	mat->set_billboard_mode(BaseMaterial3D::BILLBOARD_PARTICLES);
	if (scissor) {
		mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
		mat->set_alpha_scissor_threshold(0.5f);
		mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
	} else {
		mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
	}
	if (!shaded) {
		mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
	}
	if (additive) {
		mat->set_blend_mode(BaseMaterial3D::BLEND_MODE_ADD);
	}
	mat->set_roughness(0.9f);
	return mat;
}

Ref<QuadMesh> quad(float w, float h, const Ref<Material> &mat) {
	Ref<QuadMesh> q;
	q.instantiate();
	q->set_size(Vector2(w, h));
	q->set_material(mat);
	return q;
}

Ref<GradientTexture1D> ramp(std::initializer_list<std::pair<float, Color>> points) {
	Ref<Gradient> g;
	g.instantiate();
	PackedFloat32Array offsets;
	PackedColorArray colors;
	for (const auto &p : points) {
		offsets.push_back(p.first);
		colors.push_back(p.second);
	}
	g->set_offsets(offsets);
	g->set_colors(colors);
	Ref<GradientTexture1D> t;
	t.instantiate();
	t->set_gradient(g);
	return t;
}

Ref<CurveTexture> curve(std::initializer_list<Vector2> points, float max_value) {
	Ref<Curve> c;
	c.instantiate();
	c->set_max_value(max_value);
	for (const Vector2 &p : points) {
		c->add_point(p);
	}
	Ref<CurveTexture> t;
	t.instantiate();
	t->set_curve(c);
	return t;
}

Ref<PM> process() {
	Ref<PM> pm;
	pm.instantiate();
	return pm;
}

void range(const Ref<PM> &pm, PM::Parameter p, float lo, float hi) {
	pm->set_param_min(p, lo);
	pm->set_param_max(p, hi);
}

GPUParticles3D *spawn(Node3D *parent, const Vector3 &pos, int amount, double lifetime, const Ref<PM> &pm, const Ref<Mesh> &mesh, const AABB &visibility) {
	GPUParticles3D *p = memnew(GPUParticles3D);
	p->set_amount(std::max(1, amount));
	p->set_lifetime(lifetime);
	p->set_process_material(pm);
	p->set_draw_pass_mesh(0, mesh);
	p->set_visibility_aabb(visibility);
	p->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	p->set_position(pos);
	parent->add_child(p);
	return p;
}

}

namespace fx {

GPUParticles3D *dust(Node3D *parent, const MaterialLibrary &m, const Vector3 &center, const Vector3 &extents, float density) {
	float volume = extents.x * extents.y * extents.z * 8.0f;
	int amount = std::clamp(int(volume * 0.5f * density), 24, 420);
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_BOX);
	pm->set_emission_box_extents(extents);
	pm->set_direction(Vector3(0, 1, 0));
	pm->set_spread(180.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.0f, 0.04f);
	pm->set_gravity(Vector3(0.0f, -0.004f, 0.0f));
	pm->set_turbulence_enabled(true);
	pm->set_turbulence_noise_strength(0.12f);
	pm->set_turbulence_noise_scale(1.6f);
	pm->set_turbulence_noise_speed(Vector3(0.02f, 0.01f, 0.02f));
	range(pm, PM::PARAM_TURB_VEL_INFLUENCE, 0.02f, 0.06f);
	range(pm, PM::PARAM_SCALE, 0.5f, 1.0f);
	pm->set_color_ramp(ramp({ { 0.0f, Color(1, 1, 1, 0) }, { 0.2f, Color(1, 1, 1, 0.6f) }, { 0.8f, Color(1, 1, 1, 0.6f) }, { 1.0f, Color(1, 1, 1, 0) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_soft", true, false, Color(0.85f, 0.82f, 0.75f, 1.0f));
	GPUParticles3D *p = spawn(parent, center, amount, 14.0, pm, quad(0.018f, 0.018f, mat), AABB(-extents - Vector3(1, 1, 1), extents * 2.0f + Vector3(2, 2, 2)));
	p->set_pre_process_time(14.0);
	return p;
}

GPUParticles3D *drips(Node3D *parent, const MaterialLibrary &m, const Vector3 &top, float fall, float per_second) {
	float life = std::sqrt(2.0f * std::max(0.2f, fall) / 9.8f);
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(0.03f);
	pm->set_direction(Vector3(0, -1, 0));
	pm->set_spread(0.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.0f, 0.05f);
	pm->set_gravity(Vector3(0.0f, -9.8f, 0.0f));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_hard", true, false, Color(0.7f, 0.8f, 0.85f, 0.75f));
	mat->set_roughness(0.1f);
	GPUParticles3D *p = spawn(parent, top, int(std::ceil(per_second * life)) + 1, life, pm, quad(0.014f, 0.02f, mat), AABB(Vector3(-0.5f, -fall - 0.5f, -0.5f), Vector3(1.0f, fall + 1.0f, 1.0f)));
	p->set_randomness_ratio(0.6f);
	return p;
}

GPUParticles3D *splash(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos, float per_second) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(0.02f);
	pm->set_direction(Vector3(0, 1, 0));
	pm->set_spread(55.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.4f, 1.0f);
	pm->set_gravity(Vector3(0.0f, -9.8f, 0.0f));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_hard", true, false, Color(0.7f, 0.8f, 0.85f, 0.7f));
	GPUParticles3D *p = spawn(parent, pos, int(per_second * 0.3f * 5.0f) + 2, 0.3, pm, quad(0.008f, 0.008f, mat), AABB(Vector3(-0.5f, -0.2f, -0.5f), Vector3(1.0f, 0.8f, 1.0f)));
	p->set_explosiveness_ratio(0.6f);
	return p;
}

GPUParticles3D *steam(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos, const Vector3 &dir, float strength) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(0.05f);
	pm->set_direction(dir.normalized());
	pm->set_spread(14.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.5f * strength, 1.1f * strength);
	pm->set_gravity(Vector3(0.0f, 0.45f, 0.0f));
	range(pm, PM::PARAM_DAMPING, 0.4f, 0.8f);
	range(pm, PM::PARAM_ANGLE, 0.0f, 360.0f);
	range(pm, PM::PARAM_ANGULAR_VELOCITY, -25.0f, 25.0f);
	pm->set_param_texture(PM::PARAM_SCALE, curve({ Vector2(0.0f, 0.25f), Vector2(1.0f, 1.0f) }, 1.0f));
	pm->set_color_ramp(ramp({ { 0.0f, Color(1, 1, 1, 0) }, { 0.15f, Color(1, 1, 1, 0.22f) }, { 1.0f, Color(1, 1, 1, 0) } }));
	pm->set_turbulence_enabled(true);
	pm->set_turbulence_noise_strength(0.4f);
	pm->set_turbulence_noise_scale(2.0f);
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_smoke", true, false, Color(0.92f, 0.93f, 0.95f, 1.0f));
	return spawn(parent, pos, int(30.0f * strength) + 6, 2.4, pm, quad(0.7f * strength, 0.7f * strength, mat), AABB(Vector3(-3, -1, -3), Vector3(6, 6, 6)));
}

GPUParticles3D *smoke_plume(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos, float size, const Vector3 &wind, const Color &color) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(size * 0.25f);
	pm->set_direction(Vector3(0, 1, 0));
	pm->set_spread(10.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, size * 0.6f, size * 0.9f);
	pm->set_gravity(wind);
	range(pm, PM::PARAM_DAMPING, 0.05f, 0.15f);
	range(pm, PM::PARAM_ANGLE, 0.0f, 360.0f);
	range(pm, PM::PARAM_ANGULAR_VELOCITY, -8.0f, 8.0f);
	pm->set_param_texture(PM::PARAM_SCALE, curve({ Vector2(0.0f, 0.3f), Vector2(1.0f, 1.0f) }, 1.0f));
	pm->set_color_ramp(ramp({ { 0.0f, Color(color.r, color.g, color.b, 0) }, { 0.1f, Color(color.r, color.g, color.b, color.a) }, { 1.0f, Color(color.r, color.g, color.b, 0) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_smoke", true, false, Color(1, 1, 1, 1));
	float span = size * 20.0f;
	GPUParticles3D *p = spawn(parent, pos, 60, 16.0, pm, quad(size * 4.0f, size * 4.0f, mat), AABB(Vector3(-span, -size * 2.0f, -span), Vector3(span * 2.0f, span * 2.0f, span * 2.0f)));
	p->set_pre_process_time(16.0);
	return p;
}

GPUParticles3D *fire(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos, float radius) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(radius * 0.6f);
	pm->set_direction(Vector3(0, 1, 0));
	pm->set_spread(15.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.4f, 0.9f);
	pm->set_gravity(Vector3(0.0f, 1.3f, 0.0f));
	range(pm, PM::PARAM_ANGLE, 0.0f, 360.0f);
	pm->set_param_texture(PM::PARAM_SCALE, curve({ Vector2(0.0f, 1.0f), Vector2(1.0f, 0.25f) }, 1.0f));
	pm->set_color_ramp(ramp({ { 0.0f, Color(1.0f, 0.9f, 0.5f, 0.0f) }, { 0.1f, Color(1.0f, 0.75f, 0.3f, 0.9f) }, { 0.5f, Color(1.0f, 0.35f, 0.05f, 0.6f) }, { 1.0f, Color(0.2f, 0.05f, 0.0f, 0.0f) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_soft", false, true, Color(1, 1, 1, 1));
	return spawn(parent, pos, 46, 0.75, pm, quad(radius * 1.3f, radius * 1.6f, mat), AABB(Vector3(-2, -1, -2), Vector3(4, 5, 4)));
}

GPUParticles3D *embers(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos, float radius) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(radius * 0.5f);
	pm->set_direction(Vector3(0, 1, 0));
	pm->set_spread(25.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.6f, 1.6f);
	pm->set_gravity(Vector3(0.0f, 0.3f, 0.0f));
	pm->set_turbulence_enabled(true);
	pm->set_turbulence_noise_strength(1.2f);
	pm->set_turbulence_noise_scale(2.0f);
	pm->set_color_ramp(ramp({ { 0.0f, Color(1.0f, 0.8f, 0.3f, 1.0f) }, { 0.6f, Color(1.0f, 0.35f, 0.05f, 0.8f) }, { 1.0f, Color(0.4f, 0.05f, 0.0f, 0.0f) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_hard", false, true, Color(1, 1, 1, 1));
	return spawn(parent, pos, 18, 2.4, pm, quad(0.025f, 0.025f, mat), AABB(Vector3(-3, -1, -3), Vector3(6, 8, 6)));
}

GPUParticles3D *candle_flame(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_POINT);
	pm->set_direction(Vector3(0, 1, 0));
	pm->set_spread(6.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.04f, 0.1f);
	pm->set_gravity(Vector3(0.0f, 0.15f, 0.0f));
	pm->set_param_texture(PM::PARAM_SCALE, curve({ Vector2(0.0f, 1.0f), Vector2(1.0f, 0.3f) }, 1.0f));
	pm->set_color_ramp(ramp({ { 0.0f, Color(1.0f, 0.95f, 0.7f, 0.9f) }, { 0.5f, Color(1.0f, 0.6f, 0.15f, 0.7f) }, { 1.0f, Color(0.5f, 0.1f, 0.0f, 0.0f) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_soft", false, true, Color(1, 1, 1, 1));
	return spawn(parent, pos, 10, 0.4, pm, quad(0.055f, 0.085f, mat), AABB(Vector3(-0.3f, -0.1f, -0.3f), Vector3(0.6f, 0.6f, 0.6f)));
}

GPUParticles3D *leaves(Node3D *parent, const MaterialLibrary &m, const Vector3 &center, const Vector3 &extents, int amount) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_BOX);
	pm->set_emission_box_extents(extents);
	pm->set_direction(Vector3(0, -1, 0));
	pm->set_spread(30.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.2f, 0.5f);
	pm->set_gravity(Vector3(0.25f, -0.55f, 0.1f));
	pm->set_turbulence_enabled(true);
	pm->set_turbulence_noise_strength(1.1f);
	pm->set_turbulence_noise_scale(2.5f);
	range(pm, PM::PARAM_ANGLE, 0.0f, 360.0f);
	range(pm, PM::PARAM_ANGULAR_VELOCITY, -220.0f, 220.0f);
	range(pm, PM::PARAM_HUE_VARIATION, -0.04f, 0.04f);
	range(pm, PM::PARAM_SCALE, 0.7f, 1.3f);
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_leaf", true, false, Color(1, 1, 1, 1), true);
	float h = extents.y * 2.0f + 10.0f;
	GPUParticles3D *p = spawn(parent, center, amount, 9.0, pm, quad(0.09f, 0.09f, mat), AABB(Vector3(-extents.x - 4.0f, -h, -extents.z - 4.0f), Vector3(extents.x * 2.0f + 8.0f, h * 2.0f, extents.z * 2.0f + 8.0f)));
	p->set_pre_process_time(9.0);
	return p;
}

GPUParticles3D *moths(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos, int amount) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(0.6f);
	pm->set_direction(Vector3(1, 0, 0));
	pm->set_spread(180.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.2f, 0.6f);
	pm->set_gravity(Vector3());
	pm->set_turbulence_enabled(true);
	pm->set_turbulence_noise_strength(4.0f);
	pm->set_turbulence_noise_scale(1.0f);
	pm->set_turbulence_noise_speed(Vector3(0.3f, 0.3f, 0.3f));
	pm->set_color_ramp(ramp({ { 0.0f, Color(1, 1, 1, 0) }, { 0.1f, Color(1, 1, 1, 0.9f) }, { 0.9f, Color(1, 1, 1, 0.9f) }, { 1.0f, Color(1, 1, 1, 0) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_hard", true, false, Color(0.85f, 0.8f, 0.65f, 1.0f));
	GPUParticles3D *p = spawn(parent, pos, amount, 5.0, pm, quad(0.035f, 0.025f, mat), AABB(Vector3(-3, -3, -3), Vector3(6, 6, 6)));
	p->set_pre_process_time(5.0);
	return p;
}

GPUParticles3D *ground_fog(Node3D *parent, const MaterialLibrary &m, const Vector3 &center, const Vector3 &extents, float density) {
	float area = extents.x * extents.z * 4.0f;
	int amount = std::clamp(int(area / 7.0f * density), 6, 120);
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_BOX);
	pm->set_emission_box_extents(Vector3(extents.x, std::max(0.05f, extents.y), extents.z));
	pm->set_direction(Vector3(1, 0, 0));
	pm->set_spread(180.0f);
	pm->set_flatness(1.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.03f, 0.12f);
	pm->set_gravity(Vector3());
	range(pm, PM::PARAM_ANGLE, 0.0f, 360.0f);
	range(pm, PM::PARAM_ANGULAR_VELOCITY, -4.0f, 4.0f);
	range(pm, PM::PARAM_SCALE, 0.7f, 1.3f);
	pm->set_color_ramp(ramp({ { 0.0f, Color(1, 1, 1, 0) }, { 0.3f, Color(1, 1, 1, 0.1f) }, { 0.7f, Color(1, 1, 1, 0.1f) }, { 1.0f, Color(1, 1, 1, 0) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_smoke", true, false, Color(0.8f, 0.82f, 0.86f, 1.0f));
	GPUParticles3D *p = spawn(parent, center, amount, 18.0, pm, quad(3.5f, 3.5f, mat), AABB(-extents - Vector3(4, 2, 4), extents * 2.0f + Vector3(8, 4, 8)));
	p->set_pre_process_time(18.0);
	return p;
}

GPUParticles3D *birds(Node3D *parent, const MaterialLibrary &m, const Vector3 &center, const Vector3 &extents, const Vector3 &heading) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_BOX);
	pm->set_emission_box_extents(extents);
	pm->set_direction(heading.normalized());
	pm->set_spread(8.0f);
	pm->set_flatness(0.8f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 5.0f, 7.0f);
	pm->set_gravity(Vector3());
	pm->set_turbulence_enabled(true);
	pm->set_turbulence_noise_strength(1.5f);
	pm->set_turbulence_noise_scale(9.0f);
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_hard", false, false, Color(0.05f, 0.05f, 0.06f, 0.95f));
	GPUParticles3D *p = spawn(parent, center, 26, 40.0, pm, quad(0.4f, 0.2f, mat), AABB(Vector3(-400, -60, -400), Vector3(800, 120, 800)));
	p->set_pre_process_time(30.0);
	return p;
}

GPUParticles3D *dust_puff(Node3D *parent, const MaterialLibrary &m, const Vector3 &pos, float size) {
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(0.12f);
	pm->set_direction(Vector3(0, 1, 0));
	pm->set_spread(80.0f);
	pm->set_flatness(0.5f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.3f, 0.8f);
	pm->set_gravity(Vector3(0.0f, -0.15f, 0.0f));
	range(pm, PM::PARAM_DAMPING, 1.5f, 2.5f);
	range(pm, PM::PARAM_ANGLE, 0.0f, 360.0f);
	pm->set_param_texture(PM::PARAM_SCALE, curve({ Vector2(0.0f, 0.35f), Vector2(1.0f, 1.0f) }, 1.0f));
	pm->set_color_ramp(ramp({ { 0.0f, Color(1, 1, 1, 0.3f) }, { 1.0f, Color(1, 1, 1, 0) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_smoke", true, false, Color(0.7f, 0.66f, 0.6f, 1.0f));
	GPUParticles3D *p = spawn(parent, pos, 10, 1.3, pm, quad(size, size, mat), AABB(Vector3(-2, -1, -2), Vector3(4, 3, 4)));
	p->set_one_shot(true);
	p->set_explosiveness_ratio(0.9f);
	p->set_emitting(true);
	p->connect("finished", callable_mp(static_cast<Node *>(p), &Node::queue_free));
	return p;
}

Node3D *decal(Node3D *parent, const MaterialLibrary &m, const char *texture, const Vector3 &pos, const Vector3 &normal, const Vector2 &size, float angle, float opacity) {
	Decal *d = memnew(Decal);
	d->set_texture(Decal::TEXTURE_ALBEDO, m.texture(texture));
	d->set_size(Vector3(size.x, 0.5f, size.y));
	d->set_modulate(Color(1, 1, 1, opacity));
	d->set_normal_fade(0.3f);
	d->set_upper_fade(0.2f);
	d->set_lower_fade(0.2f);
	Vector3 y = normal.normalized();
	Vector3 ref = std::fabs(y.y) > 0.9f ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
	Vector3 x = ref.cross(y).normalized();
	Vector3 z = x.cross(y).normalized();
	Basis b(x, y, z);
	b = Basis(y, angle) * b;
	d->set_transform(Transform3D(b, pos));
	parent->add_child(d);
	return d;
}

}

float FxFlicker::rand01() {
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
	return float(seed & 0xFFFFFF) / float(0xFFFFFF);
}

void FxFlicker::setup(Mode p_mode, float energy, const String &group, uint32_t p_seed) {
	mode = p_mode;
	base_energy = energy;
	power_group = group;
	seed = p_seed ? p_seed : 1;
	set_param(Light3D::PARAM_ENERGY, energy);
	event_timer = 1.0f + rand01() * 4.0f;
	if (mode == MODE_FLUORESCENT) {
		UrbexGame *game = UrbexGame::get_singleton();
		if (game) {
			AudioStreamPlayer3D *hum = memnew(AudioStreamPlayer3D);
			hum->set_stream(game->get_sounds().get("buzz"));
			hum->set_volume_db(-18.0f);
			hum->set_max_distance(9.0f);
			hum->set_unit_size(2.0f);
			hum->set_autoplay(true);
			add_child(hum);
		}
	}
}

void FxFlicker::set_glow(const Ref<StandardMaterial3D> &material) {
	glow = material;
	if (glow.is_valid()) {
		glow_energy = glow->get_emission_energy_multiplier();
	}
}

void FxFlicker::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	bool powered = power_group.is_empty() || (game && game->is_powered(power_group));
	if (!powered) {
		set_param(Light3D::PARAM_ENERGY, 0.0f);
		if (glow.is_valid()) {
			glow->set_emission_energy_multiplier(0.0f);
		}
		return;
	}
	float dt = float(delta);
	clock += dt;
	float k = 1.0f;
	switch (mode) {
		case MODE_FIRE:
			k = 0.72f + 0.14f * std::sin(clock * 13.1f) + 0.09f * std::sin(clock * 29.7f + 1.3f) + 0.1f * (rand01() - 0.5f);
			break;
		case MODE_CANDLE:
			k = 0.86f + 0.07f * std::sin(clock * 17.0f) + 0.05f * (rand01() - 0.5f);
			break;
		case MODE_FLUORESCENT:
			event_timer -= dt;
			if (off_phase) {
				k = rand01() < 0.5f ? 0.0f : 0.3f;
				if (event_timer <= 0.0f) {
					off_phase = false;
					event_timer = 2.0f + rand01() * 7.0f;
				}
			} else {
				k = 1.0f;
				if (event_timer <= 0.0f) {
					off_phase = true;
					event_timer = 0.15f + rand01() * 0.6f;
				}
			}
			break;
		case MODE_BROKEN:
			event_timer -= dt;
			if (off_phase) {
				k = rand01() < 0.6f ? 1.0f : 0.1f;
				if (event_timer <= 0.0f) {
					off_phase = false;
					event_timer = 1.5f + rand01() * 5.0f;
				}
			} else {
				k = 0.0f;
				if (event_timer <= 0.0f) {
					off_phase = true;
					event_timer = 0.1f + rand01() * 0.4f;
				}
			}
			break;
	}
	set_param(Light3D::PARAM_ENERGY, base_energy * std::max(0.0f, k));
	if (glow.is_valid()) {
		glow->set_emission_energy_multiplier(glow_energy * std::max(0.0f, k));
	}
}

void FxBeacon::setup(const MaterialLibrary &m, const Color &a, const Color &b, bool p_always, bool p_alternate) {
	color_a = a;
	color_b = b;
	always_on = p_always;
	alternate = p_alternate;
	Ref<SphereMesh> sm;
	sm.instantiate();
	sm->set_radius(0.12f);
	sm->set_height(0.16f);
	sm->set_radial_segments(12);
	sm->set_rings(6);
	dome.instantiate();
	dome->set_albedo(a);
	dome->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
	dome->set_emission(a);
	dome->set_emission_energy_multiplier(4.0f);
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_mesh(sm);
	mi->set_material_override(dome);
	mi->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
	add_child(mi);
	spot = memnew(SpotLight3D);
	spot->set_color(a);
	spot->set_param(Light3D::PARAM_ENERGY, 6.0f);
	spot->set_param(Light3D::PARAM_RANGE, 22.0f);
	spot->set_param(Light3D::PARAM_SPOT_ANGLE, 28.0f);
	spot->set_param(Light3D::PARAM_VOLUMETRIC_FOG_ENERGY, 1.5f);
	spot->set_rotation(Vector3(-0.15f, 0.0f, 0.0f));
	add_child(spot);
	glow = memnew(OmniLight3D);
	glow->set_color(a);
	glow->set_param(Light3D::PARAM_ENERGY, 0.8f);
	glow->set_param(Light3D::PARAM_RANGE, 4.0f);
	add_child(glow);
	(void)m;
}

void FxBeacon::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !spot) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	bool active = always_on || (game && game->is_alarm_active());
	spot->set_visible(active);
	glow->set_visible(active);
	dome->set_emission_energy_multiplier(active ? 4.0f : 0.0f);
	if (!active) {
		return;
	}
	clock += float(delta);
	set_rotation(Vector3(0.0f, clock * 7.0f, 0.0f));
	if (alternate) {
		Color c = std::fmod(clock, 0.6f) < 0.3f ? color_a : color_b;
		spot->set_color(c);
		glow->set_color(c);
		dome->set_emission(c);
		dome->set_albedo(c);
	}
}

void FxSparks::setup(const MaterialLibrary &m, const String &group, float interval_min, float interval_max, uint32_t p_seed) {
	power_group = group;
	min_interval = interval_min;
	max_interval = interval_max;
	seed = p_seed ? p_seed : 7;
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_SPHERE);
	pm->set_emission_sphere_radius(0.02f);
	pm->set_direction(Vector3(0, -0.3f, 1));
	pm->set_spread(70.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 1.2f, 3.2f);
	pm->set_gravity(Vector3(0.0f, -9.8f, 0.0f));
	range(pm, PM::PARAM_DAMPING, 0.3f, 0.8f);
	pm->set_color_ramp(ramp({ { 0.0f, Color(1.0f, 1.0f, 0.85f, 1.0f) }, { 0.4f, Color(1.0f, 0.65f, 0.2f, 1.0f) }, { 1.0f, Color(0.8f, 0.2f, 0.0f, 0.0f) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_hard", false, true, Color(1, 1, 1, 1));
	particles = spawn(this, Vector3(), 22, 0.8, pm, quad(0.025f, 0.025f, mat), AABB(Vector3(-3, -4, -3), Vector3(6, 6, 6)));
	particles->set_one_shot(true);
	particles->set_explosiveness_ratio(0.95f);
	particles->set_emitting(false);
	flash = memnew(OmniLight3D);
	flash->set_color(Color(1.0f, 0.8f, 0.5f));
	flash->set_param(Light3D::PARAM_RANGE, 5.0f);
	flash->set_param(Light3D::PARAM_ENERGY, 0.0f);
	add_child(flash);
	timer = min_interval + float(seed % 100) / 100.0f * (max_interval - min_interval);
}

void FxSparks::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !particles) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game || !game->is_playing()) {
		return;
	}
	float dt = float(delta);
	if (flash_time > 0.0f) {
		flash_time -= dt;
		flash->set_param(Light3D::PARAM_ENERGY, std::max(0.0f, flash_time) * 30.0f);
	}
	if (!power_group.is_empty() && !game->is_powered(power_group)) {
		return;
	}
	timer -= dt;
	if (timer > 0.0f) {
		return;
	}
	seed = seed * 1664525u + 1013904223u;
	timer = min_interval + float((seed >> 8) % 1000) / 1000.0f * (max_interval - min_interval);
	particles->restart();
	particles->set_emitting(true);
	flash_time = 0.1f;
	game->play_sound("spark", get_global_position(), -6.0f, 0.9f + float(seed % 20) * 0.01f, 25.0f);
}

void FxDebris::setup(const MaterialLibrary &m, uint32_t p_seed) {
	seed = p_seed ? p_seed : 3;
	Ref<PM> pm = process();
	pm->set_emission_shape(PM::EMISSION_SHAPE_BOX);
	pm->set_emission_box_extents(Vector3(0.35f, 0.02f, 0.35f));
	pm->set_direction(Vector3(0, -1, 0));
	pm->set_spread(25.0f);
	range(pm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.0f, 0.4f);
	pm->set_gravity(Vector3(0.0f, -9.8f, 0.0f));
	range(pm, PM::PARAM_ANGULAR_VELOCITY, -300.0f, 300.0f);
	range(pm, PM::PARAM_SCALE, 0.4f, 1.2f);
	pm->set_particle_flag(PM::PARTICLE_FLAG_ROTATE_Y, true);
	Ref<BoxMesh> box;
	box.instantiate();
	box->set_size(Vector3(0.04f, 0.025f, 0.035f));
	box->set_material(m.get("plaster"));
	chunks = spawn(this, Vector3(), 12, 1.4, pm, box, AABB(Vector3(-2, -6, -2), Vector3(4, 7, 4)));
	chunks->set_one_shot(true);
	chunks->set_explosiveness_ratio(0.85f);
	chunks->set_emitting(false);

	Ref<PM> dm = process();
	dm->set_emission_shape(PM::EMISSION_SHAPE_BOX);
	dm->set_emission_box_extents(Vector3(0.3f, 0.02f, 0.3f));
	dm->set_direction(Vector3(0, -1, 0));
	dm->set_spread(20.0f);
	range(dm, PM::PARAM_INITIAL_LINEAR_VELOCITY, 0.1f, 0.4f);
	dm->set_gravity(Vector3(0.0f, -0.6f, 0.0f));
	range(dm, PM::PARAM_ANGLE, 0.0f, 360.0f);
	dm->set_param_texture(PM::PARAM_SCALE, curve({ Vector2(0.0f, 0.3f), Vector2(1.0f, 1.0f) }, 1.0f));
	dm->set_color_ramp(ramp({ { 0.0f, Color(1, 1, 1, 0.35f) }, { 1.0f, Color(1, 1, 1, 0) } }));
	Ref<StandardMaterial3D> mat = particle_material(m, "fx_smoke", true, false, Color(0.75f, 0.72f, 0.66f, 1.0f));
	puff = spawn(this, Vector3(), 14, 3.0, dm, quad(0.5f, 0.5f, mat), AABB(Vector3(-2, -5, -2), Vector3(4, 6, 4)));
	puff->set_one_shot(true);
	puff->set_explosiveness_ratio(0.7f);
	puff->set_emitting(false);
	timer = 4.0f + float(seed % 7);
}

void FxDebris::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint() || !chunks) {
		return;
	}
	UrbexGame *game = UrbexGame::get_singleton();
	if (!game || !game->is_playing() || !game->get_player()) {
		return;
	}
	timer -= float(delta);
	if (timer > 0.0f) {
		return;
	}
	seed = seed * 1664525u + 1013904223u;
	timer = 9.0f + float((seed >> 8) % 1200) / 100.0f;
	if ((game->get_player()->get_global_position() - get_global_position()).length() > 20.0f) {
		return;
	}
	chunks->restart();
	chunks->set_emitting(true);
	puff->restart();
	puff->set_emitting(true);
	game->play_sound("crumble", get_global_position() + Vector3(0, -1.5f, 0), -8.0f, 0.9f + float(seed % 20) * 0.01f, 25.0f);
}

void FxFlash::setup(float p_energy, float p_range, float p_life, const Color &color) {
	energy = p_energy;
	life = p_life;
	total = p_life;
	set_color(color);
	set_param(Light3D::PARAM_RANGE, p_range);
	set_param(Light3D::PARAM_ENERGY, energy);
	set_shadow(false);
}

void FxFlash::_process(double delta) {
	life -= float(delta);
	if (life <= 0.0f) {
		queue_free();
		return;
	}
	set_param(Light3D::PARAM_ENERGY, energy * (life / total));
}

}
