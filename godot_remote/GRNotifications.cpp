/* GRNotifications.cpp */
#include "GRNotifications.h"
#include "GRResources.h"
#include "GRUtils.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY

#define add_s_override add_style_override
#define set_b_icon set_icon

#include "core/engine.h"
#include "core/os/input_event.h"
#include "scene/animation/tween.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/texture_rect.h"
#else

#define add_s_override add_stylebox_override
#define set_b_icon set_button_icon

#include <BoxContainer.hpp>
#include <Engine.hpp>
#include <InputEvent.hpp>
#include <InputEventAction.hpp>
#include <InputEventGesture.hpp>
#include <InputEventJoypadButton.hpp>
#include <InputEventJoypadMotion.hpp>
#include <InputEventKey.hpp>
#include <InputEventMIDI.hpp>
#include <InputEventMagnifyGesture.hpp>
#include <InputEventMouse.hpp>
#include <InputEventMouseButton.hpp>
#include <InputEventMouseMotion.hpp>
#include <InputEventPanGesture.hpp>
#include <InputEventScreenDrag.hpp>
#include <InputEventScreenTouch.hpp>
#include <InputEventWithModifiers.hpp>
using namespace godot;
#endif

using namespace GRUtils;

GRNotifications *GRNotifications::singleton = nullptr;
GRNotificationPanelSTATIC_DATA *GRNotificationPanel::_default_data = nullptr;

std::vector<GRNotificationPanel *> GRNotifications::_get_notifications_with_title(String title) { // GRNotificationPanel *
	std::vector<GRNotificationPanel *> res; // GRNotificationPanel *

	if (singleton) {
		for (int i = (int)singleton->notifications.size() - 1; i >= 0; i--) {
			if (((GRNotificationPanel *)notifications[i])->get_title() == title) {
				res.push_back(notifications[i]);
			}
		}
	}
	return res;
}

GRNotificationPanel *GRNotifications::_get_notification(String title) {
	for (int i = (int)singleton->notifications.size() - 1; i >= 0; i--) {
		if (((GRNotificationPanel *)notifications[i])->get_title() == title) {
			return notifications[i];
		}
	}
	return nullptr;
}

void GRNotifications::_set_notifications_position(ENUM_ARG(NotificationsPosition) positon) {
	NotificationsPosition pos = (NotificationsPosition)positon;
	if (notif_list_node) {
		switch (pos) {
			case NotificationsPosition::TOP_LEFT:
			case NotificationsPosition::TOP_CENTER:
			case NotificationsPosition::TOP_RIGHT:
				notif_list_node->set_v_grow_direction(Control::GROW_DIRECTION_END);
				notif_list_node->set_alignment(BoxContainer::ALIGN_BEGIN);
				break;
			case NotificationsPosition::BOTTOM_LEFT:
			case NotificationsPosition::BOTTOM_CENTER:
			case NotificationsPosition::BOTTOM_RIGHT:
				notif_list_node->set_v_grow_direction(Control::GROW_DIRECTION_BEGIN);
				notif_list_node->set_alignment(BoxContainer::ALIGN_END);
				break;
		}
	}

	_set_all_notifications_positions(pos);
	notifications_position = pos;
}

void GRNotifications::_add_notification_or_append_string(String title, String text, ENUM_ARG(NotificationIcon) icon, bool new_string, float duration_multiplier) {
	if (!notifications_enabled)
		return;

	auto *np = _get_notification(title);
	if (np) {
		np->update_text(np->get_text() + (new_string ? "\n" + text : text));
	} else {
		_add_notification(title, text, icon, false, duration_multiplier);
	}
}

void GRNotifications::_add_notification_or_update_line(String title, String id, String text, ENUM_ARG(NotificationIcon) icon, float duration_multiplier) {
	if (!notifications_enabled)
		return;

	auto *np = cast_to<GRNotificationPanelUpdatable>(_get_notification(title));
	if (np) {
		_log("Updating existing updatable notification with Title: \"" + title + "\" ID: \"" + id + "\" Text:\"" + text + "\"", LogLevel::LL_DEBUG);
		np->set_updatable_line(this, title, id, text, (NotificationIcon)icon, duration_multiplier, style);
	} else {

		np = memnew(GRNotificationPanelUpdatable);
		notif_list_node->add_child(np);
		if (notifications_position <= NotificationsPosition::TOP_RIGHT)
			notif_list_node->move_child(np, 0);
		notifications.push_back(np);

		_log("New updatable notification added with Title: \"" + title + "\"" + " and Text:\"" + text + "\"", LogLevel::LL_DEBUG);
		emit_signal("notification_added", title, text);

		np->set_updatable_line(this, title, id, text, (NotificationIcon)icon, duration_multiplier, style);

		// FORCE UPDATE SIZE OF CONTEINER
		notif_list_node->call("_size_changed");
	}
}

