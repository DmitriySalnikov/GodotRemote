/* GRProfilerViewportMiniPreview.cpp */

#include "GRProfilerViewportMiniPreview.h"

#ifndef GDNATIVE_LIBRARY
#else
#include <SceneTree.hpp>
#include <ViewportTexture.hpp>
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY

void GRProfilerViewportMiniPreview::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_size)), &GRProfilerViewportMiniPreview::_update_size);
	ClassDB::bind_method(D_METHOD(NAMEOF(_on_draw)), &GRProfilerViewportMiniPreview::_on_draw);
}

#else

void GRProfilerViewportMiniPreview::_register_methods() {
	METHOD_REG(GRProfilerViewportMiniPreview, _on_draw);
	METHOD_REG(GRProfilerViewportMiniPreview, _update_size);
	METHOD_REG(GRProfilerViewportMiniPreview, _notification);
}

#endif

void GRProfilerViewportMiniPreview::_notification(int p_notification) {
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
			if (get_texture().is_null() || !is_enabled || !TracyIsConnected)
				break;

			Ref<Image> tmp_image = get_texture()->get_data();
			if (img_is_empty(tmp_image) && tmp_image->get_format() == Image::Format::FORMAT_RGBA8)
				_log("Can't copy viewport image data", LogLevel::LL_ERROR);
			else {

				auto r = tmp_image->get_data().read();
				FrameImage(r.ptr(), uint16_t(tmp_image->get_width()), uint16_t(tmp_image->get_height()), 1, false);
			}
			break;
		}
		case NOTIFICATION_ENTER_TREE: {
			main_vp = ST()->get_root();
			main_vp->connect("size_changed", this, NAMEOF(_update_size));
			_update_size();

			renderer = memnew(Node2D);
			renderer->set_name("MiniProfilerRenderer");
			renderer->connect("draw", this, NAMEOF(_on_draw));
			add_child(renderer);
			break;
		}
		default:
			break;
	}
}

void GRProfilerViewportMiniPreview::_update_size() {
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
		set_size(Size2(size.x - (int)size.x % 4, size.y - (int)size.y % 4));
		if (renderer)
			renderer->update();
	}
}

void GRProfilerViewportMiniPreview::_on_draw() {
	renderer->draw_texture_rect(main_vp->get_texture(), Rect2(Vector2(), get_size()), false);
}

void GRProfilerViewportMiniPreview::set_enabled_preview(bool state) {
	is_enabled = state;
}

void GRProfilerViewportMiniPreview::_init() {
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

void GRProfilerViewportMiniPreview::_deinit() {
	LEAVE_IF_EDITOR();
}
