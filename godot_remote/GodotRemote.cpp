/* GodotRemote.cpp */
#include "GodotRemote.h"
#include "GRDevice.h"
#include "GRDeviceDevelopment.h"
#include "GRDeviceStandalone.h"
#include "core/os/os.h"
#include "core/project_settings.h"
#include "editor/editor_node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "thirdparty/jpeg-compressor/jpgd.h"

GodotRemote *GodotRemote::singleton = nullptr;

using namespace GRUtils;

GodotRemote *GodotRemote::get_singleton() {
	return singleton;
}

GodotRemote::GodotRemote() {
	if (!singleton)
		singleton = this;

#ifdef TOOLS_ENABLED
	if (Engine::get_singleton()->is_editor_hint())
		if (EditorNode::get_singleton())
			EditorNode::get_singleton()->connect("play_pressed", this, "native_run_emitted");
#endif

	register_and_load_settings();
	if (is_autostart && !Engine::get_singleton()->is_editor_hint())
		call_deferred("create_and_start_device");
}

GodotRemote::~GodotRemote() {
	compress_buffer.resize(0);
}

void GodotRemote::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_and_start_device", "device_type"), &GodotRemote::create_and_start_device, DEFVAL(DeviceType::DEVICE_Auto));
	ClassDB::bind_method(D_METHOD("create_remote_device", "device_type"), &GodotRemote::create_remote_device, DEFVAL(DeviceType::DEVICE_Auto));
	ClassDB::bind_method(D_METHOD("start_remote_device"), &GodotRemote::start_remote_device);
	ClassDB::bind_method(D_METHOD("stop_remote_device"), &GodotRemote::stop_remote_device);
#ifdef TOOLS_ENABLED
	ClassDB::bind_method(D_METHOD("_adb_port_forwarding"), &GodotRemote::_adb_port_forwarding);
	ClassDB::bind_method(D_METHOD("_native_run_emitted"), &GodotRemote::_native_run_emitted);
#endif

	ClassDB::bind_method(D_METHOD("get_device"), &GodotRemote::get_device);

	BIND_ENUM_CONSTANT(DEVICE_Auto);
	BIND_ENUM_CONSTANT(DEVICE_Development);
	BIND_ENUM_CONSTANT(DEVICE_Standalone);

	// GRUtils
	BIND_ENUM_CONSTANT(SUBSAMPLING_Y_ONLY);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H1V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V2);

	BIND_ENUM_CONSTANT(LL_None);
	BIND_ENUM_CONSTANT(LL_Debug);
	BIND_ENUM_CONSTANT(LL_Normal);
	BIND_ENUM_CONSTANT(LL_Warning);
	BIND_ENUM_CONSTANT(LL_Error);

	ClassDB::bind_method(D_METHOD("set_log_level", "level"), &GodotRemote::set_log_level);
	ClassDB::bind_method(D_METHOD("compress_jpg", "image", "quality", "subsampling"), &GodotRemote::compress_jpg, DEFVAL(75), DEFVAL(Subsampling::SUBSAMPLING_H2V2));

	ClassDB::bind_method(D_METHOD("set_gravity", "value"), &GodotRemote::set_gravity);
	ClassDB::bind_method(D_METHOD("set_accelerometer", "value"), &GodotRemote::set_accelerometer);
	ClassDB::bind_method(D_METHOD("set_magnetometer", "value"), &GodotRemote::set_magnetometer);
	ClassDB::bind_method(D_METHOD("set_gyroscope", "value"), &GodotRemote::set_gyroscope);
}

void GodotRemote::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_PREDELETE:
			break;
	}
}

GRDevice *GodotRemote::get_device() const {
	return device;
}

