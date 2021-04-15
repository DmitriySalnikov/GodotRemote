/* GRProfilerViewportMiniPreview.h */
#pragma once

#include "GRUtils.h"
#include "GRViewportCaptureRect.h"

#ifndef GDNATIVE_LIBRARY
#else
using namespace godot;
#endif

class GRProfilerViewportMiniPreview : public GRViewportCaptureRect {
	GD_CLASS(GRProfilerViewportMiniPreview, GRViewportCaptureRect);

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

public:

	virtual void on_process() override;
	void set_enabled_preview(bool state);
	void _init();
	void _deinit();
};
