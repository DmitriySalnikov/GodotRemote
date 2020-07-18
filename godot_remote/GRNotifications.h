/* GRNotifications.h */
#pragma once

#include "GRUtils.h"
#include "core/reference.h"
#include "scene/gui/panel_container.h"
#include "scene/main/canvas_layer.h"

class GRNotificationPanel;
class GRNotificationStyle;

class GRNotifications : public CanvasLayer {
	GDCLASS(GRNotifications, CanvasLayer);

	friend class GRNotificationPanel;

public:
	enum class NotificationsPosition {
		//TL, TC, TR,
		//BL, BC, BR,
		TL = 0,
		TC = 1,
		TR = 2,
		BL = 3,
		BC = 4,
		BR = 5,
	};

private:
	static GRNotifications *singleton;

	float notifications_duration = 2.0;
	bool notifications_enabled = true;
	NotificationsPosition notifications_position = NotificationsPosition::TL;

	class VBoxContainer *notif_list_node = nullptr;
	List<GRNotificationPanel *> notifications;
	Ref<GRNotificationStyle> style;

	List<GRNotificationPanel *> get_all_notifications_with_title(String title);
	GRNotificationPanel *get_notification(String title);

	void _set_all_notifications_positions(NotificationsPosition pos);

	void _set_notifications_position(int positon);
	void _add_notification(String title, String text, bool update_existing = true);
	void _remove_notification(String title, bool all_entries = true);
	void _remove_exact_notification(Node *_notif);
	void _clear_notifications();

	void _remove_list();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:

	/// All functions below need to be called after init notification manager
	/// For example call this after yield(get_tree(), "idle_frame")

	static int get_notifications_position();
	static void set_notifications_position(int positon);
	static bool get_notifications_enabled();
	static void set_notifications_enabled(bool _enabled);
	static float get_notifications_duration();
	static void set_notifications_duration(float _duration);
	static Ref<class GRNotificationStyle> get_notifications_style();
	static void set_notifications_style(Ref<class GRNotificationStyle> _style);

	static Array get_all_notifications();
	static void add_notification(String title, String text, bool update_existing = true);
	static void remove_notification(String title, bool all_entries = true);
	static void remove_notification_exact(Node *_notif);
	static void clear_notifications();
	static GRNotifications *get_singleton();
	GRNotifications();
	~GRNotifications();
};

class GRNotificationPanel : public PanelContainer {
	GDCLASS(GRNotificationPanel, PanelContainer);

	friend GRNotifications;

public:
private:
	GRNotifications *owner = nullptr;

	Ref<GRNotificationStyle> style;
	static Ref<GRNotificationStyle> _default_style;

	class VBoxContainer *vbox_node = nullptr;
	class HBoxContainer *hbox_node = nullptr;
	class Label *title_node = nullptr;
	class Label *text_node = nullptr;
	class Button *close_node = nullptr;
	class Tween *tween_node = nullptr;

	void _remove_this_notification();
	void _setup_tween(Tween *_tween);
	void _update_style();

	static Ref<class GRNotificationStyle> get_style();

protected:
	static void _bind_methods();

public:
	static void clear_styles();

	void set_notification_position(GRNotifications::NotificationsPosition position);
	void set_data(GRNotifications *_owner, String title, String text, Ref<GRNotificationStyle> _style = Ref<GRNotificationStyle>());
	String get_title();
	String get_text();

	GRNotificationPanel();
	~GRNotificationPanel();
};

// STYLE REF

class GRNotificationStyle : public Reference {
	GDCLASS(GRNotificationStyle, Reference);

private:
	Ref<StyleBox> panel_style;
	Ref<Theme> close_button_theme;
	Ref<Font> title_font;
	Ref<Font> text_font;

protected:
	static void _bind_methods();

public:
	~GRNotificationStyle();

	void set_panel_style(Ref<StyleBox> style);
	Ref<StyleBox> get_panel_style();

	void set_close_button_theme(Ref<Theme> theme);
	Ref<Theme> get_close_button_theme();

	void set_title_font(Ref<Font> font);
	Ref<Font> get_title_font();

	void set_text_font(Ref<Font> font);
	Ref<Font> get_text_font();
};

VARIANT_ENUM_CAST(GRNotifications::NotificationsPosition)