void GRNotifications::_set_all_notifications_positions(NotificationsPosition pos) {
	for (int i = (int)singleton->notifications.size() - 1; i >= 0; i--) {
		GRNotificationPanel *np = notifications[i];
		if (np && !np->is_queued_for_deletion())
			np->set_notification_position((NotificationsPosition)pos);
	}
}

void GRNotifications::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
		case NOTIFICATION_EXIT_TREE: {
			GRNotificationPanel::clear_styles();
			break;
		}
	}
}

#ifndef GDNATIVE_LIBRARY

void GRNotifications::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_set_notifications_position", "pos"), &GRNotifications::_set_notifications_position);
	ClassDB::bind_method(D_METHOD("_add_notification_or_append_string", "title", "text", "icon", "new_string", "duration_multiplier"), &GRNotifications::_add_notification_or_append_string);
	ClassDB::bind_method(D_METHOD("_add_notification_or_update_line", "title", "id", "text", "icon", "duration_multiplier"), &GRNotifications::_add_notification_or_update_line);
	ClassDB::bind_method(D_METHOD("_add_notification", "title", "text", "icon", "update_existing", "duration_multiplier"), &GRNotifications::_add_notification);
	ClassDB::bind_method(D_METHOD("_remove_notification", "title", "is_all_entries"), &GRNotifications::_remove_notification);
	ClassDB::bind_method(D_METHOD("_remove_exact_notification", "notification"), &GRNotifications::_remove_exact_notification);
	ClassDB::bind_method(D_METHOD("_clear_notifications"), &GRNotifications::_clear_notifications);

	ClassDB::bind_method(D_METHOD("_remove_list"), &GRNotifications::_remove_list);

	ADD_SIGNAL(MethodInfo("notifications_toggled", PropertyInfo(Variant::BOOL, "is_enabled")));
	ADD_SIGNAL(MethodInfo("notifications_cleared"));
	ADD_SIGNAL(MethodInfo("notification_added", PropertyInfo(Variant::STRING, "title"), PropertyInfo(Variant::STRING, "text")));
	ADD_SIGNAL(MethodInfo("notification_removed", PropertyInfo(Variant::STRING, "title"), PropertyInfo(Variant::BOOL, "is_cleared")));

	BIND_ENUM_CONSTANT(ICON_NONE);
	BIND_ENUM_CONSTANT(ICON_ERROR);
	BIND_ENUM_CONSTANT(ICON_WARNING);
	BIND_ENUM_CONSTANT(ICON_SUCCESS);
	BIND_ENUM_CONSTANT(ICON_FAIL);

	BIND_ENUM_CONSTANT(TOP_LEFT);
	BIND_ENUM_CONSTANT(TOP_CENTER);
	BIND_ENUM_CONSTANT(TOP_RIGHT);
	BIND_ENUM_CONSTANT(BOTTOM_LEFT);
	BIND_ENUM_CONSTANT(BOTTOM_CENTER);
	BIND_ENUM_CONSTANT(BOTTOM_RIGHT);
}

#else

void GRNotifications::_register_methods() {
	METHOD_REG(GRNotifications, _notification);

	METHOD_REG(GRNotifications, _set_notifications_position);
	METHOD_REG(GRNotifications, _add_notification_or_append_string);
	METHOD_REG(GRNotifications, _add_notification_or_update_line);
	METHOD_REG(GRNotifications, _add_notification);
	METHOD_REG(GRNotifications, _remove_notification);
	METHOD_REG(GRNotifications, _remove_exact_notification);
	METHOD_REG(GRNotifications, _clear_notifications);

	METHOD_REG(GRNotifications, _remove_list);

	register_signal<GRNotifications>("notifications_toggled", "is_enabled", GODOT_VARIANT_TYPE_BOOL);
	register_signal<GRNotifications>("notifications_cleared", Dictionary::make());
	register_signal<GRNotifications>("notification_added", "title", GODOT_VARIANT_TYPE_STRING, "text", GODOT_VARIANT_TYPE_STRING);
	register_signal<GRNotifications>("notification_removed", "title", GODOT_VARIANT_TYPE_STRING, "is_cleared", GODOT_VARIANT_TYPE_BOOL);
}

#endif

GRNotificationPanel *GRNotifications::get_notification(String title) {
	if (singleton) {
		return singleton->_get_notification(title);
	}
	return nullptr;
}

