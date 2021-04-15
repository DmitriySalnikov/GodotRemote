/* GRProfilerViewportMiniPreview.cpp */

#include "GRProfilerViewportMiniPreview.h"

#ifndef GDNATIVE_LIBRARY
#else
#include <ViewportTexture.hpp>
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY

void GRProfilerViewportMiniPreview::_bind_methods() {
}

#else

void GRProfilerViewportMiniPreview::_register_methods() {
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
			GRViewportCaptureRect::_deinit();
			break;
		default:
			break;
	}
}

void GRProfilerViewportMiniPreview::set_enabled_preview(bool state) {
	is_enabled = state;
}

void GRProfilerViewportMiniPreview::_init() {

#ifndef GDNATIVE_LIBRARY
#else
	GRViewportCaptureRect::_init();
#endif

	set_name("GRProfilerViewportMiniPreview");
	LEAVE_IF_EDITOR();

	set_max_viewport_size(Vector2(128 * 3, 128 * 3));
	set_use_size_snapping(true);
	set_size_snap_step(4);
}

void GRProfilerViewportMiniPreview::_deinit() {
	LEAVE_IF_EDITOR();
}

void GRProfilerViewportMiniPreview::on_process() {
	if (get_texture().is_null() || !is_enabled || !TracyIsConnected)
		return;

	Ref<Image> tmp_image = get_texture()->get_data();
	if (img_is_empty(tmp_image) && tmp_image->get_format() == Image::Format::FORMAT_RGBA8)
		_log("Can't copy viewport image data", LogLevel::LL_ERROR);
	else {

		auto r = tmp_image->get_data().read();
		FrameImage(r.ptr(), uint16_t(tmp_image->get_width()), uint16_t(tmp_image->get_height()), 1, false);
	}
}
