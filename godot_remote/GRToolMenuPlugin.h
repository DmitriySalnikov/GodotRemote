/* GRToolMenuPlugin.h */
#pragma once

#if !defined(GDNATIVE_LIBRARY) && defined(TOOLS_ENABLED)
#include "GRUtils.h"

#include "editor/editor_node.h"

class GRToolMenuPlugin : public EditorPlugin {
	GD_CLASS(GRToolMenuPlugin, EditorPlugin);

	enum GodotRemoteMenuItems {
		OpenHomePage,
		OpenDownloadPage,
	};

	EditorNode *editor;
	void _menu_pressed(int id);

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

public:
	virtual String get_name() const { return "GodotRemoteMenu"; }
	bool has_main_screen() const { return false; }

	GRToolMenuPlugin(EditorNode *p_node);
	~GRToolMenuPlugin();
};

#endif