Array GRNotifications::get_all_notifications() {
	Array arr;
	if (singleton) {
		for (int i = 0; i < singleton->notifications.size(); i++) {
			arr.append(singleton->notifications[i]);
		}
	}
	return arr;
}

Array GRNotifications::get_notifications_with_title(String title) {
	Array arr;
	if (singleton) {
		auto list = singleton->_get_notifications_with_title(title);
		for (int i = 0; i < list.size(); i++) {
			arr.append(list[i]);
		}
	}
	return arr;
}

GRNotifications::NotificationsPosition GRNotifications::get_notifications_position() {
	if (singleton) {
		return singleton->notifications_position;
	}
	return NotificationsPosition::TOP_CENTER;
}

void GRNotifications::set_notifications_position(NotificationsPosition positon) {
	if (singleton) {
		singleton->call_deferred("_set_notifications_position", positon);
	}
}

bool GRNotifications::get_notifications_enabled() {
	if (singleton)
		return singleton->notifications_enabled;
	return false;
}

void GRNotifications::set_notifications_enabled(bool _enabled) {
	if (singleton) {
		singleton->notifications_enabled = _enabled;
		singleton->emit_signal("notifications_toggled", _enabled);
		if (!_enabled) {
			clear_notifications();
		}
	}
}

float GRNotifications::get_notifications_duration() {
	if (singleton)
		return singleton->notifications_duration;
	return 0.f;
}

void GRNotifications::set_notifications_duration(float _duration) {
	if (singleton)
		singleton->notifications_duration = _duration;
}

Ref<GRNotificationStyle> GRNotifications::get_notifications_style() {
	if (singleton) {
		if (singleton->style.is_valid())
			return singleton->style;
		return GRNotificationPanel::generate_default_style();
	}
	return Ref<GRNotificationStyle>();
}

void GRNotifications::set_notifications_style(Ref<GRNotificationStyle> _style) {
	if (singleton) {
		singleton->style = _style;
	}
}

void GRNotifications::add_notification_or_append_string(String title, String text, NotificationIcon icon, bool new_string, float duration_multiplier) {
	if (singleton) {
		singleton->call_deferred("_add_notification_or_append_string", title, text, icon, new_string, duration_multiplier);
	}
}

void GRNotifications::add_notification_or_update_line(String title, String id, String text, NotificationIcon icon, float duration_multiplier) {
	if (singleton) {
		singleton->call_deferred("_add_notification_or_update_line", title, id, text, icon, duration_multiplier);
	}
}

void GRNotifications::add_notification(String title, String text, NotificationIcon icon, bool update_existing, float duration_multiplier) {
	if (singleton) {
		singleton->call_deferred("_add_notification", title, text, icon, update_existing, duration_multiplier);
	}
}

void GRNotifications::remove_notification(String title, bool all_entries) {
	if (singleton) {
		singleton->call_deferred("_remove_notification", title, all_entries);
	}
}

void GRNotifications::remove_notification_exact(Node *_notif) {
	if (singleton) {
		singleton->call_deferred("_remove_exact_notification", _notif);
	}
}

void GRNotifications::clear_notifications() {
	if (singleton) {
		singleton->call_deferred("_clear_notifications");
	}
}

void GRNotifications::_add_notification(String title, String text, ENUM_ARG(NotificationIcon) icon, bool update_existing, float duration_multiplier) {
	if (!notifications_enabled)
		return;

	if (notif_list_node && !notif_list_node->is_queued_for_deletion()) {
		GRNotificationPanel *np;
		if (update_existing) {
			np = _get_notification(title);
			if (np) {
				_log("Updating existing notification with Title: \"" + title + "\"" + " and Text:\"" + text + "\"", LogLevel::LL_DEBUG);
				if (notifications_position <= NotificationsPosition::TOP_RIGHT) {
					notif_list_node->move_child(np, 0);
				} else {
					notif_list_node->move_child(np, notif_list_node->get_child_count() - 1);
				}
				goto set_new_data;
			}
		}

		np = memnew(GRNotificationPanel);
		notif_list_node->add_child(np);
		if (notifications_position <= NotificationsPosition::TOP_RIGHT)
			notif_list_node->move_child(np, 0);
		notifications.push_back(np);

		_log("New notification added with Title: \"" + title + "\"" + " and Text:\"" + text + "\"", LogLevel::LL_DEBUG);
		emit_signal("notification_added", title, text);

	set_new_data:

		np->set_data(this, title, text, (NotificationIcon)icon, duration_multiplier, style);

		// FORCE UPDATE SIZE OF CONTEINER
		notif_list_node->call("_size_changed");
	}
}

