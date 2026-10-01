#pragma once

#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

#include <string>
#include <unordered_map>

namespace urbex {

class MaterialLibrary {
public:
	void build();
	godot::Ref<godot::StandardMaterial3D> get(const std::string &name) const;
	godot::Ref<godot::ImageTexture> texture(const std::string &name) const;
	bool is_built() const { return built; }

private:
	void add_surface(const std::string &name, const std::string &tex, float meters, float roughness, float normal_strength, bool world_space);
	void add_flat(const std::string &name, float r, float g, float b, float roughness, float metallic = 0.0f);
	void add_emissive(const std::string &name, float r, float g, float b, float energy, bool unshaded);

	std::unordered_map<std::string, godot::Ref<godot::StandardMaterial3D>> materials;
	std::unordered_map<std::string, godot::Ref<godot::ImageTexture>> textures;
	std::unordered_map<std::string, godot::Ref<godot::ImageTexture>> normals;
	bool built = false;
};

} // namespace urbex
