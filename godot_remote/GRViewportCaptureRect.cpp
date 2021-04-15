/* GRViewportCaptureRect.cpp */

#include "GRViewportCaptureRect.h"

#ifndef GDNATIVE_LIBRARY
#else
#include <SceneTree.hpp>
#include <ViewportTexture.hpp>
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY

void GRViewportCaptureRect::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_size)), &GRViewportCaptureRect::_update_size);
	ClassDB::bind_method(D_METHOD(NAMEOF(_on_draw)), &GRViewportCaptureRect::_on_draw);
}

#else

void GRViewportCaptureRect::_register_methods() {
	METHOD_REG(GRViewportCaptureRect, _on_draw);
	METHOD_REG(GRViewportCaptureRect, _update_size);
	METHOD_REG(GRViewportCaptureRect, _notification);
}

#endif

void GRViewportCaptureRect::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
		case NOTIFICATION_PROCESS: {
			on_process();
			break;
		}
		case NOTIFICATION_ENTER_TREE: {
			main_vp = ST()->get_root();
			main_vp->connect("size_changed", this, NAMEOF(_update_size));
			_update_size();

			renderer = memnew(Node2D);
			renderer->connect("draw", this, NAMEOF(_on_draw));
			add_child(renderer);
			after_enter_tree();
			break;
		}
		default:
			break;
	}
}

void GRViewportCaptureRect::_update_size() {
	if (main_vp && main_vp->get_texture().is_valid()) {
		Size2 size = main_vp->get_size();
		if (size.x < 16)
			size.x = 16;
		if (size.y < 16)
			size.y = 16;

		if (get_size() == size)
			return;

		float aspect = size.x / size.y;
		if (size.x > max_viewport_size.x || size.y > max_viewport_size.y) {
			if (aspect >= 1) {
				size.x = max_viewport_size.x;
				size.y = size.x / aspect;
			} else {
				size.y = max_viewport_size.y;
				size.x = size.y * aspect;

				float a2 = size.x / size.y;
				if (size.x > max_viewport_size.x) {
					size.x = max_viewport_size.x;
					size.y = size.x * a2;
				}
			}
		}
		
		Size2 res = size;
		if (use_size_snap) {
			res = Size2(size.x - (int)size.x % size_snap_step, size.y - (int)size.y % size_snap_step);

			if (res.x < size_snap_step)
				res.x = (real_t)size_snap_step;
			if (res.y < size_snap_step)
				res.y = (real_t)size_snap_step;
		} else {
			if (res.x < 1)
				res.x = 1;
			if (res.y < 1)
				res.y = 1;
		}

		set_size(res);
		if (renderer)
			renderer->update();
	}
}

void GRViewportCaptureRect::_on_draw() {
	renderer->draw_texture_rect(main_vp->get_texture(), Rect2(Vector2(), get_size()), false);
}

void GRViewportCaptureRect::_init() {
	set_name("GRProfilerViewportMiniPreview");
	LEAVE_IF_EDITOR();

	set_process(true);

	set_hdr(false);
	set_disable_3d(true);
	set_keep_3d_linear(false);
	set_usage(Viewport::USAGE_2D);
	set_update_mode(Viewport::UPDATE_ALWAYS);
	set_transparent_background(true);
	set_disable_input(true);
	set_shadow_atlas_quadrant_subdiv(0, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(1, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(2, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(3, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
}

void GRViewportCaptureRect::_deinit() {
	LEAVE_IF_EDITOR();
}

void GRViewportCaptureRect::set_max_viewport_size(Vector2 size) {
	ERR_FAIL_COND(size.x < 16 || size.y < 16);
	if (size != max_viewport_size) {
		max_viewport_size = size;
		_update_size();
	}
}

void GRViewportCaptureRect::set_use_size_snapping(bool use) {
	use_size_snap = use;
}

void GRViewportCaptureRect::set_size_snap_step(int step) {
	size_snap_step = 4;
}
