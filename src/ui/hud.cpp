#include "ui/hud.h"

#include "core/common.h"
#include "core/game.h"
#include "levels/levels.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

#include <algorithm>

using namespace godot;

namespace urbex {

namespace {

const Color ACCENT(0.93f, 0.52f, 0.2f);
const Color TEXT(0.92f, 0.92f, 0.88f);
const Color MUTED(0.62f, 0.64f, 0.62f);

Ref<StyleBoxFlat> flat_box(const Color &bg, const Color &border, int border_width, int radius, float padding) {
	Ref<StyleBoxFlat> s;
	s.instantiate();
	s->set_bg_color(bg);
	s->set_border_color(border);
	s->set_border_width_all(border_width);
	s->set_corner_radius_all(radius);
	s->set_content_margin_all(padding);
	return s;
}

template <typename T>
T *add(Node *parent) {
	T *node = memnew(T);
	parent->add_child(node);
	return node;
}

void anchor(Control *c, float l, float t, float r, float b, float ol, float ot, float orr, float ob) {
	c->set_anchor(SIDE_LEFT, l);
	c->set_anchor(SIDE_TOP, t);
	c->set_anchor(SIDE_RIGHT, r);
	c->set_anchor(SIDE_BOTTOM, b);
	c->set_offset(SIDE_LEFT, ol);
	c->set_offset(SIDE_TOP, ot);
	c->set_offset(SIDE_RIGHT, orr);
	c->set_offset(SIDE_BOTTOM, ob);
}

Label *make_label(Node *parent, const String &text, int size, const Color &color) {
	Label *l = add<Label>(parent);
	l->set_text(text);
	l->add_theme_font_size_override("font_size", size);
	l->add_theme_color_override("font_color", color);
	l->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return l;
}

Button *make_button(Node *parent, const String &text, int min_width = 260) {
	Button *b = add<Button>(parent);
	b->set_text(text);
	b->set_custom_minimum_size(Vector2(float(min_width), 44.0f));
	b->add_theme_font_size_override("font_size", 19);
	return b;
}

ColorRect *make_rect(Node *parent, const Color &color) {
	ColorRect *r = add<ColorRect>(parent);
	r->set_color(color);
	r->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	return r;
}

} // namespace

Ref<Theme> make_ui_theme() {
	Ref<Theme> t;
	t.instantiate();
	t->set_default_font_size(18);
	t->set_stylebox("panel", "PanelContainer", flat_box(Color(0.05f, 0.055f, 0.065f, 0.82f), Color(ACCENT.r, ACCENT.g, ACCENT.b, 0.35f), 1, 4, 12.0f));
	t->set_stylebox("panel", "Panel", flat_box(Color(0.05f, 0.055f, 0.065f, 0.9f), Color(ACCENT.r, ACCENT.g, ACCENT.b, 0.35f), 1, 4, 12.0f));
	t->set_stylebox("normal", "Button", flat_box(Color(0.11f, 0.12f, 0.135f, 0.95f), Color(0.3f, 0.31f, 0.33f), 1, 3, 10.0f));
	t->set_stylebox("hover", "Button", flat_box(Color(0.2f, 0.15f, 0.11f, 0.95f), ACCENT, 1, 3, 10.0f));
	t->set_stylebox("pressed", "Button", flat_box(Color(0.35f, 0.2f, 0.1f, 0.95f), ACCENT, 2, 3, 10.0f));
	t->set_stylebox("focus", "Button", flat_box(Color(0, 0, 0, 0), ACCENT, 1, 3, 10.0f));
	t->set_stylebox("disabled", "Button", flat_box(Color(0.08f, 0.08f, 0.09f, 0.9f), Color(0.2f, 0.2f, 0.2f), 1, 3, 10.0f));
	t->set_color("font_color", "Button", TEXT);
	t->set_color("font_hover_color", "Button", Color(1.0f, 0.82f, 0.6f));
	t->set_color("font_pressed_color", "Button", Color(1.0f, 0.9f, 0.75f));
	t->set_color("font_focus_color", "Button", TEXT);
	t->set_color("font_color", "Label", TEXT);
	t->set_color("font_outline_color", "Label", Color(0, 0, 0, 0.85f));
	t->set_constant("outline_size", "Label", 4);
	t->set_color("default_color", "RichTextLabel", TEXT);
	t->set_stylebox("background", "ProgressBar", flat_box(Color(0.1f, 0.1f, 0.11f, 0.85f), Color(0.25f, 0.25f, 0.27f), 1, 2, 0.0f));
	t->set_stylebox("fill", "ProgressBar", flat_box(ACCENT, Color(0, 0, 0, 0), 0, 2, 0.0f));
	return t;
}

Control *UrbexHud::make_bar_row(Control *parent, const String &name, ProgressBar **bar, const Color &color) {
	HBoxContainer *row = add<HBoxContainer>(parent);
	row->add_theme_constant_override("separation", 10);
	Label *l = make_label(row, name, 15, MUTED);
	l->set_custom_minimum_size(Vector2(96.0f, 0.0f));
	ProgressBar *pb = add<ProgressBar>(row);
	pb->set_show_percentage(false);
	pb->set_min(0.0);
	pb->set_max(1.0);
	pb->set_step(0.001);
	pb->set_custom_minimum_size(Vector2(170.0f, 10.0f));
	pb->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	pb->add_theme_stylebox_override("fill", flat_box(color, Color(0, 0, 0, 0), 0, 2, 0.0f));
	pb->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	*bar = pb;
	return row;
}

void UrbexHud::build() {
	if (root) {
		return;
	}
	set_layer(10);
	root = add<Control>(this);
	anchor(root, 0, 0, 1, 1, 0, 0, 0, 0);
	root->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	root->set_theme(make_ui_theme());

	crosshair = make_rect(root, Color(1, 1, 1, 0.75f));
	anchor(crosshair, 0.5f, 0.5f, 0.5f, 0.5f, -2, -2, 2, 2);

	prompt = make_label(root, String(), 20, Color(1.0f, 0.95f, 0.85f));
	anchor(prompt, 0.5f, 0.5f, 0.5f, 0.5f, -500, 34, 500, 70);
	prompt->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

	PanelContainer *obj_panel = add<PanelContainer>(root);
	anchor(obj_panel, 0, 0, 0, 0, 18, 18, 470, 60);
	obj_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	VBoxContainer *obj_box = add<VBoxContainer>(obj_panel);
	level_title = make_label(obj_box, String(), 20, ACCENT);
	objectives = make_label(obj_box, String(), 16, TEXT);
	objectives->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	objectives->set_custom_minimum_size(Vector2(430.0f, 0.0f));

	PanelContainer *status_panel = add<PanelContainer>(root);
	anchor(status_panel, 0, 1, 0, 1, 18, -186, 330, -18);
	status_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	VBoxContainer *status_box = add<VBoxContainer>(status_panel);
	status_box->add_theme_constant_override("separation", 5);
	status = make_label(status_box, String(), 16, TEXT);
	make_bar_row(status_box, "Дыхалка"_u, &stamina_bar, Color(0.35f, 0.75f, 0.45f));
	make_bar_row(status_box, "Шум"_u, &noise_bar, Color(0.9f, 0.75f, 0.25f));
	make_bar_row(status_box, "На свету"_u, &light_bar, Color(0.95f, 0.95f, 0.8f));
	make_bar_row(status_box, "Фонарь"_u, &battery_bar, Color(0.45f, 0.65f, 0.95f));

	PanelContainer *inv_panel = add<PanelContainer>(root);
	anchor(inv_panel, 1, 1, 1, 1, -330, -150, -18, -18);
	inv_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	inv_panel->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
	inv_panel->set_v_grow_direction(Control::GROW_DIRECTION_BEGIN);
	inventory = make_label(inv_panel, String(), 15, TEXT);

	VBoxContainer *det_box = add<VBoxContainer>(root);
	anchor(det_box, 0.5f, 0, 0.5f, 0, -170, 16, 170, 60);
	det_box->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	detection_label = make_label(det_box, "ЗАМЕТНОСТЬ"_u, 14, MUTED);
	detection_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	detection_bar = add<ProgressBar>(det_box);
	detection_bar->set_show_percentage(false);
	detection_bar->set_min(0.0);
	detection_bar->set_max(1.0);
	detection_bar->set_step(0.001);
	detection_bar->set_custom_minimum_size(Vector2(340.0f, 8.0f));
	detection_bar->add_theme_stylebox_override("fill", flat_box(Color(1, 0.8f, 0.2f), Color(0, 0, 0, 0), 0, 2, 0.0f));
	detection_bar->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);

