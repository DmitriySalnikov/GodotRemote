/* GRViewportCaptureRect.h */
#pragma once

#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY

#include "scene/2d/node_2d.h"
#include "scene/main/viewport.h"
#else

#include <Node2D.hpp>
#include <Viewport.hpp>
using namespace godot;
#endif

class GRViewportCaptureRect : public Viewport {
	GD_CLASS(GRViewportCaptureRect, Viewport);

private:
protected:
	Viewport *main_vp = nullptr;
	Node2D *renderer = nullptr;
	Vector2 max_viewport_size = Vector2(128 * 3, 128 * 3);
	bool use_size_snap = false;
	int size_snap_step = 4;

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
	virtual void after_enter_tree(){};
	virtual void on_process(){};

	void set_max_viewport_size(Vector2 size);
	void set_use_size_snapping(bool use);
	void set_size_snap_step(int step);
	void _init();
	void _deinit();
};
