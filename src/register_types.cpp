#include "register_types.h"

#include "actors/guard.h"
#include "actors/player.h"
#include "core/effects.h"
#include "core/game.h"
#include "security/devices.h"
#include "ui/hud.h"
#include "world/door.h"
#include "world/interactable.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_urbex_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_RUNTIME_CLASS(urbex::UrbexGame);
	GDREGISTER_RUNTIME_CLASS(urbex::UrbexPlayer);
	GDREGISTER_RUNTIME_CLASS(urbex::Guard);
	GDREGISTER_RUNTIME_CLASS(urbex::Interactable);
	GDREGISTER_RUNTIME_CLASS(urbex::Pickup);
	GDREGISTER_RUNTIME_CLASS(urbex::ActionPoint);
	GDREGISTER_RUNTIME_CLASS(urbex::NoteBoard);
	GDREGISTER_RUNTIME_CLASS(urbex::ClimbPoint);
	GDREGISTER_RUNTIME_CLASS(urbex::PowerSwitch);
	GDREGISTER_RUNTIME_CLASS(urbex::ReedSwitch);
	GDREGISTER_RUNTIME_CLASS(urbex::Door);
	GDREGISTER_RUNTIME_CLASS(urbex::SecurityDevice);
	GDREGISTER_RUNTIME_CLASS(urbex::MotionSensor);
	GDREGISTER_RUNTIME_CLASS(urbex::LaserBarrier);
	GDREGISTER_RUNTIME_CLASS(urbex::SecurityCamera);
	GDREGISTER_RUNTIME_CLASS(urbex::PhotoSpot);
	GDREGISTER_RUNTIME_CLASS(urbex::FxFlicker);
	GDREGISTER_RUNTIME_CLASS(urbex::FxBeacon);
	GDREGISTER_RUNTIME_CLASS(urbex::FxSparks);
	GDREGISTER_RUNTIME_CLASS(urbex::FxDebris);
	GDREGISTER_RUNTIME_CLASS(urbex::FxFlash);
	GDREGISTER_RUNTIME_CLASS(urbex::UrbexHud);
	GDREGISTER_RUNTIME_CLASS(urbex::MainMenu);
}

void uninitialize_urbex_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
GDExtensionBool GDE_EXPORT urbex_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_urbex_module);
	init_obj.register_terminator(uninitialize_urbex_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
}
