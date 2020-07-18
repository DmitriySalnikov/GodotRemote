/* GRNotifications.cpp */
#include "GRNotifications.h"
#include "GRUtils.h"
#include "GodotRemote.h"
#include "core/engine.h"
#include "scene/animation/tween.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"

using namespace GRUtils;

GRNotifications *GRNotifications::singleton = nullptr;

Ref<GRNotificationStyle> GRNotificationPanel::_default_style;

Array GRNotifications::get_all_notifications() {
	Array arr;
	if (singleton) {
		for (int i = 0; i < singleton->notifications.size(); i++) {
			arr.append(singleton->notifications[i]);
		}
	}
	return arr;
}

List<GRNotificationPanel *> GRNotifications::get_all_notifications_with_title(String title) {
	List<GRNotificationPanel *> res;

	if (singleton) {
		for (int i = singleton->notifications.size() - 1; i >= 0; i--) {
			if (notifications[i]->get_title() == title) {
				res.push_back(notifications[i]);
			}
		}
	}
	return res;
}

GRNotificationPanel *GRNotifications::get_notification(String title) {
	for (int i = singleton->notifications.size() - 1; i >= 0; i--) {
		if (notifications[i]->get_title() == title) {
			return notifications[i];
		}
	}
	return nullptr;
}

void GRNotifications::_set_notifications_position(int positon) {
	NotificationsPosition pos = (NotificationsPosition)positon;
	if (notif_list_node) {
		switch (pos) {
			case GRNotifications::NotificationsPosition::TL:
			case GRNotifications::NotificationsPosition::TC:
			case GRNotifications::NotificationsPosition::TR:
				notif_list_node->set_v_grow_direction(Control::GROW_DIRECTION_END);
				notif_list_node->set_alignment(BoxContainer::ALIGN_BEGIN);
				break;
			case GRNotifications::NotificationsPosition::BL:
			case GRNotifications::NotificationsPosition::BC:
			case GRNotifications::NotificationsPosition::BR:
				notif_list_node->set_v_grow_direction(Control::GROW_DIRECTION_BEGIN);
				notif_list_node->set_alignment(BoxContainer::ALIGN_END);
				break;
		}
	}

	_set_all_notifications_positions(pos);
	notifications_position = pos;
}

void GRNotifications::_set_all_notifications_positions(NotificationsPosition pos) {
	for (int i = singleton->notifications.size() - 1; i >= 0; i--) {
		auto *np = notifications[i];
		if (np && !np->is_queued_for_deletion())
			np->set_notification_position((NotificationsPosition)pos);
	}
}

void GRNotifications::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_EXIT_TREE: {
			GRNotificationPanel::clear_styles();
			break;
		}
	}
}

void GRNotifications::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_set_notifications_position", "pos"), &GRNotifications::_set_notifications_position);
	ClassDB::bind_method(D_METHOD("_add_notification", "title", "text", "update_existing"), &GRNotifications::_add_notification);
	ClassDB::bind_method(D_METHOD("_remove_notification", "title", "is_all_entries"), &GRNotifications::_remove_notification);
	ClassDB::bind_method(D_METHOD("_remove_exact_notification", "notif"), &GRNotifications::_remove_exact_notification);
	ClassDB::bind_method(D_METHOD("_clear_notifications"), &GRNotifications::_clear_notifications);

	ClassDB::bind_method(D_METHOD("_remove_list"), &GRNotifications::_remove_list);

	//ADD_SIGNAL(MethodInfo("status_changed", PropertyInfo(Variant::INT, "status")));
	//ADD_PROPERTY(PropertyInfo(Variant::REAL, "port"), "set_port", "get_port");
	//BIND_ENUM_CONSTANT_CUSTOM(WorkingStatus::Starting, "STATUS_STARTING");
}

int GRNotifications::get_notifications_position() {
	if (singleton) {
		return (int)singleton->notifications_position;
	}
	return 0;
}

void GRNotifications::set_notifications_position(int positon) {
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
		if (!_enabled)
			clear_notifications();
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
	if (singleton)
		return singleton->style;
	return Ref<GRNotificationStyle>();
}

void GRNotifications::set_notifications_style(Ref<GRNotificationStyle> _style) {
	if (singleton) {
		singleton->style = _style;
	}
}