	alarm_panel = add<PanelContainer>(root);
	anchor(alarm_panel, 0.5f, 0, 0.5f, 0, -260, 74, 260, 120);
	alarm_panel->add_theme_stylebox_override("panel", flat_box(Color(0.55f, 0.05f, 0.04f, 0.88f), Color(1, 0.3f, 0.2f), 2, 4, 8.0f));
	alarm_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	alarm_label = make_label(alarm_panel, String(), 18, Color(1, 0.95f, 0.9f));
	alarm_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	alarm_panel->set_visible(false);

	messages = add<VBoxContainer>(root);
	anchor(messages, 1, 0, 1, 0, -520, 140, -18, 400);
	messages->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	messages->add_theme_constant_override("separation", 4);

	subtitle = make_label(root, String(), 20, Color(1.0f, 1.0f, 0.92f));
	anchor(subtitle, 0.5f, 1, 0.5f, 1, -600, -150, 600, -110);
	subtitle->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	subtitle->add_theme_constant_override("outline_size", 6);

	center_text = make_label(root, String(), 34, Color(1.0f, 0.95f, 0.85f));
	anchor(center_text, 0.5f, 0.35f, 0.5f, 0.35f, -700, -60, 700, 60);
	center_text->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	center_text->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
	center_text->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	center_text->add_theme_constant_override("outline_size", 8);

