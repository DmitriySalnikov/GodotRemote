/* GRNotifications.h */
#pragma once

#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#include "core/reference.h"
#include "scene/gui/panel_container.h"
#include "scene/main/canvas_layer.h"
#else

#include <Array.hpp>
#include <Button.hpp>
#include <CanvasLayer.hpp>
#include <Font.hpp>
#include <Godot.hpp>
#include <HBoxContainer.hpp>
#include <ImageTexture.hpp>
#include <Label.hpp>
#include <PanelContainer.hpp>
#include <PoolArrays.hpp>
#include <Ref.hpp>
#include <Reference.hpp>
#include <String.hpp>
#include <StyleBoxEmpty.hpp>
#include <StyleBoxFlat.hpp>
#include <TextureRect.hpp>
#include <Theme.hpp>
#include <Tween.hpp>
#include <VBoxContainer.hpp>
using namespace godot;
#endif

class GRNotificationPanel;
class GRNotificationStyle;
class GRNotificationPanelSTATIC_DATA;

class GRNotifications : public CanvasLayer {
	GD_CLASS(GRNotifications, CanvasLayer);

	friend class GRNotificationPanel;

public:
	enum NotificationIcon : int {
		ICON_NONE,
		ICON_ERROR,
		ICON_WARNING,
		ICON_SUCCESS,
		ICON_FAIL,

		ICON_MAX,
	};

	enum NotificationsPosition : int {
		TOP_LEFT = 0,
		TOP_CENTER = 1,
		TOP_RIGHT = 2,
		BOTTOM_LEFT = 3,
		BOTTOM_CENTER = 4,
		BOTTOM_RIGHT = 5,
	};

private:
	static GRNotifications *singleton;

	bool clearing_notifications = false;

	float notifications_duration = 2.0;
	bool notifications_enabled = true;
	NotificationsPosition notifications_position = NotificationsPosition::TOP_LEFT;

	class VBoxContainer *notif_list_node = nullptr;
	std::vector<GRNotificationPanel *> notifications; // GRNotificationPanel *
	Ref<GRNotificationStyle> style;

	std::vector<GRNotificationPanel *> _get_notifications_with_title(String title); // GRNotificationPanel *
	GRNotificationPanel *_get_notification(String title);

	void _set_all_notifications_positions(NotificationsPosition pos);

	void _set_notifications_position(ENUM_ARG(NotificationsPosition) positon);
	void _add_notification_or_append_string(String title, String text, ENUM_ARG(NotificationIcon) icon, bool new_string, float duration_multiplier);
	void _add_notification_or_update_line(String title, String id, String text, ENUM_ARG(NotificationIcon) icon, float duration_multiplier);
	void _add_notification(String title, String text, ENUM_ARG(NotificationIcon) icon, bool update_existing, float duration_multiplier);
	void _remove_notification(String title, bool all_entries);
	void _remove_exact_notification(Node *_notif);
	void _clear_notifications();

	void _remove_list();

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_what);

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
	static void add_notification_or_append_string(String title, String text, NotificationIcon icon = NotificationIcon::ICON_NONE, bool new_string = true, float duration_multiplier = 1.f);

	// update text in existing notification or add new notification
	static void add_notification_or_update_line(String title, String id, String text, NotificationIcon icon = NotificationIcon::ICON_NONE, float duration_multiplier = 1.);

	static void add_notification(String title, String text, NotificationIcon icon DEF_ARG(= NotificationIcon::ICON_NONE), bool update_existing = true, float duration_multiplier = 1.f);
	static void remove_notification(String title, bool all_entries = true);
	static void remove_notification_exact(Node *_notif);
	static void clear_notifications();
	static GRNotifications *get_singleton();

	void _init();
	void _deinit();
};

// Stupid class to bypass GDNative limitations
// but also it can be changed back by changing default-style generating
class GRNotificationPanelSTATIC_DATA {
public:
	Ref<GRNotificationStyle> _default_style;
#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	Dictionary _default_textures;
	Ref<ImageTexture> _default_close_texture;
#endif
};

class GRNotificationPanel : public PanelContainer {
	GD_CLASS(GRNotificationPanel, PanelContainer);

	friend GRNotifications;

protected:
	GRNotifications *owner = nullptr;

	GRNotifications::NotificationIcon notification_icon = GRNotifications::NotificationIcon::ICON_NONE;
	float duration_mul = 1.f;
	bool is_hovered = false;
	Ref<GRNotificationStyle> style;

	static GRNotificationPanelSTATIC_DATA *_default_data;

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
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	static void clear_styles();

	void set_notification_position(GRNotifications::NotificationsPosition position);
	virtual void set_data(GRNotifications *_owner, String title, String text, GRNotifications::NotificationIcon icon, float duration_multiplier DEF_ARG(= 1.f), Ref<GRNotificationStyle> _style DEF_ARG(= Ref<GRNotificationStyle>()));
	String get_title();
	String get_text();
	void update_text(String text);

	void _init();
	void _deinit();
};

class GRNotificationPanelUpdatable : public GRNotificationPanel {
	GD_S_CLASS(GRNotificationPanelUpdatable, GRNotificationPanel);

	friend GRNotifications;

protected:
	std::map<String, String> lines;
	String _get_text_from_lines();
	bool configured = false;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	void set_updatable_line(GRNotifications *_owner, String title, String id, String text, GRNotifications::NotificationIcon icon, float duration_multiplier DEF_ARG(= 1.f), Ref<GRNotificationStyle> _style DEF_ARG(= Ref<GRNotificationStyle>()));
	void remove_updatable_line(String id);
	void clear_lines();

	void _init();
	void _deinit();
};

// STYLE REF

class GRNotificationStyle : public Reference {
	GD_CLASS(GRNotificationStyle, Reference);

private:
	Ref<StyleBox> panel_style;
	Ref<Theme> close_button_theme;
	Ref<Texture> close_button_icon;
	Ref<Font> title_font;
	Ref<Font> text_font;
	Ref<Texture> n_icons[GRNotifications::NotificationIcon::ICON_MAX];

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
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

	void set_notification_icon(ENUM_ARG(GRNotifications::NotificationIcon) notification_icon, Ref<Texture> icon_texture);
	Ref<Texture> get_notification_icon(ENUM_ARG(GRNotifications::NotificationIcon) notification_icon);

	void _init();
	void _deinit();
};

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(GRNotifications::NotificationsPosition)
VARIANT_ENUM_CAST(GRNotifications::NotificationIcon)
#endif
