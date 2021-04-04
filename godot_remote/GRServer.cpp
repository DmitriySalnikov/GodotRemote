/* GRServer.cpp */

#ifndef NO_GODOTREMOTE_SERVER

#include "GRServer.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY

#include "core/input_map.h"
#include "core/io/pck_packer.h"
#include "core/io/resource_loader.h"
#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/os/input_event.h"
#include "core/os/thread_safe.h"
#include "main/input_default.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"

#else

#include <Input.hpp>
#include <InputEvent.hpp>
#include <InputMap.hpp>
#include <PCKPacker.hpp>
#include <ResourceLoader.hpp>
#include <SceneTree.hpp>
#include <ViewportTexture.hpp>

using namespace godot;
#endif

#if !(defined(GODOT_REMOTE_USE_SSE2) && (defined(_M_AMD64) || defined(_M_X64) || _M_IX86_FP == 2))
#define STREAM_SIZE_STEP 4
#else
#define STREAM_SIZE_STEP 32
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY

void GRServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_load_settings), "force_hide_notifications"), &GRServer::_load_settings, DEFVAL(false));
	ClassDB::bind_method(D_METHOD(NAMEOF(_thread_listen), "user_data"), &GRServer::_thread_listen);
	ClassDB::bind_method(D_METHOD(NAMEOF(_thread_connection), "user_data"), &GRServer::_thread_connection);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_gr_viewport)), &GRServer::get_gr_viewport);
	ClassDB::bind_method(D_METHOD(NAMEOF(force_update_custom_input_scene)), &GRServer::force_update_custom_input_scene);

	ClassDB::bind_method(D_METHOD(NAMEOF(set_video_stream_enabled), "value"), &GRServer::set_video_stream_enabled);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_skip_frames), "frames"), &GRServer::set_skip_frames);
	//ClassDB::bind_method(D_METHOD(NAMEOF(set_auto_adjust_scale)), &GRServer::set_auto_adjust_scale);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_stream_quality), "quality"), &GRServer::set_stream_quality);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_render_scale), "scale"), &GRServer::set_render_scale);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_password), "password"), &GRServer::set_password);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_target_fps), "fps"), &GRServer::set_target_fps);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_custom_input_scene), "_scn"), &GRServer::set_custom_input_scene);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_custom_input_scene_compressed), "_is_compressed"), &GRServer::set_custom_input_scene_compressed);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_custom_input_scene_compression_type), "_type"), &GRServer::set_custom_input_scene_compression_type);

	ClassDB::bind_method(D_METHOD(NAMEOF(set_encoder_threads_count), "count"), &GRServer::set_encoder_threads_count);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_encoder_threads_count)), &GRServer::get_encoder_threads_count);

	ClassDB::bind_method(D_METHOD(NAMEOF(is_video_stream_enabled)), &GRServer::is_video_stream_enabled);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_skip_frames)), &GRServer::get_skip_frames);
	//ClassDB::bind_method(D_METHOD(NAMEOF(is_auto_adjust_scale)), &GRServer::is_auto_adjust_scale);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_stream_quality)), &GRServer::get_stream_quality);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_render_scale)), &GRServer::get_render_scale);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_password)), &GRServer::get_password);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_target_fps)), &GRServer::get_target_fps);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_custom_input_scene)), &GRServer::get_custom_input_scene);
	ClassDB::bind_method(D_METHOD(NAMEOF(is_custom_input_scene_compressed)), &GRServer::is_custom_input_scene_compressed);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_custom_input_scene_compression_type)), &GRServer::get_custom_input_scene_compression_type);

	//ADD_PROPERTY(PropertyInfo(Variant::BOOL, "video_stream_enabled"), "set_video_stream_enabled", "is_video_stream_enabled");
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "skip_frames"), "set_skip_frames", "get_skip_frames");
	////ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_adjust_scale"), "set_auto_adjust_scale", "is_auto_adjust_scale");
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "stream_quality"), "set_stream_quality", "get_stream_quality");
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "render_scale"), "set_render_scale", "get_render_scale");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "custom_input_scene"), "set_custom_input_scene", "get_custom_input_scene");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "custom_input_scene_compressed"), "set_custom_input_scene_compressed", "is_custom_input_scene_compressed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "custom_input_scene_compression_type"), "set_custom_input_scene_compression_type", "get_custom_input_scene_compression_type");

	ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
	ADD_SIGNAL(MethodInfo("client_disconnected", PropertyInfo(Variant::STRING, "device_id")));

	ADD_SIGNAL(MethodInfo("client_viewport_orientation_changed", PropertyInfo(Variant::BOOL, "is_vertical")));
	ADD_SIGNAL(MethodInfo("client_viewport_aspect_ratio_changed", PropertyInfo(Variant::REAL, "stream_aspect_ratio")));
}

#else

void GRServer::_register_methods() {
	METHOD_REG(GRServer, _notification);
	METHOD_REG(GRServer, _load_settings);
	METHOD_REG(GRServer, _thread_listen);
	METHOD_REG(GRServer, _thread_connection);

	METHOD_REG(GRServer, get_gr_viewport);
	METHOD_REG(GRServer, force_update_custom_input_scene);

	METHOD_REG(GRServer, set_video_stream_enabled);
	METHOD_REG(GRServer, set_skip_frames);
	//METHOD_REG(GRServer, set_auto_adjust_scale);
	METHOD_REG(GRServer, set_stream_quality);
	METHOD_REG(GRServer, set_render_scale);
	METHOD_REG(GRServer, set_password);
	METHOD_REG(GRServer, set_target_fps);
	METHOD_REG(GRServer, set_custom_input_scene);
	METHOD_REG(GRServer, set_custom_input_scene_compressed);
	METHOD_REG(GRServer, set_custom_input_scene_compression_type);

	METHOD_REG(GRServer, set_encoder_threads_count);
	METHOD_REG(GRServer, get_encoder_threads_count);

	METHOD_REG(GRServer, is_video_stream_enabled);
	METHOD_REG(GRServer, get_skip_frames);
	//METHOD_REG(GRServer, is_auto_adjust_scale);
	METHOD_REG(GRServer, get_stream_quality);
	METHOD_REG(GRServer, get_render_scale);
	METHOD_REG(GRServer, get_password);
	METHOD_REG(GRServer, get_target_fps);
	METHOD_REG(GRServer, get_custom_input_scene);
	METHOD_REG(GRServer, is_custom_input_scene_compressed);
	METHOD_REG(GRServer, get_custom_input_scene_compression_type);

	//register_property<GRServer, bool>("video_stream_enabled", &GRServer::set_video_stream_enabled, &GRServer::is_video_stream_enabled, true);
	//register_property<GRServer, int>("skip_frames", &GRServer::set_skip_frames, &GRServer::get_skip_frames, 0);
	////register_property<GRServer, bool>("auto_adjust_scale", &GRServer::set_auto_adjust_scale, &GRServer::is_auto_adjust_scale, true);
	//register_property<GRServer, int>("stream_quality", &GRServer::set_stream_quality, &GRServer::get_stream_quality, 80);
	//register_property<GRServer, int>("render_scale", &GRServer::set_render_scale, &GRServer::get_render_scale, 0.3f);
	register_property<GRServer, String>("password", &GRServer::set_password, &GRServer::get_password, "");
	register_property<GRServer, String>("custom_input_scene", &GRServer::set_custom_input_scene, &GRServer::get_custom_input_scene, "");
	register_property<GRServer, bool>("custom_input_scene_compressed", &GRServer::set_custom_input_scene_compressed, &GRServer::is_custom_input_scene_compressed, true);
	register_property<GRServer, int>("custom_input_scene_compression_type", &GRServer::set_custom_input_scene_compression_type, &GRServer::get_custom_input_scene_compression_type, 0);

	register_signal<GRServer>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
	register_signal<GRServer>("client_disconnected", "device_id", GODOT_VARIANT_TYPE_STRING);

	register_signal<GRServer>("client_viewport_orientation_changed", "is_vertical", GODOT_VARIANT_TYPE_BOOL);
	register_signal<GRServer>("client_viewport_aspect_ratio_changed", "stream_aspect_ratio", GODOT_VARIANT_TYPE_REAL);
}