void GRNotifications::_remove_notification(String title, bool all_entries) {
	if (all_entries) {
		auto nps = _get_notifications_with_title(title);
		for (int i = 0; i < nps.size(); i++) {
			_remove_exact_notification(nps[i]);
		}
	} else {
		_remove_exact_notification(_get_notification(title));
	}
}

void GRNotifications::_remove_exact_notification(Node *_notif) {
	GRNotificationPanel *np = cast_to<GRNotificationPanel>(_notif);
	if (np) {
		emit_signal("notification_removed", np->get_text(), clearing_notifications);

		notif_list_node->remove_child(np);
		vec_remove_idx(notifications, np);
		memdelete(np);

		// FORCE UPDATE SIZE OF CONTEINER
		notif_list_node->call("_size_changed");
	}
}

void GRNotifications::_clear_notifications() {
	clearing_notifications = true;
	for (int i = 0; i < notifications.size(); i++) {
		GRNotificationPanel *tmp = notifications[i];
		notif_list_node->remove_child(tmp);
		vec_remove_idx(notifications, tmp);
		memdelete(tmp);
	}
	emit_signal("notifications_cleared");
	clearing_notifications = false;
}

GRNotifications *GRNotifications::get_singleton() {
	return singleton;
}

void GRNotifications::_init() {
	set_name("Notifications");

	LEAVE_IF_EDITOR();

	if (!singleton)
		singleton = this;

	GRNotificationPanel::_default_data = new GRNotificationPanelSTATIC_DATA();

	notifications_enabled = GET_PS(GodotRemote::ps_notifications_enabled_name);
	notifications_position = (NotificationsPosition)(int)GET_PS(GodotRemote::ps_noticications_position_name);
	notifications_duration = GET_PS(GodotRemote::ps_notifications_duration_name);

	notif_list_node = memnew(VBoxContainer);
	notif_list_node->set_name("NotificationList");
	call_deferred("add_child", notif_list_node); // add new function to check ST() and then add_child to prevent leak when generation mono glue

	notif_list_node->set_anchor_and_margin(MARGIN_LEFT, 0.f, 8.f);
	notif_list_node->set_anchor_and_margin(MARGIN_RIGHT, 1.f, -8.f);
	notif_list_node->set_anchor_and_margin(MARGIN_TOP, 0.f, 8.f);
	notif_list_node->set_anchor_and_margin(MARGIN_BOTTOM, 1.f, -8.f);
	notif_list_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	notif_list_node->set_mouse_filter(Control::MouseFilter::MOUSE_FILTER_IGNORE);

	set_notifications_position(notifications_position);
}

void GRNotifications::_deinit() {
	LEAVE_IF_EDITOR();

	if (this == singleton)
		singleton = nullptr;
	call_deferred("_remove_list");

	notifications.clear();
	GRNotificationPanel::_default_data->_default_close_texture.unref();
	GRNotificationPanel::_default_data->_default_style.unref();
	GRNotificationPanel::_default_data->_default_textures.clear();
	delete GRNotificationPanel::_default_data;
	GRNotificationPanel::_default_data = nullptr;

	style.unref();
}

