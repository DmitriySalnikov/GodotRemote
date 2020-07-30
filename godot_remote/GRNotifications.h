/* GRNotifications.h */
#pragma once

#include "GRUtils.h"
#include "core/reference.h"
#include "scene/gui/panel_container.h"
#include "scene/main/canvas_layer.h"

class GRNotificationPanel;
class GRNotificationStyle;

enum NotificationIcon {
	None,
	_Error,
	Warning,
	Success,
	Fail,

	MAX,
};

enum NotificationsPosition {
	//TL, TC, TR,
	//BL, BC, BR,
	TL = 0,
	TC = 1,
	TR = 2,
	BL = 3,
	BC = 4,
	BR = 5,
};

class GRNotifications : public CanvasLayer {
	GDCLASS(GRNotifications, CanvasLayer);

	friend class GRNotificationPanel;

public:
private:
	static GRNotifications *singleton;

	bool clearing_notifications = false;

	float notifications_duration = 2.0;
	bool notifications_enabled = true;
	NotificationsPosition notifications_position = NotificationsPosition::TL;

	class VBoxContainer *notif_list_node = nullptr;
	List<GRNotificationPanel *> notifications;
	Ref<GRNotificationStyle> style;

	List<GRNotificationPanel *> _get_notifications_with_title(String title);
	GRNotificationPanel *_get_notification(String title);

	void _set_all_notifications_positions(NotificationsPosition pos);

	void _set_notifications_position(NotificationsPosition positon);
	void _add_notification_or_append_string(String title, String text, NotificationIcon icon, bool new_string, float duration_multiplier);
	void _add_notification_or_update_line(String title, String id, String text, NotificationIcon icon, float duration_multiplier);
	void _add_notification(String title, String text, NotificationIcon icon, bool update_existing, float duration_multiplier);
	void _remove_notification(String title, bool all_entries);
	void _remove_exact_notification(Node *_notif);
	void _clear_notifications();

	void _remove_list();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	/// All functions below need to be called after init notification manager
	/// For example call this after yield(get_tree(), "idle_frame")

	static GRNotificationPanel *get_notification(String title);
	static Array get_all_notifications();
	static Array get_notifications_with_title(String title);

	static NotificationsPosition get_notifications_position();
	static void set_notifications_position(NotificationsPosition positon);
	static bool get_notifications_enabled();
	static void set_notifications_enabled(bool _enabled);
	static float get_notifications_duration();
	static void set_notifications_duration(float _duration);
	static Ref<class GRNotificationStyle> get_notifications_style();
	static void set_notifications_style(Ref<class GRNotificationStyle> _style);

	// append text to existing notification or add new notification
	static void add_notification_or_append_string(String title, String text, NotificationIcon icon = NotificationIcon::None, bool new_string = true, float duration_multiplier = 1.f);

	// update text in existing notification or add new notification
	static void add_notification_or_update_line(String title, String id, String text, NotificationIcon icon = NotificationIcon::None, float duration_multiplier = 1.f);

	static void add_notification(String title, String text, NotificationIcon icon = NotificationIcon::None, bool update_existing = true, float duration_multiplier = 1.f);
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

protected:
	GRNotifications *owner = nullptr;

	NotificationIcon notification_icon = NotificationIcon::None;
	float duration_mul = 1.f;
	bool is_hovered = false;
	Ref<GRNotificationStyle> style;
	static Ref<GRNotificationStyle> _default_style;
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	static Dictionary _default_textures;
	static Ref<ImageTexture> _default_close_texture;
#endif

	class VBoxContainer *vbox_node = nullptr;
	class HBoxContainer *hbox_node = nullptr;
	class TextureRect *icon_tex_node = nullptr;
	class Label *title_node = nullptr;
	class Label *text_node = nullptr;
	class Button *close_node = nullptr;
	class Tween *tween_node = nullptr;

	void _panel_hovered();
	void _panel_lose_hover();
	void _remove_this_notification();
	void _setup_tween(Tween *_tween);
	void _update_style();

	static Ref<class GRNotificationStyle> generate_default_style();
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	static void _load_default_textures();
#endif

protected:
	static void _bind_methods();

public:
	static void clear_styles();

	void set_notification_position(NotificationsPosition position);
	virtual void set_data(GRNotifications *_owner, String title, String text, NotificationIcon icon, float duration_multiplier = 1.f, Ref<GRNotificationStyle> _style = Ref<GRNotificationStyle>());
	String get_title();
	String get_text();
	void update_text(String text);

	GRNotificationPanel();
	~GRNotificationPanel();
};

class GRNotificationPanelUpdatable : public GRNotificationPanel {
	GDCLASS(GRNotificationPanelUpdatable, GRNotificationPanel);

	friend GRNotifications;

protected:
	Dictionary lines;
	String _get_text_from_lines();
	bool configured = false;
	static void _bind_methods();

public:
	void set_updatable_line(GRNotifications *_owner, String title, String id, String text, NotificationIcon icon, float duration_multiplier = 1.f, Ref<GRNotificationStyle> _style = Ref<GRNotificationStyle>());
	void remove_updatable_line(String id);
	void clear_lines();
};

// STYLE REF

class GRNotificationStyle : public Reference {
	GDCLASS(GRNotificationStyle, Reference);

private:
	Ref<StyleBox> panel_style;
	Ref<Theme> close_button_theme;
	Ref<Texture> close_button_icon;
	Ref<Font> title_font;
	Ref<Font> text_font;
	Dictionary icons;

protected:
	static void _bind_methods();

public:
	~GRNotificationStyle();

	void set_panel_style(Ref<StyleBox> style);
	Ref<StyleBox> get_panel_style();

	void set_close_button_theme(Ref<Theme> theme);
	Ref<Theme> get_close_button_theme();

	void set_close_button_icon(Ref<Texture> icon);
	Ref<Texture> get_close_button_icon();

	void set_title_font(Ref<Font> font);
	Ref<Font> get_title_font();

	void set_text_font(Ref<Font> font);
	Ref<Font> get_text_font();

	void set_notification_icon(NotificationIcon notification_icon, Ref<Texture> icon_texture);
	Ref<Texture> get_notification_icon(NotificationIcon notification_icon);
};

VARIANT_ENUM_CAST(NotificationsPosition)
VARIANT_ENUM_CAST(NotificationIcon)