#endif

void GRServer::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE: {
			_deinit();
			GRDevice::_deinit();
			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			if (get_status() == (int)WorkingStatus::STATUS_WORKING) {
				_internal_call_only_deffered_stop();
			}
			break;
		}
		case NOTIFICATION_PROCESS: {
			FrameMark;
			break;
		}
	}
}

void GRServer::set_auto_adjust_scale(bool _val) {
	auto_adjust_scale = _val;
}

bool GRServer::is_auto_adjust_scale() {
	return auto_adjust_scale;
}

bool GRServer::set_video_stream_enabled(bool val) {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion() &&
			resize_viewport->is_video_stream_enabled() != val) {
		resize_viewport->set_video_stream_enabled(val);
		return true;
	}
	return false;
}

bool GRServer::is_video_stream_enabled() {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion())
		return resize_viewport->is_video_stream_enabled();
	return false;
}

bool GRServer::set_compression_type(ImageCompressionType _type) {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion() &&
			resize_viewport->get_compression_type() != _type) {
		resize_viewport->set_compression_type(_type);
		return true;
	}
	return false;
}

GRDevice::ImageCompressionType GRServer::get_compression_type() {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion())
		return resize_viewport->get_compression_type();
	return ImageCompressionType::COMPRESSION_UNCOMPRESSED;
}

bool GRServer::set_render_scale(float _scale) {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion() &&
			resize_viewport->get_rendering_scale() != _scale) {
		resize_viewport->set_rendering_scale(_scale);
		return true;
	}
	return false;
}

float GRServer::get_render_scale() {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion())
		return resize_viewport->get_rendering_scale();
	return -1;
}

bool GRServer::set_skip_frames(int fps) {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion() &&
			resize_viewport->get_skip_frames() != fps) {
		resize_viewport->set_skip_frames(fps);
		return true;
	}
	return false;
}

int GRServer::get_skip_frames() {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion())
		return resize_viewport->get_skip_frames();
	return -1;
}

bool GRServer::set_encoder_threads_count(int count) {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion() &&
			resize_viewport->get_encoder_threads_count() != count) {
		resize_viewport->set_encoder_threads_count(count);
		return true;
	}
	return false;
}

int GRServer::get_encoder_threads_count() {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion())
		return resize_viewport->get_encoder_threads_count();
	return GET_PS(GodotRemote::ps_server_image_encoder_threads_count_name);
}

bool GRServer::set_stream_quality(int quality) {
	ERR_FAIL_COND_V(quality < 0 || quality > 100, false);
	if (resize_viewport && !resize_viewport->is_queued_for_deletion() &&
			resize_viewport->get_stream_quality() != quality) {
		resize_viewport->set_stream_quality(quality);
		return true;
	}
	return false;
}

int GRServer::get_stream_quality() {
	if (resize_viewport && !resize_viewport->is_queued_for_deletion())
		return resize_viewport->get_stream_quality();
	return -1;
}

void GRServer::set_password(String _pass) {
	password = _pass;
}

String GRServer::get_password() {
	return password;
}

void GRServer::set_custom_input_scene(String _scn) {
	if (custom_input_scene != _scn) {
		custom_input_scene = _scn;
		force_update_custom_input_scene();
	}
}

String GRServer::get_custom_input_scene() {
	return custom_input_scene;
}

void GRServer::set_custom_input_scene_compressed(bool _is_compressed) {
	custom_input_pck_compressed = _is_compressed;
}

bool GRServer::is_custom_input_scene_compressed() {
	return custom_input_pck_compressed;
}

void GRServer::set_custom_input_scene_compression_type(int _type) {
	custom_input_pck_compression_type = ENUM_CONV(Compression::Mode) _type;
}

int GRServer::get_custom_input_scene_compression_type() {
	return custom_input_pck_compression_type;
}

bool GRServer::set_target_fps(int value) {
	if (target_fps != value) {
		target_fps = value;
		if (client_connected) {
			Engine::get_singleton()->set_target_fps(value);
		}
		return true;
	}
	return false;
}

int GRServer::get_target_fps() {
	return target_fps;
}

void GRServer::_init() {
	set_name("GodotRemoteServer");
	LEAVE_IF_EDITOR();
	TracyAppInfo("Godot Remote Server", 20);
	set_process(true);

	tcp_server.instance();

#ifndef GDNATIVE_LIBRARY
#else
	GRDevice::_init();
#endif

	custom_input_scene_regex_resource_finder.instance();
	custom_input_scene_regex_resource_finder->compile(custom_input_scene_regex_resource_finder_pattern);
}

void GRServer::_deinit() {
	LEAVE_IF_EDITOR();
	if (get_status() == (int)WorkingStatus::STATUS_WORKING) {
		_internal_call_only_deffered_stop();
	}
}

void GRServer::_internal_call_only_deffered_start() {
	ZoneScopedNC("Client Start", tracy::Color::Green3);
	switch ((WorkingStatus)get_status()) {
		case WorkingStatus::STATUS_WORKING:
			ERR_FAIL_MSG("Can't start already working GodotRemote Server");
		case WorkingStatus::STATUS_STARTING:
			ERR_FAIL_MSG("Can't start already starting GodotRemote Server");
		case WorkingStatus::STATUS_STOPPING:
			ERR_FAIL_MSG("Can't start stopping GodotRemote Server");
	}

	set_status(WorkingStatus::STATUS_STARTING);
	_log("Starting GodotRemote server. Version: " + str(GodotRemote::get_singleton()->get_version()), LogLevel::LL_NORMAL);

	if (server_thread_listen) {
		server_thread_listen->close_thread();
		memdelete(server_thread_listen);
		server_thread_listen = nullptr;
	}

	if (!resize_viewport) {
		resize_viewport = memnew(GRSViewport);
		resize_viewport->server = this;
		add_child(resize_viewport);
	}

	server_thread_listen = memnew(ListenerThreadParamsServer);
	Thread_start(server_thread_listen->thread_ref, this, _thread_listen, server_thread_listen);

	set_status(WorkingStatus::STATUS_WORKING);
	call_deferred(NAMEOF(_load_settings), true);

	GRNotifications::add_notification_or_update_line("Godot Remote Server", "1status", "Server started", GRNotifications::GRNotifications::NotificationIcon::ICON_SUCCESS, 1.f);
}

void GRServer::_internal_call_only_deffered_stop() {
	ZoneScopedNC("Client Stop", tracy::Color::Red3);
	switch ((WorkingStatus)get_status()) {
		case WorkingStatus::STATUS_STOPPED:
			ERR_FAIL_MSG("Can't stop already stopped GodotRemote Server");
		case WorkingStatus::STATUS_STOPPING:
			ERR_FAIL_MSG("Can't stop already stopping GodotRemote Server");
		case WorkingStatus::STATUS_STARTING:
			ERR_FAIL_MSG("Can't stop starting GodotRemote Server");
	}

	set_status(WorkingStatus::STATUS_STOPPING);
	_log("Stopping GodotRemote server", LogLevel::LL_NORMAL);

	if (server_thread_listen) {
		server_thread_listen->close_thread();
		memdelete(server_thread_listen);
		server_thread_listen = nullptr;
	}

	if (resize_viewport)
		resize_viewport->_stop_encoder();
	resize_viewport = nullptr;

	_send_queue_resize(0);

	set_status(WorkingStatus::STATUS_STOPPED);

	GRNotifications::add_notification_or_update_line("Godot Remote Server", "1status", "Server stopped", GRNotifications::NotificationIcon::ICON_FAIL, 1.f);
}