bool GodotRemote::create_remote_device(DeviceType type) {
	stop_remote_device();

	GRDevice *d = nullptr;

	switch (type) {
		// automatically start server if it not a standalone build
		case GodotRemote::DEVICE_Auto:
			if (!OS::get_singleton()->has_feature("standalone")) {
#ifndef NO_GODOTREMOTE_SERVER
				d = memnew(GRDeviceDevelopment);
#else
				ERR_FAIL_V_MSG(false, "Server not included in this build!");
#endif
			}
			break;
		case GodotRemote::DEVICE_Development:
#ifndef NO_GODOTREMOTE_SERVER
			d = memnew(GRDeviceDevelopment);
#else
			ERR_FAIL_V_MSG(false, "Server not included in this build!");
#endif
			break;
		case GodotRemote::DEVICE_Standalone:
#ifndef NO_GODOTREMOTE_CLIENT
			d = memnew(GRDeviceStandalone);
#else
			ERR_FAIL_V_MSG(false, "Client not included in this build!");
#endif
			break;
		default:
			ERR_FAIL_V_MSG(false, "Not allowed type!");
			break;
	}

	if (d) {
		device = d;
		SceneTree::get_singleton()->get_root()->call_deferred("add_child", device);
		SceneTree::get_singleton()->get_root()->call_deferred("move_child", device, 0);
		return true;
	}

	return false;
}

bool GodotRemote::start_remote_device() {
	if (device) {
		return device->start();
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

void GodotRemote::register_and_load_settings() {
#define DEF_SET(var, name, def_val, info) \
	var = GLOBAL_DEF(name, def_val);      \
	ProjectSettings::get_singleton()->set_custom_property_info(name, info)
#define DEF_SET_ENUM(var, type, name, def_val, info) \
	var = (type)(int)GLOBAL_DEF(name, def_val);      \
	ProjectSettings::get_singleton()->set_custom_property_info(name, info)
#define DEF_(name, def_val, info) \
	GLOBAL_DEF(name, def_val);    \
	ProjectSettings::get_singleton()->set_custom_property_info(name, info)

	DEF_SET(is_autostart, ps_autoload_name, true, PropertyInfo(Variant::BOOL, ps_autoload_name));
	DEF_SET(port, ps_port_name, port, PropertyInfo(Variant::INT, ps_port_name, PROPERTY_HINT_RANGE, "0,65535"));
	DEF_(ps_jpg_mb_size_name, 4, PropertyInfo(Variant::INT, ps_jpg_mb_size_name, PROPERTY_HINT_RANGE, "1,128"));
	DEF_(ps_config_adb_name, true, PropertyInfo(Variant::BOOL, ps_config_adb_name));

#undef DEF_SET
#undef DEF_
}

void GodotRemote::create_and_start_device(DeviceType type) {
	create_remote_device(type);
	start_remote_device();
}

#ifdef TOOLS_ENABLED
// TODO need to try get every device IDs and setup forwarding for each
#include "editor/editor_export.h"
#include "editor/editor_run_native.h"
#include "editor/editor_settings.h"

void GodotRemote::_adb_port_forwarding() {
	List<String> args;
	args.push_back("reverse");
	args.push_back("tcp:" + str(port));
	args.push_back("tcp:" + str(port));

	String res;
	Error err = OS::get_singleton()->execute(EditorSettings::get_singleton()->get_setting("export/android/adb"), args, true, nullptr, &res, nullptr, true);
	if (err == OK) {
		_log("ADB result: \n" + res);
	}
}

void GodotRemote::_native_run_emitted() {
	// call_deferred because debugger can't connect to game if process blocks thread on start
	if (GET_PS(ps_config_adb_name))
		call_deferred("adb_port_forwarding");
}
#endif

//if (Engine::get_singleton()->is_editor_hint()) {
//	if (EditorExport::get_singleton()) {
//		int c = EditorExport::get_singleton()->get_export_platform_count();
//		for (int i = 0; i < c; i++) {
//			auto p = EditorExport::get_singleton()->get_export_platform(i);
//			_log(p->get_os_name());
//			int c2 = p->get_options_count();
//			for (int i = 0; i < c2; i++) {
//				_log(p->get_option_label(i));
//			}
//		}
//	}
//}