void GRNotifications::_remove_list() {
	_clear_notifications();
	remove_child(notif_list_node);
	memdelete(notif_list_node);
	notif_list_node = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// NOTIFICATION PANEL
//////////////////////////////////////////////////////////////////////////

void GRNotificationPanel::_panel_hovered() {
	tween_node->stop_all();
	tween_node->reset_all();
	is_hovered = true;
}

void GRNotificationPanel::_panel_lose_hover() {
	_setup_tween(tween_node);
	tween_node->start();
	is_hovered = false;
}

void GRNotificationPanel::_remove_this_notification() {
	if (owner && !owner->is_queued_for_deletion()) {
		owner->remove_notification_exact(this);
	}
}

void GRNotificationPanel::_setup_tween(Tween *_tween) {
	const float duration = owner->get_notifications_duration() * duration_mul;
	_tween->remove_all();
	_tween->interpolate_property(this, NodePath("modulate"), Color(1, 1, 1, 1), Color(1, 1, 1, 0), duration, Tween::TRANS_QUART, Tween::EASE_IN);
	_tween->interpolate_callback(this, duration, "_remove_this_notification", Variant(), Variant(), Variant(), Variant(), Variant());
}

void GRNotificationPanel::_update_style() {
	Ref<GRNotificationStyle> s = _default_data->_default_style;
	if (style.is_valid())
		s = style;

	add_s_override("panel", s->get_panel_style());

	title_node->add_font_override("font", s->get_title_font());
	text_node->add_font_override("font", s->get_text_font());

	close_node->set_theme(s->get_close_button_theme());
	close_node->add_font_override("font", s->get_title_font());
	close_node->set_text(s->get_close_button_icon().is_null() ? "[x]" : "");
	close_node->set_b_icon(s->get_close_button_icon());

	icon_tex_node->set_texture(s->get_notification_icon(notification_icon));
	icon_tex_node->set_visible(icon_tex_node->get_texture().is_valid());
}

Ref<GRNotificationStyle> GRNotificationPanel::generate_default_style() {
	Ref<GRNotificationStyle> res_style(memnew(GRNotificationStyle));

	Ref<StyleBoxFlat> panel(memnew(StyleBoxFlat));
	panel->set_bg_color(Color(0.172549f, 0.188235f, 0.305882f, 0.300588f));
	panel->set_corner_detail(1);
	panel->set_border_width_all(1);
	panel->set_border_color(Color(0.482353f, 0.47451f, 0.52549f, 0.686275f));
	panel->set_shadow_color(Color(0, 0, 0, 0.254902f));
	panel->set_shadow_size(2);
	panel->set_default_margin(MARGIN_BOTTOM, 4);
	panel->set_default_margin(MARGIN_LEFT, 4);
	panel->set_default_margin(MARGIN_RIGHT, 4);
	panel->set_default_margin(MARGIN_TOP, 5);
	res_style->set_panel_style(panel);

	Ref<StyleBoxEmpty> btn_nrm(memnew(StyleBoxEmpty));
	btn_nrm->set_default_margin(MARGIN_BOTTOM, 1);
	btn_nrm->set_default_margin(MARGIN_LEFT, 1);
	btn_nrm->set_default_margin(MARGIN_RIGHT, 1);
	btn_nrm->set_default_margin(MARGIN_TOP, 1);

	Ref<StyleBoxEmpty> btn_prsd(memnew(StyleBoxEmpty));
	btn_prsd->set_default_margin(MARGIN_BOTTOM, 0);
	btn_prsd->set_default_margin(MARGIN_LEFT, 1);
	btn_prsd->set_default_margin(MARGIN_RIGHT, 1);
	btn_prsd->set_default_margin(MARGIN_TOP, 2);

	Ref<Theme> close_btn_theme(memnew(Theme));
	close_btn_theme->set_stylebox("hover", "Button", btn_nrm);
	close_btn_theme->set_stylebox("normal", "Button", btn_nrm);
	close_btn_theme->set_stylebox("pressed", "Button", btn_prsd);
	res_style->set_close_button_theme(close_btn_theme);

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
#define SetIcon(_i) res_style->set_notification_icon(_i, _default_data->_default_textures[(int)_i])

	if (_default_data->_default_textures.empty() || _default_data->_default_close_texture.is_null())
		_load_default_textures();

	res_style->set_close_button_icon(_default_data->_default_close_texture);
	SetIcon(GRNotifications::NotificationIcon::ICON_ERROR);
	SetIcon(GRNotifications::NotificationIcon::ICON_WARNING);
	SetIcon(GRNotifications::NotificationIcon::ICON_SUCCESS);
	SetIcon(GRNotifications::NotificationIcon::ICON_FAIL);

#undef SetIcon
#endif

	return res_style;
}

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
void GRNotificationPanel::_load_default_textures() {

#define LoadTex(_i, _n)                                  \
	{                                                    \
		img.instance();                                  \
		GetPoolVectorFromBin(tmp_arr, _n);               \
		img->load_png_from_buffer(tmp_arr);              \
                                                         \
		tex.instance();                                  \
		tex->create_from_image(img);                     \
		_default_data->_default_textures[(int)_i] = tex; \
	}

	Ref<Image> img;
	Ref<ImageTexture> tex;

	LoadTex(GRNotifications::NotificationIcon::ICON_ERROR, GRResources::Bin_ErrorIconPNG);
	LoadTex(GRNotifications::NotificationIcon::ICON_WARNING, GRResources::Bin_WarningIconPNG);
	LoadTex(GRNotifications::NotificationIcon::ICON_SUCCESS, GRResources::Bin_ConnectedIconPNG);
	LoadTex(GRNotifications::NotificationIcon::ICON_FAIL, GRResources::Bin_DisconnectedIconPNG);

	{
		img.instance();
		GetPoolVectorFromBin(tmp_arr, GRResources::Bin_CloseIconPNG);
		img->load_png_from_buffer(tmp_arr);

		tex.instance();
		tex->create_from_image(img);
		_default_data->_default_close_texture = tex;
	}

#undef LoadTex
}
#endif

#ifndef GDNATIVE_LIBRARY

void GRNotificationPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_remove_this_notification"), &GRNotificationPanel::_remove_this_notification);
	ClassDB::bind_method(D_METHOD("_panel_hovered"), &GRNotificationPanel::_panel_hovered);
	ClassDB::bind_method(D_METHOD("_panel_lose_hover"), &GRNotificationPanel::_panel_lose_hover);

	ClassDB::bind_method(D_METHOD("get_title"), &GRNotificationPanel::get_title);
	ClassDB::bind_method(D_METHOD("get_text"), &GRNotificationPanel::get_text);
	ClassDB::bind_method(D_METHOD("update_text", "text"), &GRNotificationPanel::update_text);
}

