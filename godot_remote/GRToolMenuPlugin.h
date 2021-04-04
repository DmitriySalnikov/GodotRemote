/* GRToolMenuPlugin.h */
#pragma once

#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#include "editor/editor_node.h"
#else

#include <EditorPlugin.hpp>
#endif

#if defined(TOOLS_ENABLED) || defined(GDNATIVE_LIBRARY)

class GRToolMenuPlugin : public EditorPlugin {
	GD_CLASS(GRToolMenuPlugin, EditorPlugin);

	enum GodotRemoteMenuItems {
		OpenHomePage,
		OpenDownloadPage,
	};

	void _menu_pressed(int id);

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
	virtual String get_name() const { return "GodotRemoteMenu"; }
	bool has_main_screen() const { return false; }

	void _init();
	void _deinit();

#ifndef GDNATIVE_LIBRARY
	GRToolMenuPlugin(EditorNode *p_node);
	~GRToolMenuPlugin();
#else
#endif
};

#endif