	viewfinder = add<Control>(root);
	anchor(viewfinder, 0, 0, 1, 1, 0, 0, 0, 0);
	viewfinder->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	const float m = 70.0f;
	const float len = 60.0f;
	const float th = 3.0f;
	struct Corner {
		float ax, ay;
		float sx, sy;
	};
	for (const Corner &c : { Corner{ 0, 0, 1, 1 }, Corner{ 1, 0, -1, 1 }, Corner{ 0, 1, 1, -1 }, Corner{ 1, 1, -1, -1 } }) {
		ColorRect *h = make_rect(viewfinder, Color(1, 1, 1, 0.8f));
		float x0 = c.sx > 0 ? m : -m - len;
		float y0 = c.sy > 0 ? m : -m - th;
		anchor(h, c.ax, c.ay, c.ax, c.ay, x0, y0, x0 + len, y0 + th);
		ColorRect *v = make_rect(viewfinder, Color(1, 1, 1, 0.8f));
		float vx0 = c.sx > 0 ? m : -m - th;
		float vy0 = c.sy > 0 ? m : -m - len;
		anchor(v, c.ax, c.ay, c.ax, c.ay, vx0, vy0, vx0 + th, vy0 + len);
	}
	for (float f : { 1.0f / 3.0f, 2.0f / 3.0f }) {
		ColorRect *gv = make_rect(viewfinder, Color(1, 1, 1, 0.12f));
		anchor(gv, f, 0, f, 1, 0, m, 1, -m);
		ColorRect *gh = make_rect(viewfinder, Color(1, 1, 1, 0.12f));
		anchor(gh, 0, f, 1, f, m, 0, -m, 1);
	}
	Label *rec = make_label(viewfinder, "● ФОТО"_u, 18, Color(1.0f, 0.3f, 0.25f));
	anchor(rec, 0, 0, 0, 0, m + 14, m + 12, m + 200, m + 40);
	viewfinder_hint = make_label(viewfinder, String(), 20, Color(0.75f, 1.0f, 0.75f));
	anchor(viewfinder_hint, 0.5f, 1, 0.5f, 1, -500, -m - 50, 500, -m - 14);
	viewfinder_hint->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	viewfinder->set_visible(false);

