/* GodotRemote.cpp */
#include "GodotRemote.h"
#include "GRDevice.h"
#include "GRDeviceDevelopment.h"
#include "GRDeviceStandalone.h"
#include "core/os/os.h"
#include "core/project_settings.h"
#include "jpge.h"
#include "main/input_default.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "thirdparty/jpeg-compressor/jpgd.h"

GodotRemote *GodotRemote::singleton = nullptr;
const String GodotRemote::ps_autoload_name = "debug/settings/godot_remote/autostart";

using namespace GRUtils;

void GodotRemote::set_log_level(GRUtils::LogLevel lvl) {
	GRUtils::set_log_level(lvl);
}

PoolVector<uint8_t> GodotRemote::compress_jpg(Ref<Image> orig_img, int quality, float scale, int subsampling) {
	PoolVector<uint8_t> res;
	ERR_FAIL_COND_V(!orig_img.ptr(), res);
	ERR_FAIL_COND_V(scale < 0.01f, res);
	int time = OS::get_singleton()->get_ticks_usec();

#define Count(str)                                                                                              \
	print_line(str + String(": ") + String::num_real((OS::get_singleton()->get_ticks_usec() - time) / 1000.0)); \
	time = OS::get_singleton()->get_ticks_usec()

#define Count(str)

	Image img;
	img.copy_internals_from(orig_img);
	Count("Copy img");
	if (scale != 1.f)
		img.resize(img.get_width() * scale, img.get_height() * scale);
	Count("Resize img");
	img.convert(Image::FORMAT_RGBA8);
	Count("Convert img");

	jpge::params params;
	params.m_quality = quality;
	params.m_subsampling = (jpge::subsampling_t)subsampling;

	ERR_FAIL_COND_V(!params.check(), res);
	auto rb = compress_buffer.read();
	auto ri = img.get_data().read();

	int size = compress_buffer.size();
	ERR_FAIL_COND_V_MSG(!jpge::compress_image_to_jpeg_file_in_memory(
								(void *)rb.ptr(),
								size,
								img.get_width(),
								img.get_height(),
								4,
								(const unsigned char *)ri.ptr(),
								params),
			res, "GodotRemote::Error! Can't compress image.");

	rb.release();
	ri.release();
	Count("Compress img");

	res.append_array(compress_buffer.subarray(0, size - 1));
	Count("Combine arrays");

#undef Count
	return res;
}

void GodotRemote::set_gravity(const Vector3 &p_gravity) const {
	if (id)
		id->set_gravity(p_gravity);
}

void GodotRemote::set_accelerometer(const Vector3 &p_accel) const {
	if (id)
		id->set_accelerometer(p_accel);
}

void GodotRemote::set_magnetometer(const Vector3 &p_magnetometer) const {
	if (id)
		id->set_magnetometer(p_magnetometer);
}

void GodotRemote::set_gyroscope(const Vector3 &p_gyroscope) const {
	if (id)
		id->set_gyroscope(p_gyroscope);
}

GRDevice *GodotRemote::get_device() const {
	return device;
}

bool GodotRemote::start_remote_device(DeviceType type) {
	stop_remote_device();
	GRDevice *d = nullptr;

	switch (type) {
		case GodotRemote::DEVICE_Auto:
			if (OS::get_singleton()->has_feature("standalone"))
				d = new GRDeviceStandalone();
			else
				d = new GRDeviceDevelopment();
			break;
		case GodotRemote::DEVICE_Development:
			d = new GRDeviceDevelopment();
			break;
		case GodotRemote::DEVICE_Standalone:
			d = new GRDeviceStandalone();
			break;
		default:
			ERR_FAIL_V_MSG(false, "Not allowed type!");
			break;
	}

	if (d->start()) {
		device = d;
		SceneTree::get_singleton()->get_root()->call_deferred("add_child", device);
		SceneTree::get_singleton()->get_root()->call_deferred("move_child", device, 0);
		return true;
	}

	return false;
}

bool GodotRemote::stop_remote_device() {
	if (device) {
		device->stop();
		device->queue_delete();
		device = nullptr;
		return true;
	}
	return false;
}

void GodotRemote::_bind_methods() {
	ClassDB::bind_method(D_METHOD("compress_jpg", "image", "quality", "scale", "subsampling"), &GodotRemote::compress_jpg, DEFVAL(1.f), DEFVAL(Subsampling::SUBSAMPLING_H2V2));
	ClassDB::bind_method(D_METHOD("start_remote_device", "device_type"), &GodotRemote::start_remote_device, DEFVAL(DeviceType::DEVICE_Auto));
	ClassDB::bind_method(D_METHOD("stop_remote_device"), &GodotRemote::stop_remote_device);

	ClassDB::bind_method(D_METHOD("get_device"), &GodotRemote::get_device);

	ClassDB::bind_method(D_METHOD("set_gravity", "value"), &GodotRemote::set_gravity);
	ClassDB::bind_method(D_METHOD("set_accelerometer", "value"), &GodotRemote::set_accelerometer);
	ClassDB::bind_method(D_METHOD("set_magnetometer", "value"), &GodotRemote::set_magnetometer);
	ClassDB::bind_method(D_METHOD("set_gyroscope", "value"), &GodotRemote::set_gyroscope);

	BIND_ENUM_CONSTANT(DEVICE_Auto);
	BIND_ENUM_CONSTANT(DEVICE_Development);
	BIND_ENUM_CONSTANT(DEVICE_Standalone);

	BIND_ENUM_CONSTANT(SUBSAMPLING_Y_ONLY);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H1V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V2);

	// GRUtils
	int LL_None = GRUtils::LL_None;
	int LL_Debug = GRUtils::LL_Debug;
	int LL_Normal = GRUtils::LL_Normal;
	int LL_Warning = GRUtils::LL_Warning;
	int LL_Error = GRUtils::LL_Error;
	BIND_ENUM_CONSTANT(LL_None);
	BIND_ENUM_CONSTANT(LL_Debug);
	BIND_ENUM_CONSTANT(LL_Normal);
	BIND_ENUM_CONSTANT(LL_Warning);
	BIND_ENUM_CONSTANT(LL_Error);
	ClassDB::bind_method(D_METHOD("set_log_level", "level"), &GodotRemote::set_log_level);
}

void GodotRemote::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_PREDELETE:
			break;
	}
}

GodotRemote *GodotRemote::get_singleton() {
	return singleton;
}

GodotRemote::GodotRemote() {
	if (!singleton)
		singleton = this;

	compress_buffer.resize(1024 * 1024);

	id = (InputDefault *)Input::get_singleton();

	Variant autostart = GLOBAL_GET(ps_autoload_name);
	if (autostart.get_type() != Variant::BOOL) {
		ProjectSettings::get_singleton()->clear(ps_autoload_name);
		autostart = GLOBAL_DEF(ps_autoload_name, true);
	}

	if (autostart && !Engine::get_singleton()->is_editor_hint())
		call_deferred("start_remote_device");
}
