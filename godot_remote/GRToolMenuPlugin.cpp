/* GRToolMenuPlugin.cpp */

#include "GRToolMenuPlugin.h"

#ifndef GDNATIVE_LIBRARY
#include "scene/gui/popup_menu.h"
#else
#include "GodotRemote.h"

#include <AcceptDialog.hpp>
#include <EditorInterface.hpp>
#include <PopupMenu.hpp>
#include <ProjectSettings.hpp>
#endif

void GRToolMenuPlugin::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifdef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
#ifdef GDNATIVE_LIBRARY
			_deinit();
#endif
			break;
	}
}

void GRToolMenuPlugin::_menu_pressed(int id) {
	GodotRemoteMenuItems item = (GodotRemoteMenuItems)id;
	switch (item) {
		case OpenHomePage:
			OS::get_singleton()->shell_open("https://github.com/DmitriySalnikov/GodotRemote#godot-remote");
			break;
		case OpenDownloadPage:
			OS::get_singleton()->shell_open("https://dmitriysalnikov.itch.io/godot-remote");
			break;
		default:
			break;
	}
}

#ifndef GDNATIVE_LIBRARY
void GRToolMenuPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_menu_pressed)), &GRToolMenuPlugin::_menu_pressed);
}
#else
void GRToolMenuPlugin::_register_methods() {
	METHOD_REG(GRToolMenuPlugin, _notification);
	METHOD_REG(GRToolMenuPlugin, _menu_pressed);
}

void GRToolMenuPlugin::_show_warning_window() {
	AcceptDialog *ad = memnew(AcceptDialog);
	ad->set_title("Warning!");
	ad->set_exclusive(true);
	ad->set_text("The GDNative version of Godot Remote can work\nunstable and cause crashes of running games.");
	ad->connect("popup_hide", ad, "queue_free");
	get_editor_interface()->get_base_control()->add_child(ad);
	ad->call_deferred("popup_centered");
}
#endif

void GRToolMenuPlugin::_init() {
	auto menu = memnew(PopupMenu);

	menu->add_item("Open Home Page", GodotRemoteMenuItems::OpenHomePage);
	menu->add_item("Open Download Page", GodotRemoteMenuItems::OpenDownloadPage);
	menu->connect("id_pressed", this, "_menu_pressed");
	call_deferred(NAMEOF(add_tool_submenu_item), "Godot Remote", menu);

#ifdef GDNATIVE_LIBRARY
	bool show_warning = GET_PS(GodotRemote::ps_gdnative_warning_name);
	if (show_warning)
		_show_warning_window();
	ProjectSettings::get_singleton()->set_setting(GodotRemote::ps_gdnative_warning_name, false);
#endif
}

void GRToolMenuPlugin::_deinit() {
	remove_tool_menu_item("Godot Remote");
}

#ifndef GDNATIVE_LIBRARY
GRToolMenuPlugin::GRToolMenuPlugin(EditorNode *p_node) {
	_init();
}

GRToolMenuPlugin::~GRToolMenuPlugin() {
	_deinit();
}
#else
#endif