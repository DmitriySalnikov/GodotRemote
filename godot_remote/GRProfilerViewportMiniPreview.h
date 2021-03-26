/* GRProfilerViewportMiniPreview.h */
#pragma once

#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY

#include "scene/2d/node_2d.h"
#include "scene/main/viewport.h"
#else

#include <Node2D.hpp>
#include <Viewport.hpp>
#include <ViewportTexture.hpp>
using namespace godot;
#endif

class GRProfilerViewportMiniPreview : public Viewport {
	GD_CLASS(GRProfilerViewportMiniPreview, Viewport);

private:
protected:
	Viewport *main_vp = nullptr;
	Node2D *renderer = nullptr;
	Vector2 max_viewport_size = Vector2(128 * 3, 128 * 3);

// by default it will disabled for mobile
#if defined(__ANDROID__)
	bool is_enabled = false;
#else
	bool is_enabled = true;
#endif

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif
	void _notification(int p_notification);
	void _update_size();
	void _on_draw();

public:
	void set_enabled_preview(bool state);
	void _init();
	void _deinit();
};