#else

void GRNotificationPanel::_register_methods() {
	METHOD_REG(GRNotificationPanel, _notification);

	METHOD_REG(GRNotificationPanel, _remove_this_notification);
	METHOD_REG(GRNotificationPanel, _panel_hovered);
	METHOD_REG(GRNotificationPanel, _panel_lose_hover);

	METHOD_REG(GRNotificationPanel, get_title);
	METHOD_REG(GRNotificationPanel, get_text);
	METHOD_REG(GRNotificationPanel, update_text);
}

#endif

void GRNotificationPanel::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
	}
}

void GRNotificationPanel::clear_styles() {
	if (_default_data) {
		_default_data->_default_style.unref();
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
		_default_data->_default_close_texture.unref();
		_default_data->_default_textures.clear();
#endif
	}
}

void GRNotificationPanel::set_notification_position(GRNotifications::NotificationsPosition position) {
	switch (position) {
		case GRNotifications::NotificationsPosition::TOP_LEFT:
		case GRNotifications::NotificationsPosition::BOTTOM_LEFT:
			set_h_size_flags(0);
			break;
		case GRNotifications::NotificationsPosition::TOP_RIGHT:
		case GRNotifications::NotificationsPosition::BOTTOM_RIGHT:
			set_h_size_flags(SIZE_SHRINK_END);
			break;
		case GRNotifications::NotificationsPosition::TOP_CENTER:
		case GRNotifications::NotificationsPosition::BOTTOM_CENTER:
			set_h_size_flags(SIZE_SHRINK_CENTER);
			break;
	}
}

void GRNotificationPanel::set_data(GRNotifications *_owner, String title, String text, GRNotifications::NotificationIcon icon, float duration_multiplier, Ref<GRNotificationStyle> _style) {
	owner = _owner;
	notification_icon = icon;
	title_node->set_text(title);
	text_node->set_text(text);
	duration_mul = duration_multiplier;
	style = _style;

	_update_style();
	_setup_tween(tween_node);
	tween_node->start();

	if (owner) {
		set_notification_position((GRNotifications::NotificationsPosition)owner->get_notifications_position());
	}
}

String GRNotificationPanel::get_title() {
	return title_node->get_text();
}

String GRNotificationPanel::get_text() {
	return text_node->get_text();
}

void GRNotificationPanel::update_text(String text) {
	text_node->set_text(text);
	_setup_tween(tween_node);

	if (!is_hovered)
		tween_node->start();
}

void GRNotificationPanel::_init() {
	set_name("NotificationPanel");

	LEAVE_IF_EDITOR();

	if (_default_data && _default_data->_default_style.is_null())
		_default_data->_default_style = generate_default_style();

	set_mouse_filter(Control::MouseFilter::MOUSE_FILTER_PASS);
	connect("mouse_entered", this, "_panel_hovered");
	connect("mouse_exited", this, "_panel_lose_hover");

	vbox_node = memnew(VBoxContainer);
	hbox_node = memnew(HBoxContainer);
	icon_tex_node = memnew(TextureRect);
	title_node = memnew(Label);
	text_node = memnew(Label);
	close_node = memnew(Button);
	tween_node = memnew(Tween);

	add_child(vbox_node);
	add_child(tween_node);
	vbox_node->add_child(hbox_node);
	vbox_node->add_child(text_node);
	hbox_node->add_child(icon_tex_node);
	hbox_node->add_child(title_node);
	hbox_node->add_child(close_node);

	vbox_node->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	vbox_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	vbox_node->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);

	hbox_node->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	hbox_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	hbox_node->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);

	icon_tex_node->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	icon_tex_node->set_h_size_flags(SIZE_FILL);
	icon_tex_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	icon_tex_node->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

	title_node->set_h_size_flags(SIZE_EXPAND_FILL);
	title_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	title_node->set_v_size_flags(SIZE_FILL);
	title_node->set_align(Label::ALIGN_CENTER);
	title_node->set_valign(Label::VALIGN_CENTER);
	title_node->set_text("UNTITLED");

	text_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	text_node->set_custom_minimum_size(Size2(128, 0));
	text_node->set_align(Label::ALIGN_CENTER);
	text_node->set_text("EMPTY TEXT");

	close_node->set_focus_mode(Control::FOCUS_NONE);
	close_node->connect("pressed", this, "_remove_this_notification");

	//_setup_tween(tween_node);
	//_update_style();
}