GRSViewport *GRServer::get_gr_viewport() {
	return resize_viewport;
}

void GRServer::force_update_custom_input_scene() {
	custom_input_scene_was_updated = false;
}

void GRServer::_adjust_viewport_scale() {
	if (!resize_viewport)
		return;

	const float smooth = 0.8f;
	float scale = resize_viewport->auto_scale;

	if (!auto_adjust_scale) {
		//if (scale == -1.f)
		//	return;

		scale = -1.f;
		goto end;
	}

	if (prev_avg_fps == 0) {
		prev_avg_fps = fps_counter.get_avg();
	} else {
		prev_avg_fps = (prev_avg_fps * smooth) + (fps_counter.get_avg() * (1.f - smooth));
	}
	_log(prev_avg_fps, LogLevel::LL_NORMAL);

	if (prev_avg_fps < resize_viewport->get_skip_frames() - 4) {
		scale -= 0.001f;
	} else if (prev_avg_fps > resize_viewport->get_skip_frames() - 1) {
		scale += 0.0012f;
	}

	if (scale < 0.1f)
		scale = 0.1f;
	if (scale > 1)
		scale = 1;

end:
	resize_viewport->auto_scale = scale;
	prev_avg_fps = 0;
	resize_viewport->call_deferred(NAMEOF(_update_size));
}

