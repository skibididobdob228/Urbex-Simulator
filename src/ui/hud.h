#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/progress_bar.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include <vector>

namespace urbex {

godot::Ref<godot::Theme> make_ui_theme();

class UrbexHud : public godot::CanvasLayer {
	GDCLASS(UrbexHud, godot::CanvasLayer)

	godot::Control *root = nullptr;
	godot::Control *crosshair = nullptr;
	godot::Label *prompt = nullptr;
	godot::Label *objectives = nullptr;
	godot::Label *level_title = nullptr;
	godot::Label *status = nullptr;
	godot::Label *inventory = nullptr;
	godot::ProgressBar *stamina_bar = nullptr;
	godot::ProgressBar *noise_bar = nullptr;
	godot::ProgressBar *light_bar = nullptr;
	godot::ProgressBar *battery_bar = nullptr;
	godot::ProgressBar *detection_bar = nullptr;
	godot::Label *detection_label = nullptr;
	godot::PanelContainer *alarm_panel = nullptr;
	godot::Label *alarm_label = nullptr;
	godot::VBoxContainer *messages = nullptr;
	godot::Label *subtitle = nullptr;
	godot::Control *viewfinder = nullptr;
	godot::Label *viewfinder_hint = nullptr;
	godot::ColorRect *flash_rect = nullptr;
	godot::ColorRect *fade_rect = nullptr;
	godot::ColorRect *vignette = nullptr;
	godot::PanelContainer *note_panel = nullptr;
	godot::Label *note_title = nullptr;
	godot::RichTextLabel *note_body = nullptr;
	godot::Label *center_text = nullptr;

	godot::Control *pause_menu = nullptr;
	godot::Control *result_screen = nullptr;
	godot::Label *result_title = nullptr;
	godot::RichTextLabel *result_body = nullptr;
	godot::Button *result_retry = nullptr;

	struct Message {
		godot::Label *label = nullptr;
		float life = 0.0f;
	};
	std::vector<Message> message_list;
	float subtitle_timer = 0.0f;
	float flash = 0.0f;
	float alarm_clock = 0.0f;
	bool alarm_on = false;
	float center_timer = 0.0f;

	godot::Control *make_bar_row(godot::Control *parent, const godot::String &name, godot::ProgressBar **bar, const godot::Color &color);

protected:
	static void _bind_methods() {}

public:
	void build();
	void tick(double delta);

	void set_prompt(const godot::String &text);
	void set_objectives(const godot::String &text);
	void set_level_title(const godot::String &text);
	void set_status(const godot::String &text);
	void set_inventory(const godot::String &text);
	void set_bars(float stamina, float noise, float light, float battery);
	void set_detection(float value, bool chase);
	void set_alarm(bool active, const godot::String &text);
	void push_message(const godot::String &text, const godot::Color &color);
	void show_subtitle(const godot::String &who, const godot::String &line);
	void set_viewfinder(bool visible, const godot::String &hint);
	void trigger_flash();
	void set_fade(float alpha);
	void show_center(const godot::String &text, float seconds);
	void show_note(const godot::String &title, const godot::String &body);
	void hide_note();
	bool is_note_open() const;
	void set_gameplay_visible(bool visible);

	void show_pause(bool visible);
	void show_result(const godot::String &title, const godot::Color &title_color, const godot::String &body);
	void hide_result();
	bool is_pause_visible() const;

	godot::Button *pause_resume = nullptr;
	godot::Button *pause_restart = nullptr;
	godot::Button *pause_menu_button = nullptr;
	godot::Button *result_menu = nullptr;
	godot::Button *get_result_retry() const { return result_retry; }
};

class MainMenu : public godot::Control {
	GDCLASS(MainMenu, godot::Control)

	godot::VBoxContainer *level_list = nullptr;
	godot::RichTextLabel *details = nullptr;
	godot::Control *info_panel = nullptr;
	godot::RichTextLabel *info_text = nullptr;
	godot::Label *sens_label = nullptr;
	int selected = 0;
	std::vector<godot::Button *> level_buttons;

	void select_level(int index);
	void start_selected();
	void show_info(int page);
	void hide_info();
	void on_sensitivity(double value);

protected:
	static void _bind_methods() {}

public:
	void build();
	void refresh();
};

}