void GRNotificationPanel::_deinit() {
	LEAVE_IF_EDITOR();

	if (style.is_valid())
		style.unref();
}

//////////////////////////////////////////////////////////////////////////
// UPDATABLE NOTIFICATION PANEL
//////////////////////////////////////////////////////////////////////////

void GRNotificationPanelUpdatable::_init() {
	LEAVE_IF_EDITOR();
#ifndef GDNATIVE_LIBRARY
#else
	GRNotificationPanel::_init();
#endif
}

void GRNotificationPanelUpdatable::_deinit() {
	LEAVE_IF_EDITOR();
}

String GRNotificationPanelUpdatable::_get_text_from_lines() {
	String res = "";
	int i = 0;
	for (auto lv : lines) {
		res += str(lv.second);
		if (i < lines.size() - 1) {
			res += "\n";
		}
		i++;
	}
	return res;
}

#ifndef GDNATIVE_LIBRARY
void GRNotificationPanelUpdatable::_bind_methods() {
	ClassDB::bind_method(D_METHOD("clear_lines"), &GRNotificationPanelUpdatable::clear_lines);
	ClassDB::bind_method(D_METHOD("remove_updatable_line", "id"), &GRNotificationPanelUpdatable::remove_updatable_line);
}

#else

void GRNotificationPanelUpdatable::_register_methods() {
	METHOD_REG(GRNotificationPanelUpdatable, _notification);

	METHOD_REG(GRNotificationPanelUpdatable, clear_lines);
	METHOD_REG(GRNotificationPanelUpdatable, remove_updatable_line);
}

#endif

void GRNotificationPanelUpdatable::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			GRNotificationPanel::_deinit();
			break;
	}
}

void GRNotificationPanelUpdatable::set_updatable_line(GRNotifications *_owner, String title, String id, String text, GRNotifications::NotificationIcon icon, float duration_multiplier, Ref<GRNotificationStyle> _style) {
	if (configured) {
		lines[id] = text;
		text_node->set_text(_get_text_from_lines());

		_setup_tween(tween_node);
		if (!is_hovered) {
			tween_node->start();
		}
	} else {
		owner = _owner;
		notification_icon = icon;
		lines[id] = text;
		title_node->set_text(title);
		text_node->set_text(_get_text_from_lines());
		duration_mul = duration_multiplier;
		style = _style;

		_update_style();
		_setup_tween(tween_node);
		tween_node->start();

		if (owner) {
			set_notification_position((GRNotifications::NotificationsPosition)owner->get_notifications_position());
		}
	}
}

void GRNotificationPanelUpdatable::remove_updatable_line(String id) {
	if (lines.find(id) != lines.end()) {
		lines.erase(id);
		text_node->set_text(_get_text_from_lines());

		_setup_tween(tween_node);
		if (!is_hovered) {
			tween_node->start();
		}
	}
}

void GRNotificationPanelUpdatable::clear_lines() {
	lines.clear();
	text_node->set_text(_get_text_from_lines());

	_setup_tween(tween_node);
	if (!is_hovered) {
		tween_node->start();
	}
}

// STYLE REF

#ifndef GDNATIVE_LIBRARY
void GRNotificationStyle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_notification_icon", "notification_icon", "icon_texture"), &GRNotificationStyle::set_notification_icon);
	ClassDB::bind_method(D_METHOD("get_notification_icon", "notification_icon"), &GRNotificationStyle::get_notification_icon);

	ClassDB::bind_method(D_METHOD("set_panel_style", "style"), &GRNotificationStyle::set_panel_style);
	ClassDB::bind_method(D_METHOD("set_close_button_theme", "theme"), &GRNotificationStyle::set_close_button_theme);
	ClassDB::bind_method(D_METHOD("set_close_button_icon", "icon"), &GRNotificationStyle::set_close_button_icon);
	ClassDB::bind_method(D_METHOD("set_title_font", "font"), &GRNotificationStyle::set_title_font);
	ClassDB::bind_method(D_METHOD("set_text_font", "font"), &GRNotificationStyle::set_text_font);

	ClassDB::bind_method(D_METHOD("get_panel_style"), &GRNotificationStyle::get_panel_style);
	ClassDB::bind_method(D_METHOD("get_close_button_theme"), &GRNotificationStyle::get_close_button_theme);
	ClassDB::bind_method(D_METHOD("get_close_button_icon"), &GRNotificationStyle::get_close_button_icon);
	ClassDB::bind_method(D_METHOD("get_title_font"), &GRNotificationStyle::get_title_font);
	ClassDB::bind_method(D_METHOD("get_text_font"), &GRNotificationStyle::get_text_font);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "panel_style"), "set_panel_style", "get_panel_style");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "close_button_theme"), "set_close_button_theme", "get_close_button_theme");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "close_button_icon"), "set_close_button_icon", "get_close_button_icon");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "title_font"), "set_title_font", "get_title_font");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "text_font"), "set_text_font", "get_text_font");
}