	flash_rect = make_rect(root, Color(1, 1, 1, 0));
	anchor(flash_rect, 0, 0, 1, 1, 0, 0, 0, 0);
	fade_rect = make_rect(root, Color(0, 0, 0, 0));
	anchor(fade_rect, 0, 0, 1, 1, 0, 0, 0, 0);

	note_panel = add<PanelContainer>(root);
	anchor(note_panel, 0.5f, 0.5f, 0.5f, 0.5f, -390, -270, 390, 270);
	VBoxContainer *note_box = add<VBoxContainer>(note_panel);
	note_box->add_theme_constant_override("separation", 10);
	note_title = make_label(note_box, String(), 24, ACCENT);
	note_body = add<RichTextLabel>(note_box);
	note_body->set_use_bbcode(true);
	note_body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	note_body->add_theme_font_size_override("normal_font_size", 17);
	note_body->add_theme_font_size_override("bold_font_size", 17);
	Label *note_hint = make_label(note_box, "[E] или [Esc] — закрыть"_u, 14, MUTED);
	note_hint->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	note_panel->set_visible(false);

	pause_menu = add<Control>(root);
	anchor(pause_menu, 0, 0, 1, 1, 0, 0, 0, 0);
	ColorRect *dim = make_rect(pause_menu, Color(0, 0, 0, 0.6f));
	anchor(dim, 0, 0, 1, 1, 0, 0, 0, 0);
	VBoxContainer *pause_box = add<VBoxContainer>(pause_menu);
	anchor(pause_box, 0.5f, 0.5f, 0.5f, 0.5f, -150, -130, 150, 130);
	pause_box->add_theme_constant_override("separation", 12);
	Label *pause_title = make_label(pause_box, "ПАУЗА"_u, 34, ACCENT);
	pause_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	pause_resume = make_button(pause_box, "Продолжить"_u);
	pause_restart = make_button(pause_box, "Начать вылазку заново"_u);
	pause_menu_button = make_button(pause_box, "В главное меню"_u);
	pause_menu->set_visible(false);

	result_screen = add<Control>(root);
	anchor(result_screen, 0, 0, 1, 1, 0, 0, 0, 0);
	ColorRect *rdim = make_rect(result_screen, Color(0, 0, 0, 0.72f));
	anchor(rdim, 0, 0, 1, 1, 0, 0, 0, 0);
	PanelContainer *rpanel = add<PanelContainer>(result_screen);
	anchor(rpanel, 0.5f, 0.5f, 0.5f, 0.5f, -420, -300, 420, 300);
	VBoxContainer *rbox = add<VBoxContainer>(rpanel);
	rbox->add_theme_constant_override("separation", 12);
	result_title = make_label(rbox, String(), 38, ACCENT);
	result_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	result_body = add<RichTextLabel>(rbox);
	result_body->set_use_bbcode(true);
	result_body->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	result_body->add_theme_font_size_override("normal_font_size", 18);
	result_body->add_theme_font_size_override("bold_font_size", 18);
	HBoxContainer *rbuttons = add<HBoxContainer>(rbox);
	rbuttons->set_alignment(BoxContainer::ALIGNMENT_CENTER);
	rbuttons->add_theme_constant_override("separation", 16);
	result_retry = make_button(rbuttons, "Ещё раз"_u, 220);
	result_menu = make_button(rbuttons, "В главное меню"_u, 220);
	result_screen->set_visible(false);
}

