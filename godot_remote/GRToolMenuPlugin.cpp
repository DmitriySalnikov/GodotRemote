/* GRToolMenuPlugin.cpp */

#include "GRToolMenuPlugin.h"

#if !defined(GDNATIVE_LIBRARY) && defined(TOOLS_ENABLED)
#include "core/os/os.h"
#include "scene/gui/popup_menu.h"

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
	METHOD_REG(GRToolMenuPlugin, _menu_pressed);
}
#endif

GRToolMenuPlugin::GRToolMenuPlugin(EditorNode *p_node) {
	editor = p_node;
	auto menu = memnew(PopupMenu);

	menu->add_item("Open Home Page", GodotRemoteMenuItems::OpenHomePage);
	menu->add_item("Open Download Page", GodotRemoteMenuItems::OpenDownloadPage);
	menu->connect("id_pressed", this, "_menu_pressed");
	add_tool_submenu_item("Godot Remote", menu);
}

GRToolMenuPlugin::~GRToolMenuPlugin() {
	remove_tool_menu_item("Godot Remote");
}

#endif