#pragma once

#include "core/builder.h"
#include "core/level_data.h"

#include <vector>

namespace urbex {

class UrbexGame;

struct LevelInfo {
	const char *id;
	godot::String title;
	godot::String tagline;
	godot::String description;
	godot::String difficulty;
	void (*build)(LevelBuilder &, LevelData &, UrbexGame &);
};

const std::vector<LevelInfo> &level_catalog();

void build_hospital(LevelBuilder &b, LevelData &data, UrbexGame &game);
void build_shelter(LevelBuilder &b, LevelData &data, UrbexGame &game);
void build_rooftop(LevelBuilder &b, LevelData &data, UrbexGame &game);

godot::String stalker_memo_text();
godot::String controls_text();

}