void UrbexHud::tick(double delta) {
	float dt = float(delta);
	for (size_t i = 0; i < message_list.size();) {
		Message &m = message_list[i];
		m.life -= dt;
		if (m.life <= 0.0f) {
			m.label->queue_free();
			message_list.erase(message_list.begin() + long(i));
			continue;
		}
		float a = std::min(1.0f, m.life / 1.2f);
		m.label->set_modulate(Color(1, 1, 1, a));
		i++;
	}
	if (subtitle_timer > 0.0f) {
		subtitle_timer -= dt;
		subtitle->set_modulate(Color(1, 1, 1, std::min(1.0f, subtitle_timer / 0.6f)));
		if (subtitle_timer <= 0.0f) {
			subtitle->set_text(String());
		}
	}
	if (center_timer > 0.0f) {
		center_timer -= dt;
		center_text->set_modulate(Color(1, 1, 1, std::min(1.0f, center_timer / 1.0f)));
		if (center_timer <= 0.0f) {
			center_text->set_text(String());
		}
	}
	if (flash > 0.0f) {
		flash = std::max(0.0f, flash - dt * 3.5f);
		flash_rect->set_color(Color(1, 1, 1, flash * 0.8f));
	}
	if (alarm_on) {
		alarm_clock += dt;
		float pulse = 0.65f + 0.35f * std::sin(alarm_clock * 7.0f);
		alarm_panel->set_modulate(Color(1, 1, 1, pulse));
	}
}

void UrbexHud::set_prompt(const String &text) {
	prompt->set_text(text);
}

void UrbexHud::set_objectives(const String &text) {
	objectives->set_text(text);
}

void UrbexHud::set_level_title(const String &text) {
	level_title->set_text(text);
}

void UrbexHud::set_status(const String &text) {
	status->set_text(text);
}

void UrbexHud::set_inventory(const String &text) {
	inventory->set_text(text);
}

void UrbexHud::set_bars(float stamina, float noise, float light, float battery) {
	stamina_bar->set_value(stamina);
	noise_bar->set_value(noise);
	light_bar->set_value(light);
	battery_bar->set_value(battery);
}

void UrbexHud::set_detection(float value, bool chase) {
	detection_bar->set_value(chase ? 1.0 : value);
	Ref<StyleBoxFlat> fill = detection_bar->get_theme_stylebox("fill");
	Color c = chase ? Color(1.0f, 0.15f, 0.1f) : Color(1.0f, 1.0f - value * 0.8f, 0.2f);
	if (fill.is_valid()) {
		fill->set_bg_color(c);
	}
	if (chase) {
		detection_label->set_text("ПОГОНЯ! БЕГИ И ПРЯЧЬСЯ"_u);
		detection_label->add_theme_color_override("font_color", Color(1.0f, 0.3f, 0.25f));
	} else if (value > 0.01f) {
		detection_label->set_text("ТЕБЯ ЗАМЕЧАЮТ"_u);
		detection_label->add_theme_color_override("font_color", Color(1.0f, 0.85f, 0.4f));
	} else {
		detection_label->set_text("НЕЗАМЕТЕН"_u);
		detection_label->add_theme_color_override("font_color", MUTED);
	}
}

void UrbexHud::set_alarm(bool active, const String &text) {
	alarm_on = active;
	alarm_panel->set_visible(active);
	alarm_label->set_text(text);
}

void UrbexHud::push_message(const String &text, const Color &color) {
	Label *l = make_label(messages, text, 17, color);
	l->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	l->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	l->set_custom_minimum_size(Vector2(500.0f, 0.0f));
	message_list.push_back({ l, 6.0f });
	while (message_list.size() > 6) {
		message_list.front().label->queue_free();
		message_list.erase(message_list.begin());
	}
}

void UrbexHud::show_subtitle(const String &who, const String &line) {
	subtitle->set_text(who + String(": ") + line);
	subtitle_timer = 3.5f;
}

void UrbexHud::set_viewfinder(bool visible, const String &hint) {
	viewfinder->set_visible(visible);
	crosshair->set_visible(!visible);
	viewfinder_hint->set_text(hint);
}

void UrbexHud::trigger_flash() {
	flash = 1.0f;
}

void UrbexHud::set_fade(float alpha) {
	fade_rect->set_color(Color(0, 0, 0, alpha));
}

void UrbexHud::show_center(const String &text, float seconds) {
	center_text->set_text(text);
	center_timer = seconds;
	center_text->set_modulate(Color(1, 1, 1, 1));
}

void UrbexHud::show_note(const String &title, const String &body) {
	note_title->set_text(title);
	note_body->set_text(body);
	note_body->scroll_to_line(0);
	note_panel->set_visible(true);
}

void UrbexHud::hide_note() {
	note_panel->set_visible(false);
}