#else

void GRNotificationStyle::_register_methods() {
	METHOD_REG(GRNotificationStyle, _notification);

	METHOD_REG(GRNotificationStyle, set_notification_icon);
	METHOD_REG(GRNotificationStyle, get_notification_icon);

	METHOD_REG(GRNotificationStyle, set_panel_style);
	METHOD_REG(GRNotificationStyle, set_close_button_theme);
	METHOD_REG(GRNotificationStyle, set_close_button_icon);
	METHOD_REG(GRNotificationStyle, set_title_font);
	METHOD_REG(GRNotificationStyle, set_text_font);

	METHOD_REG(GRNotificationStyle, get_panel_style);
	METHOD_REG(GRNotificationStyle, get_close_button_theme);
	METHOD_REG(GRNotificationStyle, get_close_button_icon);
	METHOD_REG(GRNotificationStyle, get_title_font);
	METHOD_REG(GRNotificationStyle, get_text_font);

	register_property<GRNotificationStyle, Ref<StyleBox> >("panel_style", &GRNotificationStyle::set_panel_style, &GRNotificationStyle::get_panel_style, nullptr);
	register_property<GRNotificationStyle, Ref<Theme> >("close_button_theme", &GRNotificationStyle::set_close_button_theme, &GRNotificationStyle::get_close_button_theme, nullptr);
	register_property<GRNotificationStyle, Ref<Texture> >("close_button_icon", &GRNotificationStyle::set_close_button_icon, &GRNotificationStyle::get_close_button_icon, nullptr);
	register_property<GRNotificationStyle, Ref<Font> >("title_font", &GRNotificationStyle::set_title_font, &GRNotificationStyle::get_title_font, nullptr);
	register_property<GRNotificationStyle, Ref<Font> >("text_font", &GRNotificationStyle::set_text_font, &GRNotificationStyle::get_text_font, nullptr);
}

#endif

void GRNotificationStyle::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
	}
}

void GRNotificationStyle::_init() {
	LEAVE_IF_EDITOR();

	for (int i = 0; i < GRNotifications::NotificationIcon::ICON_MAX; i++) {
		n_icons[i] = Ref<Texture>();
	}
}

void GRNotificationStyle::_deinit() {
	LEAVE_IF_EDITOR();

	panel_style.unref();
	close_button_theme.unref();
	close_button_icon.unref();
	title_font.unref();
	text_font.unref();

	for (int i = 0; i < GRNotifications::NotificationIcon::ICON_MAX; i++) {
		n_icons[i].unref();
	}
}

void GRNotificationStyle::set_panel_style(Ref<StyleBox> style) {
	panel_style = style;
}

Ref<StyleBox> GRNotificationStyle::get_panel_style() {
	return panel_style;
}

void GRNotificationStyle::set_close_button_theme(Ref<Theme> theme) {
	close_button_theme = theme;
}

Ref<Theme> GRNotificationStyle::get_close_button_theme() {
	return close_button_theme;
}

void GRNotificationStyle::set_close_button_icon(Ref<Texture> icon) {
	close_button_icon = icon;
}

Ref<Texture> GRNotificationStyle::get_close_button_icon() {
	return close_button_icon;
}

void GRNotificationStyle::set_title_font(Ref<Font> font) {
	title_font = font;
}

Ref<Font> GRNotificationStyle::get_title_font() {
	return title_font;
}

void GRNotificationStyle::set_text_font(Ref<Font> font) {
	text_font = font;
}

Ref<Font> GRNotificationStyle::get_text_font() {
	return text_font;
}

void GRNotificationStyle::set_notification_icon(ENUM_ARG(GRNotifications::NotificationIcon) notification_icon, Ref<Texture> icon_texture) {
	ERR_FAIL_INDEX(notification_icon, (int)GRNotifications::NotificationIcon::ICON_MAX);
	n_icons[notification_icon] = icon_texture;
}

Ref<Texture> GRNotificationStyle::get_notification_icon(ENUM_ARG(GRNotifications::NotificationIcon) notification_icon) {
	return n_icons[notification_icon];
}
