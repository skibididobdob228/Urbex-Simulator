#pragma once

#include <godot_cpp/classes/audio_stream_wav.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace urbex {

class SoundBank {
public:
	void build();
	godot::Ref<godot::AudioStreamWAV> get(const std::string &name) const;
	godot::Ref<godot::AudioStreamWAV> variant(const std::string &base, int index) const;
	bool is_built() const { return built; }

private:
	void store(const std::string &name, const std::vector<float> &samples, bool loop = false);

	std::unordered_map<std::string, godot::Ref<godot::AudioStreamWAV>> sounds;
	std::unordered_map<std::string, int> variant_counts;
	bool built = false;
};

}