bool UrbexHud::is_note_open() const {
	return note_panel && note_panel->is_visible();
}

void UrbexHud::set_gameplay_visible(bool visible) {
	root->set_visible(visible);
}

void UrbexHud::show_pause(bool visible) {
	pause_menu->set_visible(visible);
	if (visible) {
		pause_resume->grab_focus();
	}
}

bool UrbexHud::is_pause_visible() const {
	return pause_menu && pause_menu->is_visible();
}

void UrbexHud::show_result(const String &title, const Color &title_color, const String &body) {
	result_title->set_text(title);
	result_title->add_theme_color_override("font_color", title_color);
	result_body->set_text(body);
	result_screen->set_visible(true);
	viewfinder->set_visible(false);
	note_panel->set_visible(false);
	result_retry->grab_focus();
}

void UrbexHud::hide_result() {
	result_screen->set_visible(false);
}

void MainMenu::build() {
	if (level_list) {
		return;
	}
	set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	set_theme(make_ui_theme());

	ColorRect *bg = make_rect(this, Color(0.045f, 0.05f, 0.06f));
	anchor(bg, 0, 0, 1, 1, 0, 0, 0, 0);
	ColorRect *stripe = make_rect(this, Color(ACCENT.r, ACCENT.g, ACCENT.b, 0.85f));
	anchor(stripe, 0, 0, 0, 1, 0, 0, 6, 0);
	ColorRect *glow = make_rect(this, Color(0.12f, 0.08f, 0.05f, 0.5f));
	anchor(glow, 0.55f, 0, 1, 1, 0, 0, 0, 0);

	MarginContainer *margin = add<MarginContainer>(this);
	anchor(margin, 0, 0, 1, 1, 0, 0, 0, 0);
	for (const char *side : { "margin_left", "margin_right", "margin_top", "margin_bottom" }) {
		margin->add_theme_constant_override(side, 56);
	}
	HBoxContainer *columns = add<HBoxContainer>(margin);
	columns->add_theme_constant_override("separation", 48);

	VBoxContainer *left = add<VBoxContainer>(columns);
	left->set_custom_minimum_size(Vector2(470.0f, 0.0f));
	left->add_theme_constant_override("separation", 10);
	Label *title = make_label(left, "URBEX"_u, 92, ACCENT);
	title->add_theme_constant_override("outline_size", 0);
	Label *sub = make_label(left, "заброшки · бомбари · крыши · ЧОП"_u, 22, MUTED);
	sub->add_theme_constant_override("outline_size", 0);
	Control *spacer = add<Control>(left);
	spacer->set_custom_minimum_size(Vector2(0, 26));
	make_label(left, "ВЫБЕРИ ОБЪЕКТ"_u, 16, MUTED);

	level_list = add<VBoxContainer>(left);
	level_list->add_theme_constant_override("separation", 8);
	const std::vector<LevelInfo> &levels = level_catalog();
	for (size_t i = 0; i < levels.size(); i++) {
		Button *b = make_button(level_list, levels[i].title, 460);
		b->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		b->connect("pressed", callable_mp(this, &MainMenu::select_level).bind(int(i)));
		level_buttons.push_back(b);
	}

	Control *spacer2 = add<Control>(left);
	spacer2->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	HBoxContainer *sens_row = add<HBoxContainer>(left);
	sens_row->add_theme_constant_override("separation", 12);
	sens_label = make_label(sens_row, "Чувствительность мыши"_u, 16, MUTED);
	HSlider *slider = add<HSlider>(sens_row);
	slider->set_min(0.3);
	slider->set_max(2.5);
	slider->set_step(0.05);
	slider->set_value(UrbexGame::get_singleton()->get_sensitivity());
	slider->set_custom_minimum_size(Vector2(180.0f, 24.0f));
	slider->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
	slider->connect("value_changed", callable_mp(this, &MainMenu::on_sensitivity));

	HBoxContainer *bottom = add<HBoxContainer>(left);
	bottom->add_theme_constant_override("separation", 10);
	Button *memo = make_button(bottom, "Памятка сталкера"_u, 170);
	memo->connect("pressed", callable_mp(this, &MainMenu::show_info).bind(0));
	Button *controls = make_button(bottom, "Управление"_u, 140);
	controls->connect("pressed", callable_mp(this, &MainMenu::show_info).bind(1));
	Button *quit = make_button(bottom, "Выход"_u, 110);
	quit->connect("pressed", callable_mp(UrbexGame::get_singleton(), &UrbexGame::quit_game));

	VBoxContainer *right = add<VBoxContainer>(columns);
	right->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	right->add_theme_constant_override("separation", 16);
	PanelContainer *details_panel = add<PanelContainer>(right);
	details_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	details = add<RichTextLabel>(details_panel);
	details->set_use_bbcode(true);
	details->add_theme_font_size_override("normal_font_size", 18);
	details->add_theme_font_size_override("bold_font_size", 19);
	Button *start = make_button(right, "НАЧАТЬ ВЫЛАЗКУ"_u, 300);
	start->set_custom_minimum_size(Vector2(300.0f, 58.0f));
	start->add_theme_font_size_override("font_size", 24);
	start->connect("pressed", callable_mp(this, &MainMenu::start_selected));

	info_panel = add<Control>(this);
	anchor(info_panel, 0, 0, 1, 1, 0, 0, 0, 0);
	ColorRect *dim = make_rect(info_panel, Color(0, 0, 0, 0.75f));
	anchor(dim, 0, 0, 1, 1, 0, 0, 0, 0);
	dim->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	PanelContainer *ipanel = add<PanelContainer>(info_panel);
	anchor(ipanel, 0.5f, 0.5f, 0.5f, 0.5f, -480, -330, 480, 330);
	VBoxContainer *ibox = add<VBoxContainer>(ipanel);
	ibox->add_theme_constant_override("separation", 10);
	info_text = add<RichTextLabel>(ibox);
	info_text->set_use_bbcode(true);
	info_text->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	info_text->add_theme_font_size_override("normal_font_size", 17);
	info_text->add_theme_font_size_override("bold_font_size", 18);
	Button *close = make_button(ibox, "Закрыть"_u, 200);
	close->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
	close->connect("pressed", callable_mp(this, &MainMenu::hide_info));
	info_panel->set_visible(false);

	select_level(0);
}