void GRNotifications::add_notification(String title, String text, bool update_existing) {
	if (singleton) {
		singleton->call_deferred("_add_notification", title, text, update_existing);
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

void GRNotifications::_add_notification(String title, String text, bool update_existing) {
	if (!notifications_enabled)
		return;

	if (notif_list_node && !notif_list_node->is_queued_for_deletion()) {
		GRNotificationPanel *np;
		if (update_existing) {
			np = get_notification(title);
			if (np) {
				_log("Updating existing notification with Title: \"" + title + "\"" + " and Text:\"" + text + "\"", LogLevel::LL_Debug);
				goto set_new_data;
			}
		}

		_log("New notification added with Title: \"" + title + "\"" + " and Text:\"" + text + "\"", LogLevel::LL_Debug);
		np = memnew(GRNotificationPanel);
		notif_list_node->add_child(np);
		if (notifications_position <= NotificationsPosition::TR)
			notif_list_node->move_child(np, 0);
		notifications.push_back(np);

	set_new_data:

		np->set_data(this, title, text, style);

		// FORCE UPDATE SIZE OF CONTEINER
		notif_list_node->call("_size_changed");
	}
}

void GRNotifications::_remove_notification(String title, bool all_entries) {
	if (all_entries) {
		auto nps = get_all_notifications_with_title(title);
		for (int i = 0; i < nps.size(); i++) {
			notif_list_node->remove_child(nps[i]);
			notifications.erase(nps[i]);
			memdelete(nps[i]);
		}
	} else {
		auto *np = get_notification(title);
		notif_list_node->remove_child(np);
		notifications.erase(np);
		memdelete(np);
	}

	// FORCE UPDATE SIZE OF CONTEINER
	notif_list_node->call("_size_changed");
}

void GRNotifications::_remove_exact_notification(Node *_notif) {
	GRNotificationPanel *np = cast_to<GRNotificationPanel>(_notif);
	if (np) {
		notif_list_node->remove_child(np);
		notifications.erase(np);
		memdelete(np);

		// FORCE UPDATE SIZE OF CONTEINER
		notif_list_node->call("_size_changed");
	}
}

void GRNotifications::_clear_notifications() {
	for (int i = 0; i < notifications.size(); i++) {
		notif_list_node->remove_child(notifications[i]);
		notifications.erase(notifications[i]);
		memdelete(notifications[i]);
	}
}

GRNotifications *GRNotifications::get_singleton() {
	return singleton;
}

GRNotifications::GRNotifications() {
	if (!singleton)
		singleton = this;

	set_name("Notifications");

	if (Engine::get_singleton()->is_editor_hint())
		return;

	notifications_enabled = GET_PS(GodotRemote::ps_notifications_enabled_name);
	notifications_position = (NotificationsPosition)(int)GET_PS(GodotRemote::ps_noticications_position_name);
	notifications_duration = GET_PS(GodotRemote::ps_notifications_duration_name);

	notif_list_node = memnew(VBoxContainer);
	notif_list_node->set_name("NotificationList");
	call_deferred("add_child", notif_list_node);

	notif_list_node->set_anchor_and_margin(MARGIN_LEFT, 0.f, 8.f);
	notif_list_node->set_anchor_and_margin(MARGIN_RIGHT, 1.f, -8.f);
	notif_list_node->set_anchor_and_margin(MARGIN_TOP, 0.f, 8.f);
	notif_list_node->set_anchor_and_margin(MARGIN_BOTTOM, 1.f, -8.f);
	notif_list_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	notif_list_node->set_mouse_filter(Control::MouseFilter::MOUSE_FILTER_IGNORE);

	set_notifications_position((int)notifications_position);
}

GRNotifications::~GRNotifications() {
	if (this == singleton)
		singleton = nullptr;
	call_deferred("_remove_list");
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

void GRNotificationPanel::_remove_this_notification() {
	if (owner && !owner->is_queued_for_deletion()) {
		owner->remove_notification_exact(this);
	}
}

void GRNotificationPanel::_setup_tween(Tween *_tween) {
	const float duration = owner->get_notifications_duration();
	_tween->remove_all();
	_tween->interpolate_property(this, NodePath("modulate"), Color(1, 1, 1, 1), Color(1, 1, 1, 0), duration, Tween::TRANS_QUART, Tween::EASE_IN);
	_tween->interpolate_callback(this, duration, "_remove_this_notification", Variant(), Variant(), Variant(), Variant(), Variant());
}

void GRNotificationPanel::_update_style() {
	Ref<GRNotificationStyle> s = get_style();
	if (style.is_valid())
		s = style;

	add_style_override("panel", s->get_panel_style());
	title_node->add_font_override("font", s->get_title_font());
	text_node->add_font_override("font", s->get_text_font());
	close_node->set_theme(s->get_close_button_theme());
	close_node->add_font_override("font", s->get_title_font());
}

Ref<GRNotificationStyle> GRNotificationPanel::get_style() {
	if (_default_style.is_valid()) {
		return _default_style;
	}
	_default_style.instance();

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
	_default_style->set_panel_style(panel);

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
	_default_style->set_close_button_theme(close_btn_theme);

	return _default_style;
}

void GRNotificationPanel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_remove_this_notification"), &GRNotificationPanel::_remove_this_notification);

	ClassDB::bind_method(D_METHOD("get_title"), &GRNotificationPanel::get_title);
	ClassDB::bind_method(D_METHOD("get_text"), &GRNotificationPanel::get_text);
}

void GRNotificationPanel::clear_styles() {
	if (_default_style.is_valid())
		_default_style.unref();
}

void GRNotificationPanel::set_notification_position(GRNotifications::NotificationsPosition position) {
	switch (position) {
		case GRNotifications::NotificationsPosition::TL:
		case GRNotifications::NotificationsPosition::BL:
			set_h_size_flags(0);
			break;
		case GRNotifications::NotificationsPosition::TR:
		case GRNotifications::NotificationsPosition::BR:
			set_h_size_flags(SIZE_SHRINK_END);
			break;
		case GRNotifications::NotificationsPosition::TC:
		case GRNotifications::NotificationsPosition::BC:
			set_h_size_flags(SIZE_SHRINK_CENTER);
			break;
	}
}

void GRNotificationPanel::set_data(GRNotifications *_owner, String title, String text, Ref<GRNotificationStyle> _style) {
	owner = _owner;
	title_node->set_text(title);
	text_node->set_text(text);
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

GRNotificationPanel::GRNotificationPanel() {
	set_name("NotificationPanel");

	set_mouse_filter(Control::MouseFilter::MOUSE_FILTER_IGNORE);

	vbox_node = memnew(VBoxContainer);
	hbox_node = memnew(HBoxContainer);
	title_node = memnew(Label);
	text_node = memnew(Label);
	close_node = memnew(Button);
	tween_node = memnew(Tween);

	add_child(vbox_node);
	add_child(tween_node);
	vbox_node->add_child(hbox_node);
	vbox_node->add_child(text_node);
	hbox_node->add_child(title_node);
	hbox_node->add_child(close_node);

	vbox_node->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	vbox_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	vbox_node->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);

	hbox_node->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	hbox_node->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
	hbox_node->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);

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

	close_node->set_text("[x]");
	close_node->set_focus_mode(Control::FOCUS_NONE);
	close_node->connect("pressed", this, "_remove_this_notification");

	_setup_tween(tween_node);
	_update_style();
}

GRNotificationPanel::~GRNotificationPanel() {
	if (style.is_valid())
		style.unref();
}

// STYLE REF

void GRNotificationStyle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_panel_style"), &GRNotificationStyle::set_panel_style);
	ClassDB::bind_method(D_METHOD("set_close_button_theme"), &GRNotificationStyle::set_close_button_theme);
	ClassDB::bind_method(D_METHOD("set_title_font"), &GRNotificationStyle::set_title_font);
	ClassDB::bind_method(D_METHOD("set_text_font"), &GRNotificationStyle::set_text_font);

	ClassDB::bind_method(D_METHOD("get_panel_style"), &GRNotificationStyle::get_panel_style);
	ClassDB::bind_method(D_METHOD("get_close_button_theme"), &GRNotificationStyle::get_close_button_theme);
	ClassDB::bind_method(D_METHOD("get_title_font"), &GRNotificationStyle::get_title_font);
	ClassDB::bind_method(D_METHOD("get_text_font"), &GRNotificationStyle::get_text_font);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "panel_style"), "set_panel_style", "get_panel_style");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "close_button_theme"), "set_close_button_theme", "get_close_button_theme");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "title_font"), "set_title_font", "get_title_font");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "text_font"), "set_text_font", "get_text_font");
}

GRNotificationStyle::~GRNotificationStyle() {
	panel_style.unref();
	close_button_theme.unref();
	title_font.unref();
	text_font.unref();
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
