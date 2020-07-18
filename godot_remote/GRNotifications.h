/* GRNotifications.h */
#pragma once

#include "GRUtils.h"
#include "scene/gui/panel_container.h"
#include "scene/main/canvas_layer.h"

class GRNotificationPanel;

class GRNotifications : public CanvasLayer {
	GDCLASS(GRNotifications, CanvasLayer);

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

	static int get_notifications_position();
	static void set_notifications_position(int positon);
	static bool get_notifications_enabled();
	static void set_notifications_enabled(bool _enabled);
	static float get_notifications_duration();
	static void set_notifications_duration(float _duration);

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

public:
private:
	GRNotifications *owner = nullptr;
	static Ref<StyleBoxFlat> default_style;
	static Ref<StyleBoxEmpty> close_normal_style;
	static Ref<StyleBoxEmpty> close_pressed_style;

	class VBoxContainer *vbox_node = nullptr;
	class HBoxContainer *hbox_node = nullptr;
	class Label *title_node = nullptr;
	class Label *text_node = nullptr;
	class Button *close_node = nullptr;
	class Tween *tween_node = nullptr;

	void _remove_this_notification();
	void _setup_tween(Tween * _tween);
	Ref<StyleBoxFlat> get_default_style();
	Ref<StyleBoxEmpty> get_close_normal_style();
	Ref<StyleBoxEmpty> get_close_pressed_style();

protected:
	static void _bind_methods();

public:
	static void clear_styles();

	void set_notification_position(GRNotifications::NotificationsPosition position);
	void set_data(GRNotifications *_owner, String title, String text);
	String get_title();
	String get_text();

	GRNotificationPanel();
	~GRNotificationPanel();
};

VARIANT_ENUM_CAST(GRNotifications::NotificationsPosition)