void MainMenu::refresh() {
	select_level(selected);
	if (!level_buttons.empty()) {
		level_buttons[size_t(selected)]->grab_focus();
	}
}

void MainMenu::select_level(int index) {
	const std::vector<LevelInfo> &levels = level_catalog();
	if (index < 0 || index >= int(levels.size())) {
		return;
	}
	selected = index;
	const LevelInfo &info = levels[size_t(index)];
	String text;
	text += String("[font_size=30][color=#ee8533]") + info.title + "[/color][/font_size]\n";
	text += String("[color=#a0a49f]") + info.tagline + "[/color]\n\n";
	text += info.description + String("\n\n");
	text += String("[b]Сложность:[/b] "_u) + info.difficulty + "\n";
	int best = UrbexGame::get_singleton()->get_best_score(index);
	if (best > 0) {
		text += String("[b]Лучший результат:[/b] "_u) + String::num_int64(best) + " лайков в паблике"_u;
	} else {
		text += String("[color=#a0a49f]Ещё не пройдено[/color]"_u);
	}
	details->set_text(text);
	for (size_t i = 0; i < level_buttons.size(); i++) {
		level_buttons[i]->set_modulate(int(i) == index ? Color(1.0f, 0.85f, 0.65f) : Color(1, 1, 1));
	}
}

void MainMenu::start_selected() {
	UrbexGame::get_singleton()->start_level(selected);
}

void MainMenu::show_info(int page) {
	info_text->set_text(page == 0 ? stalker_memo_text() : controls_text());
	info_text->scroll_to_line(0);
	info_panel->set_visible(true);
}

void MainMenu::hide_info() {
	info_panel->set_visible(false);
}

void MainMenu::on_sensitivity(double value) {
	UrbexGame::get_singleton()->set_sensitivity(float(value));
}

} // namespace urbex