void GRServer::_load_settings(bool force_hide_notifications) {
#define IS_NOTIF_SHOWING if (!force_hide_notifications)

	using_client_settings = false;

	const String title = "Server settings updated";
	IS_NOTIF_SHOWING GRNotifications::add_notification_or_update_line(title, "title", "Loaded default server settings", GRNotifications::NotificationIcon::ICON_NONE, 1.f);

	// only updated by server itself
	password = GET_PS(GodotRemote::ps_server_password_name);
	set_custom_input_scene(GET_PS(GodotRemote::ps_server_custom_input_scene_name));
#ifndef GDNATIVE_LIBRARY
	set_custom_input_scene_compressed(GET_PS(GodotRemote::ps_server_custom_input_scene_compressed_name));
	set_custom_input_scene_compression_type(ENUM_CONV(Compression::Mode)(int) GET_PS(GodotRemote::ps_server_custom_input_scene_compression_type_name));
#endif

	IS_NOTIF_SHOWING GRNotifications::add_notification_or_update_line(title, "cis", "Custom input scene: " + str(get_custom_input_scene()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
	if (!get_custom_input_scene().empty()) {
		IS_NOTIF_SHOWING {
			GRNotifications::add_notification_or_update_line(title, "cisc", "Scene compressed: " + str(is_custom_input_scene_compressed()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
			GRNotifications::add_notification_or_update_line(title, "cisct", "Scene compression type: " + str(get_custom_input_scene_compression_type()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
		}
	}

	// can be updated by client
	auto_adjust_scale = GET_PS(GodotRemote::ps_server_auto_adjust_scale_name); // TODO move to viewport

	IS_NOTIF_SHOWING GRNotifications::add_notification_or_update_line(title, "auto_scale", "Auto adjust scale: " + str(auto_adjust_scale), GRNotifications::NotificationIcon::ICON_NONE, 1.f); // TODO not completed
	if (resize_viewport && !resize_viewport->is_queued_for_deletion()) {
		set_video_stream_enabled((bool)GET_PS(GodotRemote::ps_server_stream_enabled_name));
		set_compression_type((ImageCompressionType)(int)GET_PS(GodotRemote::ps_server_compression_type_name));
		set_stream_quality(GET_PS(GodotRemote::ps_server_stream_quality_name));
		set_render_scale(GET_PS(GodotRemote::ps_server_scale_of_sending_stream_name));
		set_skip_frames(GET_PS(GodotRemote::ps_server_stream_skip_frames_name));
		set_target_fps(GET_PS(GodotRemote::ps_server_target_fps_name));
		set_encoder_threads_count(GET_PS(GodotRemote::ps_server_image_encoder_threads_count_name));

		IS_NOTIF_SHOWING {
			GRNotifications::add_notification_or_update_line(title, "stream", "Stream enabled: " + str(is_video_stream_enabled()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
			GRNotifications::add_notification_or_update_line(title, "compression", "Compression type: " + str(get_compression_type()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
			GRNotifications::add_notification_or_update_line(title, "quality", "Stream Quality: " + str(get_stream_quality()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
			GRNotifications::add_notification_or_update_line(title, "skip", "Skip Frames: " + str(get_skip_frames()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
			GRNotifications::add_notification_or_update_line(title, "scale", "Scale of stream: " + str(get_render_scale()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
			GRNotifications::add_notification_or_update_line(title, "fps", "Target FPS: " + str(get_target_fps()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
			GRNotifications::add_notification_or_update_line(title, "threads", "Encoder Threads: " + str(get_encoder_threads_count()), GRNotifications::NotificationIcon::ICON_NONE, 1.f);
		}
	} else {
		_log("Resize viewport not found!", LogLevel::LL_ERROR);
		GRNotifications::add_notification("Critical Error", "Resize viewport not found!", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
	}

	// clear prev notification?
	IS_NOTIF_SHOWING {
		GRNotificationPanelUpdatable *np = cast_to<GRNotificationPanelUpdatable>(GRNotifications::get_notification(title));
		if (np) {
			np->clear_lines();
		}
	}

#undef IS_NOTIF_SHOWING
}

void GRServer::_update_settings_from_client(const std::map<int, Variant> settings) {
#define SET_BODY(_func, _id, _text, _type)                                                                                                           \
	if (_func(value)) {                                                                                                                              \
		GRNotifications::add_notification_or_update_line(title, _id, _text + str((_type value)), GRNotifications::NotificationIcon::ICON_NONE, 1.f); \
		using_client_settings = true;                                                                                                                \
		using_client_settings_recently_updated = true;                                                                                               \
	}
#define SET_BODY_CONVERT(_func, _conv, _id, _text, _type)                                                                                            \
	if (_func(_conv(value))) {                                                                                                                       \
		GRNotifications::add_notification_or_update_line(title, _id, _text + str((_type value)), GRNotifications::NotificationIcon::ICON_NONE, 1.f); \
		using_client_settings = true;                                                                                                                \
		using_client_settings_recently_updated = true;                                                                                               \
	}

	const String title = "Server settings updated";

	GRNotificationPanelUpdatable *np = cast_to<GRNotificationPanelUpdatable>(GRNotifications::get_notification(title));
	if (np) {
		np->remove_updatable_line("title");
	}

	if (!resize_viewport || resize_viewport->is_queued_for_deletion()) {
		_log("Resize viewport not found!", LogLevel::LL_ERROR);
		GRNotifications::add_notification("Critical Error", "Resize viewport not found!", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
	}

	for (auto p : settings) {
		Variant key = p.first;
		Variant value = p.second;
		_log("Trying to set server setting from client with key: " + str((int)key) + " and value: " + str(value), LogLevel::LL_DEBUG);

		if (key.get_type() == Variant::INT) {
			TypesOfServerSettings k = (TypesOfServerSettings)(int)key;
			switch (k) {
				case TypesOfServerSettings::SERVER_SETTINGS_USE_INTERNAL:
					if ((bool)value) {
						call_deferred(NAMEOF(_load_settings));
						return;
					}
					break;
				case TypesOfServerSettings::SERVER_SETTINGS_VIDEO_STREAM_ENABLED: {
					SET_BODY(set_video_stream_enabled, "stream", "Stream enabled: ", (bool));
					break;
				}
				case TypesOfServerSettings::SERVER_SETTINGS_COMPRESSION_TYPE: {
					SET_BODY_CONVERT(set_compression_type, (ImageCompressionType)(int), "compression", "Compression type: ", (int));
					break;
				}
				case TypesOfServerSettings::SERVER_SETTINGS_STREAM_QUALITY: {
					SET_BODY(set_stream_quality, "quality", "Stream Quality: ", (int));
					break;
				}
				case TypesOfServerSettings::SERVER_SETTINGS_SKIP_FRAMES: {
					SET_BODY(set_skip_frames, "skip", "Skip Frames: ", (int));
					break;
				}
				case TypesOfServerSettings::SERVER_SETTINGS_RENDER_SCALE: {
					SET_BODY(set_render_scale, "scale", "Scale of stream: ", (float));
					break;
				}
				case TypesOfServerSettings::SERVER_SETTINGS_TARGET_FPS: {
					SET_BODY(set_target_fps, "fps", "Target FPS: ", (int));
					break;
				}
				case TypesOfServerSettings::SERVER_SETTINGS_THREADS_NUMBER: {
					SET_BODY(set_encoder_threads_count, "threads", "Encoder Threads: ", (int));
					break;
				}
				default:
					_log("Unknown server setting with code: " + str((int)k), LogLevel::LL_NORMAL);
					break;
			}
		}
	}

#undef SET_BODY
#undef SET_BODY_CONVERT
}

void GRServer::_reset_counters() {
	GRDevice::_reset_counters();
	prev_avg_fps = 0;
}

//////////////////////////////////////////////
////////////////// THREAD ////////////////////
//////////////////////////////////////////////

void GRServer::_thread_listen(Variant p_userdata) {
	Thread_set_name("Server Listen for Connections");

	ListenerThreadParamsServer *this_thread_info = VARIANT_OBJ_CAST_TO(p_userdata, ListenerThreadParamsServer);
	OS *os = OS::get_singleton();
	ConnectionThreadParamsServer *connection_thread_info = nullptr;
	Error err = Error::OK;
	bool listening_error_notification_shown = false;

	while (!this_thread_info->stop_thread) {
		ZoneScopedNC("Server Listen For Connections", tracy::Color::Orange2);
		if (!tcp_server->is_listening()) {
			err = tcp_server->listen(port);

			if (err != Error::OK) {
				switch (err) {
					case Error::ERR_UNAVAILABLE: {
						String txt = "Socket listening unavailable";
						if (!listening_error_notification_shown) {
							_log(txt, LogLevel::LL_ERROR);
							GRNotifications::add_notification("Can't start listening", txt, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.25f);
						}
						break;
					}
					case Error::ERR_ALREADY_IN_USE: {
						String txt = "Socket already in use";
						if (!listening_error_notification_shown) {
							_log(txt, LogLevel::LL_ERROR);
							GRNotifications::add_notification("Can't start listening", txt, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.25f);
						}
						break;
					}
					case Error::ERR_INVALID_PARAMETER: {
						String txt = "Invalid listening address";
						if (!listening_error_notification_shown) {
							_log(txt, LogLevel::LL_ERROR);
							GRNotifications::add_notification("Can't start listening", txt, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.25f);
						}
						break;
					}
					case Error::ERR_CANT_CREATE: {
						String txt = "Can't bind listener";
						if (!listening_error_notification_shown) {
							_log(txt, LogLevel::LL_ERROR);
							GRNotifications::add_notification("Can't start listening", txt, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.25f);
						}
						break;
					}
					case Error::FAILED: {
						String txt = "Failed to start listening";
						if (!listening_error_notification_shown) {
							_log(txt, LogLevel::LL_ERROR);
							GRNotifications::add_notification("Can't start listening", txt, GRNotifications::NotificationIcon::ICON_ERROR, true, 1.25f);
						}
						break;
					}
				}

				listening_error_notification_shown = true;
				sleep_usec(1000_ms);
				continue;
			} else {
				_log("Start listening port " + str(port), LogLevel::LL_NORMAL);
				GRNotifications::add_notification_or_update_line("Godot Remote Server", "2listening", "Start listening on port: " + str(port), GRNotifications::NotificationIcon::ICON_SUCCESS, 1.f);
			}
		}
		listening_error_notification_shown = false;

		if (connection_thread_info) {
			if (connection_thread_info->ppeer.is_null()) {
				connection_thread_info->break_connection = true;
			}

			if (connection_thread_info->finished || connection_thread_info->break_connection) {
				_log("Waiting connection thread...", LogLevel::LL_DEBUG);
				connection_thread_info->close_thread();
				memdelete(connection_thread_info);
				connection_thread_info = nullptr;
			}
		}

		if (tcp_server->is_connection_available()) {
			Ref<StreamPeerTCP> con = tcp_server->take_connection();
			con->set_no_delay(true);
			String address = CONNECTION_ADDRESS(con);

			Ref<PacketPeerStream> ppeer = newref(PacketPeerStream);
			ppeer->set_stream_peer(con);
			ppeer->set_output_buffer_max_size((1024 * 1024) * ((int)GET_PS(GodotRemote::ps_server_jpg_buffer_mb_size_name)));

			if (!connection_thread_info) {
				Dictionary ret_data;
				GRDevice::AuthResult res = _auth_client(ppeer, ret_data, false);
				String dev_id = "";

				if (ret_data.has("id")) {
					dev_id = ret_data["id"];
				}

				switch (res) {
					case GRDevice::AuthResult::OK:
						connection_thread_info = memnew(ConnectionThreadParamsServer);
						connection_thread_info->device_id = dev_id;
						connection_thread_info->ppeer = ppeer;

						custom_input_scene_was_updated = false;
						client_connected++;

						Thread_start(connection_thread_info->thread_ref, this, _thread_connection, connection_thread_info);
						_log("New connection from " + address, LogLevel::LL_NORMAL);

						call_deferred("emit_signal", "client_connected", dev_id);
						GRNotifications::add_notification("Connected", "Client connected: " + address + "\nDevice ID: " + connection_thread_info->device_id, GRNotifications::NotificationIcon::ICON_SUCCESS, true, 1.f);
						break;

					case GRDevice::AuthResult::VersionMismatch:
					case GRDevice::AuthResult::IncorrectPassword:
					case GRDevice::AuthResult::Error:
					case GRDevice::AuthResult::RefuseConnection:
					case GRDevice::AuthResult::Timeout:
						continue;
					default:
						_log("Unknown error code. Disconnecting. " + address, LogLevel::LL_NORMAL);
						continue;
				}

			} else {
				Dictionary ret_data;
				GRDevice::AuthResult res = _auth_client(ppeer, ret_data, true);
			}
		} else {
			_log("Waiting...", LogLevel::LL_DEBUG);
			sleep_usec(50_ms);
		}
	}

	if (connection_thread_info) {
		_log("Closing connection thread...", LogLevel::LL_DEBUG);
		connection_thread_info->break_connection = true;
		connection_thread_info->close_thread();
		memdelete(connection_thread_info);
		connection_thread_info = nullptr;
	}

	tcp_server->stop();
	this_thread_info->finished = true;
}

void GRServer::_thread_connection(Variant p_userdata) {
	ConnectionThreadParamsServer *thread_info = VARIANT_OBJ_CAST_TO(p_userdata, ConnectionThreadParamsServer);
	Ref<StreamPeerTCP> connection = thread_info->ppeer->get_stream_peer();
	Ref<PacketPeerStream> ppeer = thread_info->ppeer;
	_reset_counters();
	_send_queue_resize(0);

	GodotRemote *gr = GodotRemote::get_singleton();
	Input *input = Input::get_singleton();
	Error err = Error::OK;

	// Store prev target fps of game
	int64_t default_target_fps = Engine::get_singleton()->get_target_fps();
	Engine::get_singleton()->set_target_fps(target_fps);

	Input::MouseMode mouse_mode = Input::MOUSE_MODE_VISIBLE;
	String address = CONNECTION_ADDRESS(connection);
	Thread_set_name(("Server Connection: " + address).ascii().get_data());

	uint64_t time64 = get_time_usec();
	uint64_t prev_send_settings_time = time64;
	uint64_t prev_send_image_time = time64;
	uint64_t prev_ping_sending_time = time64;
	uint64_t prev_process_image_time = time64;
	//uint64_t prev_send_sync_time = time64;

	bool ping_sended = false;
	bool time_synced = false;

	while (!thread_info->break_connection && connection.is_valid() &&
			!connection->is_queued_for_deletion() && connection->is_connected_to_host()) {
		ZoneScopedNC("Server Loop", tracy::Color::OrangeRed4);

		bool nothing_happens = true;
		float fps = Engine::get_singleton()->get_frames_per_second();
		if (fps == 0) {
			fps = 1;
		}

		if (resize_viewport && !resize_viewport->is_queued_for_deletion()) {
			if (resize_viewport->get_skip_frames())
				fps = fps / resize_viewport->get_skip_frames();

			if (!resize_viewport->is_processing()) {
				resize_viewport->_start_encoder();
				resize_viewport->force_get_image();
			}
		}

		uint64_t send_data_time_us = uint64_t(1000000.0 / fps);

		///////////////////////////////////////////////////////////////////
		// SENDING
		uint64_t start_while_time = get_time_usec();

		// TIME SYNC
		//time = os->get_ticks_usec();
		//if (time - prev_send_sync_time > 1000_ms) {
		if (!time_synced) {
			ZoneScopedNC("Send Sync Time", tracy::Color::Bisque);
			time_synced = true;
			nothing_happens = false;
			//prev_send_sync_time = time;
			auto pack = shared_new(GRPacketSyncTime);
			err = ppeer->put_var(pack->get_data());
			if ((int)err) {
				_log("Can't send sync time data! Code: " + str((int)err), LogLevel::LL_ERROR);
				goto end_send;
			}
		}

		// IMAGE
		time64 = get_time_usec();
		// if image compressed and data is ready
		if (resize_viewport && !resize_viewport->is_queued_for_deletion() &&
				resize_viewport->has_data_to_send()) {
			ZoneScopedNC("Send Buffered Image Data", tracy::Color::Gray71);
			nothing_happens = false;

			std::shared_ptr<GRPacket> pack = resize_viewport->pop_data_to_send();
			if (pack) {
				// avg fps
				_update_avg_fps(time64 - prev_send_image_time);
				_adjust_viewport_scale();
				prev_send_image_time = time64;

				err = ppeer->put_var(pack->get_data());

				if ((int)err) {
					_log("Can't send image data! Code: " + str((int)err), LogLevel::LL_ERROR);
					goto end_send;
				}
			} else {
				_log("Got empty image data from the encoder", LogLevel::LL_ERROR);
			}
		} else {
			if (!is_video_stream_enabled()) {
				_update_avg_fps(0);
			}
		}

		// SERVER SETTINGS
		time64 = get_time_usec();
		if (time64 - prev_send_settings_time > 1000_ms && using_client_settings) {
			prev_send_settings_time = time64;
			if (using_client_settings_recently_updated) {
				using_client_settings_recently_updated = false;
			} else {
				ZoneScopedNC("Send server settings", tracy::Color::Yellow4);
				nothing_happens = false;

				auto pack = shared_new(GRPacketServerSettings);
				pack->add_setting((int)TypesOfServerSettings::SERVER_SETTINGS_VIDEO_STREAM_ENABLED, is_video_stream_enabled());
				pack->add_setting((int)TypesOfServerSettings::SERVER_SETTINGS_COMPRESSION_TYPE, get_compression_type());
				pack->add_setting((int)TypesOfServerSettings::SERVER_SETTINGS_STREAM_QUALITY, get_stream_quality());
				pack->add_setting((int)TypesOfServerSettings::SERVER_SETTINGS_RENDER_SCALE, get_render_scale());
				pack->add_setting((int)TypesOfServerSettings::SERVER_SETTINGS_SKIP_FRAMES, get_skip_frames());

				err = ppeer->put_var(pack->get_data());
				if ((int)err) {
					_log("Send server settings failed with code: " + str((int)err), LogLevel::LL_ERROR);
					goto end_send;
				}
			}
		}

		// MOUSE MODE
		if (input->get_mouse_mode() != mouse_mode) {
			ZoneScopedNC("Send Mouse Mode Sync Data", tracy::Color::CadetBlue1);
			nothing_happens = false;
			mouse_mode = input->get_mouse_mode();

			auto pack = shared_new(GRPacketMouseModeSync);
			pack->set_mouse_mode(mouse_mode);

			err = ppeer->put_var(pack->get_data());
			if ((int)err) {
				_log("Send mouse mode sync failed with code: " + str((int)err), LogLevel::LL_ERROR);
				goto end_send;
			}
		}

		// PING
		time64 = get_time_usec();
		if ((time64 - prev_ping_sending_time) > 100_ms && !ping_sended) {
			ZoneScopedNC("Send Ping", tracy::Color::CadetBlue);
			nothing_happens = false;
			ping_sended = true;

			auto pack = shared_new(GRPacketPing);
			err = ppeer->put_var(pack->get_data());

			prev_ping_sending_time = time64;
			if ((int)err) {
				_log("Send ping failed with code: " + str((int)err), LogLevel::LL_ERROR);
				goto end_send;
			}
		}

		// CUSTOM INPUT SCENE
		if (!custom_input_scene_was_updated) {
			ZoneScopedNC("Send Custom Input Scene", tracy::Color::Moccasin);
			custom_input_scene_was_updated = true;

			std::shared_ptr<GRPacketCustomInputScene> pack;
			if (!custom_input_scene.empty()) {
				pack = _create_custom_input_pack(custom_input_scene, custom_input_pck_compressed, custom_input_pck_compression_type);
			} else {
				pack = shared_new(GRPacketCustomInputScene);
			}

			err = ppeer->put_var(pack->get_data());

			if ((int)err) {
				_log("Send custom input failed with code: " + str((int)err), LogLevel::LL_ERROR);
				goto end_send;
			}
		}

		// SEND QUEUE
		while (!send_queue.empty() && (get_time_usec() - start_while_time) <= send_data_time_us / 2) {
			ZoneScopedNC("Send Queued Data", tracy::Color::Burlywood1);
			std::shared_ptr<GRPacket> packet = _send_queue_pop_front();

			if (packet) {
				err = ppeer->put_var(packet->get_data());

				if ((int)err) {
					_log("Put data from queue failed with code: " + str((int)err), LogLevel::LL_ERROR);
					goto end_send;
				}
			}
		}

	end_send:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after sending!", LogLevel::LL_ERROR);
			GRNotifications::add_notification("Error", "Lost connection after sending data!", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
			continue;
		}

		///////////////////////////////////////////////////////////////////
		// RECEIVING
		{
			ZoneScopedNC("Receive Data", tracy::Color::IndianRed1);
			uint64_t recv_start_time = get_time_usec();
			while (connection->is_connected_to_host() && ppeer->get_available_packet_count() > 0 &&
					(get_time_usec() - recv_start_time) < send_data_time_us / 2) {
				ZoneScopedNC("Get Available Packet", tracy::Color::MintCream);
				nothing_happens = false;
				Variant res;
#ifndef GDNATIVE_LIBRARY
				err = (Error)(int)ppeer->get_var(res);
#else
				err = Error::OK;
				res = ppeer->get_var();
#endif

				if ((int)err) {
					_log("Can't receive packet!", LogLevel::LL_ERROR);
					continue;
				}

				std::shared_ptr<GRPacket> pack = GRPacket::create(res);
				if (!pack) {
					_log("Received packet was NULL", LogLevel::LL_ERROR);
					continue;
				}

				GRPacket::PacketType type = pack->get_type();

				switch (type) {
					case GRPacket::PacketType::SyncTime: {
						ERR_PRINT("NOT IMPLEMENTED");
						break;
					}
					case GRPacket::PacketType::InputData: {
						shared_cast_def(GRPacketInputData, data, pack);
						if (!data) {
							_log("Incorrect GRPacketInputData", LogLevel::LL_ERROR);
							break;
						}
						ZoneScopedNC("Parce Input Data", tracy::Color::MediumOrchid3);

						for (int i = 0; i < data->get_inputs_count(); i++) {
							std::shared_ptr<GRInputData> id = data->get_input_data(i);
							GRInputData::InputType ev_type = id->get_type();

							if (ev_type >= GRInputData::InputType::_InputEvent) {
								shared_cast_def(GRInputDataEvent, ied, id);
								if (!ied) {
									_log("GRInputDataEvent is null", LogLevel::LL_ERROR);
									continue;
								}

								Ref<InputEvent> ev = ied->construct_event();
								if (ev.is_valid()) {
									Input::get_singleton()->call_deferred("parse_input_event", ev);
								}
							} else {
								switch (ev_type) {
									case GRInputData::InputType::_NoneIT: {
										_log("Not valid input type! 0", LogLevel::LL_NORMAL);
										break;
									}
									case GRInputData::InputType::_InputDeviceSensors: {
										shared_cast_def(GRInputDeviceSensorsData, sd, id);
										if (!sd) {
											_log("GRInputDeviceSensorsData is null", LogLevel::LL_ERROR);
											continue;
										}

										auto s = sd->get_sensors();
										set_accelerometer(s[0]);
										set_gravity(s[1]);
										set_gyroscope(s[2]);
										set_magnetometer(s[3]);

										break;
									}
									default: {
										_log("Not supported input type! " + str((int)ev_type), LogLevel::LL_ERROR);
										continue;
										break;
									}
								}
							}
						}
						break;
					}
					case GRPacket::PacketType::ServerSettings: {
						shared_cast_def(GRPacketServerSettings, data, pack);
						if (!data) {
							_log("Incorrect GRPacketServerSettings", LogLevel::LL_ERROR);
							continue;
						}
						_update_settings_from_client(data->get_settings());
						break;
					}
					case GRPacket::PacketType::ClientStreamOrientation: {
						shared_cast_def(GRPacketClientStreamOrientation, data, pack);
						if (!data) {
							_log("Incorrect GRPacketClientStreamOrientation", LogLevel::LL_ERROR);
							continue;
						}
						call_deferred("emit_signal", "client_viewport_orientation_changed", data->is_vertical());
						break;
					}
					case GRPacket::PacketType::StreamAspectRatio: {
						shared_cast_def(GRPacketStreamAspectRatio, data, pack);
						if (!data) {
							_log("Incorrect GRPacketStreamAspectRatio", LogLevel::LL_ERROR);
							continue;
						}
						call_deferred("emit_signal", "client_viewport_aspect_ratio_changed", data->get_aspect());
						break;
					}
					case GRPacket::PacketType::CustomUserData: {
						shared_cast_def(GRPacketCustomUserData, data, pack);
						if (!data) {
							_log("Incorrect GRPacketCustomUserData", LogLevel::LL_ERROR);
							continue;
						}
						call_deferred("emit_signal", "user_data_received", data->get_packet_id(), data->get_user_data());
						break;
					}
					case GRPacket::PacketType::Ping: {
						auto pack = shared_new(GRPacketPong);
						err = ppeer->put_var(pack->get_data());

						if ((int)err) {
							_log("Send pong failed with code: " + str((int)err), LogLevel::LL_ERROR);
							goto end_recv;
						}
						break;
					}
					case GRPacket::PacketType::Pong: {
						_update_avg_ping(get_time_usec() - prev_ping_sending_time);
						ping_sended = false;
						break;
					}
					default: {
						_log("Not supported packet type! " + str((int)type), LogLevel::LL_WARNING);
						break;
					}
				}
			}
		}
	end_recv:

		if (!connection->is_connected_to_host()) {
			_log("Lost connection after receiving!", LogLevel::LL_ERROR);
			GRNotifications::add_notification("Error", "Lost connection after receiving data!", GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
			continue;
		}

		if (nothing_happens) // for less cpu using
			sleep_usec(1_ms);
	}

	_log("Closing connection thread with address: " + address, LogLevel::LL_DEBUG);

	if (resize_viewport)
		resize_viewport->_stop_encoder();

	if (connection->is_connected_to_host()) {
		GRNotifications::add_notification("Disconnected", "Closing connection with " + address, GRNotifications::NotificationIcon::ICON_FAIL, false, 1.f);
	} else {
		GRNotifications::add_notification("Disconnected", "Client disconnected: " + address, GRNotifications::NotificationIcon::ICON_FAIL, false, 1.f);
	}

	if (ppeer.is_valid()) {
		ppeer.unref();
	}
	thread_info->ppeer.unref();
	thread_info->break_connection = true;
	client_connected--;

	// Reset target fps
	Engine::get_singleton()->set_target_fps(default_target_fps);
	_send_queue_resize(0);

	//call_deferred(NAMEOF(_load_settings));
	call_deferred("emit_signal", "client_disconnected", thread_info->device_id);

	thread_info->finished = true;
}

GRServer::AuthResult GRServer::_auth_client(Ref<PacketPeerStream> &ppeer, Dictionary &ret_data, bool refuse_connection) {
	// _v - variable definition, _n - dict key, _c - fail condition, _e - error message, _r - return value on fail condition
#define packet_error_check(_t)              \
	if ((int)err) {                         \
		_log(_t, LogLevel::LL_DEBUG);       \
		con->disconnect_from_host();        \
		return GRDevice::AuthResult::Error; \
	}
#define dict_get(_t, _v, _n, _c, _e, _r) \
	_t _v;                               \
	if (dict.has(_n))                    \
		_v = V_CAST(dict[_n], _t);       \
	else                                 \
		goto error_dict;                 \
	if (_c) {                            \
		ppeer->put_var((int)_r);         \
		_log(_e, LogLevel::LL_DEBUG);    \
		con->disconnect_from_host();     \
		return _r;                       \
	}
#define wait_packet(_n)                                                                                       \
	{                                                                                                         \
		ZoneScopedNC("Waiting Auth Packet: " #_n, tracy::Color::DarkCyan);                                    \
		time = (uint32_t)OS::get_singleton()->get_ticks_msec();                                               \
		while (ppeer->get_available_packet_count() == 0) {                                                    \
			if (OS::get_singleton()->get_ticks_msec() - time > 150) {                                         \
				_log("Connection timeout. Refusing " + address + ". Waited: " + str(_n), LogLevel::LL_DEBUG); \
				goto timeout;                                                                                 \
			}                                                                                                 \
			if (!con->is_connected_to_host()) {                                                               \
				return GRDevice::AuthResult::Error;                                                           \
			}                                                                                                 \
			sleep_usec(1_ms);                                                                                 \
		}                                                                                                     \
	}

	ZoneScopedNC("Authorization", tracy::Color::Cyan3);

	Ref<StreamPeerTCP> con = ppeer->get_stream_peer();
	String address = CONNECTION_ADDRESS(con);
	uint32_t time = 0;

	Error err = Error::OK;
	Variant res;
	if (!refuse_connection) {
		// PUT client can try to connect
		err = ppeer->put_var((int)GRDevice::AuthResult::TryToConnect);
		packet_error_check("Can't send authorization init packet to " + address + ". Code: " + str((int)err));

		// GET auth data
		wait_packet("auth_data");

#ifndef GDNATIVE_LIBRARY
		err = (Error)(int)ppeer->get_var(res);
		packet_error_check("Can't get authorization data from client to " + address + ". Code: " + str((int)err));
#else
		err = Error::OK;
		res = ppeer->get_var();
#endif

		Dictionary dict = res;
		if (dict.empty()) {
			goto error_dict;
		} else {
			dict_get(String, id, "id",
					id.empty(), "Device ID field is empty or does not exists. " + address,
					GRDevice::AuthResult::VersionMismatch);

			ret_data["id"] = id;

			dict_get(PoolByteArray, ver, "version",
					ver.size() == 0, "Version field is empty or does not exists. " + address,
					GRDevice::AuthResult::VersionMismatch);

			if (!validate_version(ver)) {
				_log("Version mismatch", LogLevel::LL_ERROR);
				ppeer->put_var((int)GRDevice::AuthResult::VersionMismatch); // just try to send and forget
				return GRDevice::AuthResult::VersionMismatch;
			}

			// TODO thread safe password...
			if (!password.empty()) {
				dict_get(String, tmp_pass, "password",
						tmp_pass != password, "Incorrect password. " + address,
						GRDevice::AuthResult::IncorrectPassword);
			}
		}

		// PUT auth ok
		err = ppeer->put_var((int)GRDevice::AuthResult::OK);
		packet_error_check("Can't send final authorization packet from client to " + address + ". Code: " + str((int)err));

		return GRDevice::AuthResult::OK;

	error_dict:
		_log("Got invalid authorization data from client. " + address, LogLevel::LL_NORMAL);
		err = ppeer->put_var((int)GRDevice::AuthResult::Error);
		packet_error_check("Can't send error code to client " + address + ". Code: " + str((int)err));
		con->disconnect_from_host();
		return GRDevice::AuthResult::Error;

	} else {
		// PUT refuse connection
		Error err = ppeer->put_var((int)GRDevice::AuthResult::RefuseConnection);
		con->disconnect_from_host();
		return GRDevice::AuthResult::RefuseConnection;
	}
timeout:
	con->disconnect_from_host();
	return GRDevice::AuthResult::Timeout;

#undef dict_get
#undef wait_packet
#undef packet_error_check
}

std::shared_ptr<GRPacketCustomInputScene> GRServer::_create_custom_input_pack(String _scene_path, bool compress, ENUM_ARG(Compression::Mode) compression_type) {
	ZoneScopedNC("Create Custom Input Scene Pack", tracy::Color::LimeGreen);

	auto pack = shared_new(GRPacketCustomInputScene);
	std::vector<String> files;
	_scan_resource_for_dependencies_recursive(_scene_path, files);
#ifndef GDNATIVE_LIBRARY
#else
	compress = false;
#endif

	if (files.size()) {
		String pck_file = _scene_path.get_base_dir() + "tmp.pck";
		Ref<PCKPacker> pck = memnew(PCKPacker);
		Error err = pck->pck_start(pck_file);

		if ((int)err) {
			_log("Can't create PCK file. Code: " + str((int)err), LogLevel::LL_ERROR);
		} else {

			Error add_err = Error::OK;
			for (int i = 0; i < files.size(); i++) {
				add_err = pck->add_file(files[i], files[i]);
				if ((int)add_err) {
					_log("Can't add file to PCK. Code: " + str((int)add_err), LogLevel::LL_ERROR);
					break;
				}
			}

			if (!(int)add_err) {
				err = pck->flush();

				if ((int)err) {
					_log("Can't flush PCK file. Code: " + str((int)err), LogLevel::LL_ERROR);
				} else {

					// if OK show which files added
					_log(str(files.size()) + " files added to custom input PCK", LogLevel::LL_NORMAL);
					_log(str_arr(files, true, 0, ",\n"), LogLevel::LL_DEBUG);

#ifndef GDNATIVE_LIBRARY
					FileAccess *file = FileAccess::open(pck_file, FileAccess::ModeFlags::READ, &err);
#else
					File *file = memnew(File);
					err = file->open(pck_file, File::ModeFlags::READ);
#endif

					if ((int)err) {
						_log("Can't open PCK file for reading. Code: " + str((int)err), LogLevel::LL_ERROR);
					} else {
						PoolByteArray arr;
#ifndef GDNATIVE_LIBRARY
						err = arr.resize(file->get_len());
						if ((int)err) {
							_log("Can't resize temp buffer array. Code: " + str((int)err), LogLevel::LL_ERROR);
						} else {
#else
						{
#endif

#ifndef GDNATIVE_LIBRARY
							auto w = arr.write();
							int res = file->get_buffer(w.ptr(), arr.size());
							release_pva_write(w);
							file->close();

							if (res != arr.size()) {
#else
							arr = file->get_buffer(file->get_len());
							int file_length = (int)file->get_len();
							file->close();

							if (file_length != arr.size()) {
								int res = (int)Error::ERR_FILE_CANT_READ;
#endif

								_log("PCK was not fully read. " + str(res) + " of " + str(arr.size()), LogLevel::LL_ERROR);
							} else {

								if (compress) {
									PoolByteArray com;
									err = compress_bytes(arr, com, compression_type);
									if ((int)err) {
										_log("Can't compress PCK data. Code: " + str((int)err), LogLevel::LL_ERROR);
									}

									pack->set_scene_path(_scene_path);
									pack->set_scene_data(com);
									pack->set_compressed(true);
									pack->set_compression_type(compression_type);
									pack->set_original_size(arr.size());
								} else {
									pack->set_scene_path(_scene_path);
									pack->set_scene_data(arr);
									pack->set_compressed(false);
									pack->set_compression_type(0);
								}

#ifndef GDNATIVE_LIBRARY
								DirAccess *dir = DirAccess::open(_scene_path.get_base_dir());
								if (dir) {
									dir->remove(pck_file);
									memdelete(dir);
								}
#else
								Directory *dir = memnew(Directory);
								if (dir) {
									dir->open(_scene_path.get_base_dir());
									dir->remove(pck_file);
									memdelete(dir);
								}
#endif
							}
						}
					}
					memdelete(file);
				}
			}
		}
	} else {
		_log("Files to pack not found! Scene path: " + _scene_path, LogLevel::LL_ERROR);
	}

	files.clear();
	return pack;
}

void GRServer::_scan_resource_for_dependencies_recursive(String _d, std::vector<String> &_arr) {
	if (!is_vector_contains(_arr, _d)) {
		_arr.push_back(_d);
	} else {
		return;
	}

	Error err = Error::OK;

	String text = file_get_as_string(_d, &err);

	if ((int)err) {
		_log("Can't read file as text: " + _d, LogLevel::LL_ERROR);
	} else {
		String imp = _d + ".import";
		text += file_get_as_string(imp, &err);
		if ((int)err) {
			_log(".import file not found for " + imp, LogLevel::LL_DEBUG);
		} else {
			if (!is_vector_contains(_arr, imp)) {
				_arr.push_back(imp);
			}
		}
		// Check for .md5 files in .import folder
		String imp_folder = "res://.import/"; // workaround for GDNative
		if (_d.begins_with(imp_folder)) {
			String md5 = _d.get_basename() + ".md5";
			text += file_get_as_string(md5, &err);
			// Not error == OK
			if (!(int)err) {
				if (!is_vector_contains(_arr, md5)) {
					_arr.push_back(md5);
				}
			}
		}

		Array res = custom_input_scene_regex_resource_finder->search_all(text);

		for (int i = 0; i < res.size(); i++) {
			Ref<RegExMatch> rem = res[i];
			String path = rem->get_string(1);
			path = path.trim_suffix("\\"); // Needed for avoiding escape symbols in build-in scripts

			_scan_resource_for_dependencies_recursive(path, _arr);
		}
	}
}

//////////////////////////////////////////////
////////////// GRSViewport ///////////////////
//////////////////////////////////////////////

#ifndef GDNATIVE_LIBRARY

void GRSViewport::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_update_size)), &GRSViewport::_update_size);
	ClassDB::bind_method(D_METHOD(NAMEOF(_draw_main_vp)), &GRSViewport::_draw_main_vp);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_rendering_scale)), &GRSViewport::set_rendering_scale);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_rendering_scale)), &GRSViewport::get_rendering_scale);

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "rendering_scale", PROPERTY_HINT_RANGE, "0,1,0.001"), "set_rendering_scale", "get_rendering_scale");
}

#else

void GRSViewport::_register_methods() {
	METHOD_REG(GRSViewport, _notification);
	METHOD_REG(GRSViewport, _update_size);
	METHOD_REG(GRSViewport, _draw_main_vp);

	METHOD_REG(GRSViewport, set_rendering_scale);
	METHOD_REG(GRSViewport, get_rendering_scale);

	register_property<GRSViewport, float>("rendering_scale", &GRSViewport::set_rendering_scale, &GRSViewport::get_rendering_scale, 0.3f, GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT, GODOT_PROPERTY_HINT_RANGE, "0,1,0.001");
}

#endif

void GRSViewport::_notification(int p_notification) {
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
			frames_from_prev_image++;

			if (video_stream_enabled) {
				is_empty_image_sended = false;

				// update resizing viewport
				renderer->update();

				if (frames_from_prev_image > skip_frames) {
					frames_from_prev_image = 0;

					if (get_texture().is_null())
						break;

					// getting 'screenshot'
					Ref<Image> tmp_image;
					{
						ZoneScopedNC("Get image data from VisualServer", tracy::Color::OrangeRed4);
						tmp_image = get_texture()->get_data();
					}

					if (img_is_empty(tmp_image))
						_log("Can't copy viewport image data", LogLevel::LL_ERROR);
					else {
						ZoneScopedNC("Commit Image to Encoder", tracy::Color::DarkOrange3);
						Scoped_lock(stream_mutex);
						if (stream_manager) {
							stream_manager->commit_image(tmp_image, (uint64_t)(get_process_delta_time() * 1000000));
						}
					}
				}
			} else {
				if (!is_empty_image_sended) {
					is_empty_image_sended = true;
					stream_manager->commit_stream_end();
				}
			}

			// update window aspect ratio for client
			Size2 size = OS::get_singleton()->get_window_size();
			float aspect = size.x / size.y;
			if (aspect != prev_screen_aspect_ratio) {
				prev_screen_aspect_ratio = aspect;

				if (server) {
					auto asp_pck = std::make_shared<GRPacketStreamAspectRatio>();
					asp_pck->set_aspect(aspect);
					server->send_packet(asp_pck);
				}
			}

			break;
		}
		case NOTIFICATION_ENTER_TREE: {
			main_vp = ST()->get_root();
			main_vp->connect("size_changed", this, NAMEOF(_update_size));
			_update_size();

			renderer = memnew(Node2D);
			renderer->connect("draw", this, NAMEOF(_draw_main_vp));
			add_child(renderer);

			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			renderer = nullptr;
			main_vp->disconnect("size_changed", this, NAMEOF(_update_size));
			main_vp = nullptr;
			break;
		}
		default:
			break;
	}
}

void GRSViewport::_update_size() {
	float scale = rendering_scale;
	if (auto_scale > 0)
		scale = auto_scale;

	if (main_vp && main_vp->get_texture().is_valid()) {
		Size2 size = main_vp->get_size() * scale;
		if (get_size() == size)
			return;

		int width = (int)size.x;
		int height = (int)size.y;

		width = width - (int)width % STREAM_SIZE_STEP;
		height = height - (int)height % STREAM_SIZE_STEP;

		if (width < 32)
			width = 32;
		else if (width > Image::MAX_WIDTH)
			width = Image::MAX_WIDTH;

		if (height < 32)
			height = 32;
		else if (height > Image::MAX_HEIGHT)
			height = Image::MAX_HEIGHT;

		Size2 ns = Size2(real_t(width), real_t(height));
		if (get_size() != ns) {
			set_size(ns);
		}
	}
}

void GRSViewport::_draw_main_vp() {
	renderer->draw_texture_rect(main_vp->get_texture(), Rect2(Vector2(), get_size()), false);
}

void GRSViewport::force_get_image() {
	frames_from_prev_image = skip_frames;
}

bool GRSViewport::has_data_to_send() {
	Scoped_lock(stream_mutex);
	if (stream_manager)
		return stream_manager->has_data_to_send();
	return false;
}

std::shared_ptr<GRPacket> GRSViewport::pop_data_to_send() {
	Scoped_lock(stream_mutex);
	if (stream_manager)
		return stream_manager->pop_data_to_send();
	return std::shared_ptr<GRPacket>();
}

void GRSViewport::set_video_stream_enabled(bool val) {
	video_stream_enabled = val;
}

bool GRSViewport::is_video_stream_enabled() {
	return video_stream_enabled;
}

void GRSViewport::set_rendering_scale(float val) {
	rendering_scale = val;
	call_deferred(NAMEOF(_update_size));
}

float GRSViewport::get_rendering_scale() {
	return rendering_scale;
}

void GRSViewport::set_compression_type(GRDevice::ImageCompressionType val) {
	Scoped_lock(stream_mutex);
	compression_type = val;
	if (stream_manager)
		stream_manager->set_compression_type(compression_type, this);
}

GRDevice::ImageCompressionType GRSViewport::get_compression_type() {
	return compression_type;
}

void GRSViewport::set_stream_quality(int _quality) {
	ERR_FAIL_COND(_quality < 1 || _quality > 100);
	stream_quality = _quality;
}

int GRSViewport::get_stream_quality() {
	return stream_quality;
}

void GRSViewport::set_skip_frames(int skip) {
	skip_frames = skip;
}

int GRSViewport::get_skip_frames() {
	return skip_frames;
}

void GRSViewport::set_encoder_threads_count(int count) {
	Scoped_lock(stream_mutex);
	if (stream_manager) {
		stream_manager->set_threads_count(count);
	}
}

int GRSViewport::get_encoder_threads_count() {
	Scoped_lock(stream_mutex);
	if (stream_manager) {
		return stream_manager->get_threads_count();
	}
	return GET_PS(GodotRemote::ps_server_image_encoder_threads_count_name);
}

void GRSViewport::_start_encoder() {
	Scoped_lock(stream_mutex);
	set_process(true);
	if (stream_manager) {
		stream_manager->start(compression_type, this);
	}
	prev_screen_aspect_ratio = 0.000001f;
}

void GRSViewport::_stop_encoder() {
	Scoped_lock(stream_mutex);
	set_process(false);
	if (stream_manager) {
		stream_manager->set_active(false);
	}
	prev_screen_aspect_ratio = 0.000001f;
}

void GRSViewport::_init() {
	set_name("GRSViewport");
	LEAVE_IF_EDITOR();

	set_process(false);

	rendering_scale = GET_PS(GodotRemote::ps_server_scale_of_sending_stream_name);

	set_process_priority(-1);
	set_hdr(false);
	set_disable_3d(true);
	set_keep_3d_linear(true);
	set_usage(Viewport::USAGE_2D);
	set_update_mode(Viewport::UPDATE_ALWAYS);
	set_transparent_background(false);
	set_disable_input(true);
	set_shadow_atlas_quadrant_subdiv(0, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(1, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(2, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	set_shadow_atlas_quadrant_subdiv(3, Viewport::SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);

	Scoped_lock(stream_mutex);
	stream_manager = memnew(GRStreamEncodersManager);
}

void GRSViewport::_deinit() {
	LEAVE_IF_EDITOR();

	Scoped_lock(stream_mutex);
	if (stream_manager) {
		memdelete(stream_manager);
	}
}

#endif // NO_GODOTREMOTE_SERVER
