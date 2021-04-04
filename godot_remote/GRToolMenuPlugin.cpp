/* GRToolMenuPlugin.cpp */

#include "GRToolMenuPlugin.h"

#ifndef GDNATIVE_LIBRARY
#include "scene/gui/popup_menu.h"
#else
#include <PopupMenu.hpp>
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
			// TODO change to itch.io
			OS::get_singleton()->shell_open("https://github.com/DmitriySalnikov/GodotRemote/releases/latest");
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
#endif

void GRToolMenuPlugin::_init() {
	auto menu = memnew(PopupMenu);

	menu->add_item("Open Home Page", GodotRemoteMenuItems::OpenHomePage);
	menu->add_item("Open Download Page", GodotRemoteMenuItems::OpenDownloadPage);
	menu->connect("id_pressed", this, "_menu_pressed");
	add_tool_submenu_item("Godot Remote", menu);
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