/* GRServer.h */
#pragma once

#ifndef NO_GODOTREMOTE_SERVER

#include "GRDevice.h"
#include "GRStreamEncoders.h"

#ifndef GDNATIVE_LIBRARY
#include "core/io/compression.h"
#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "modules/regex/regex.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"
#include "scene/main/viewport.h"
#else
#include <Control.hpp>
#include <Node2D.hpp>
#include <PacketPeerStream.hpp>
#include <RegEx.hpp>
#include <RegExMatch.hpp>
#include <StreamPeerTCP.hpp>
#include <TCP_Server.hpp>
#include <Viewport.hpp>
using namespace godot;
#endif

class GRServer : public GRDevice {
	GD_CLASS(GRServer, GRDevice);

private:
#ifndef GDNATIVE_LIBRARY
#else
public:
#endif
	class ListenerThreadParamsServer : public Object {
		GD_CLASS(ListenerThreadParamsServer, Object);

	public:
		Ref<_Thread> thread_ref;

		bool stop_thread = false;
		bool finished = false;

		static void _register_methods(){};
		void _init(){};

		void close_thread() {
			stop_thread = true;
			Thread_close(thread_ref);
		}

		~ListenerThreadParamsServer() {
			LEAVE_IF_EDITOR();
			close_thread();
		}
	};

	class ConnectionThreadParamsServer : public Object {
		GD_CLASS(ConnectionThreadParamsServer, Object);

	public:
		String device_id = "";
		RefStd(PacketPeerStream) ppeer;

		Ref<_Thread> thread_ref;

		bool break_connection = false;
		bool finished = false;

		static void _register_methods(){};
		void _init(){};

		void close_thread() {
			break_connection = true;
			Thread_close(thread_ref);
		}

		~ConnectionThreadParamsServer() {
			LEAVE_IF_EDITOR();
			close_thread();
			ppeer = nullptr;
		}
	};

private:
	Mutex_define(connection_mutex, "Connection Lock");
	Mutex_define(udp_lock, "UDP Thread Lock");
	ListenerThreadParamsServer *server_thread_listen = nullptr;
	Ref<TCP_Server> tcp_server;
	class GRSViewport *resize_viewport = nullptr;
#ifdef GODOT_REMOTE_AUTO_CONNECTION_ENABLED
	class GRViewportCaptureRect *udp_preview_viewport = nullptr;
#endif
	int64_t unique_server_id = 0;
	int client_connected = 0;
	int target_fps = 60;
	int current_listening_port = 0;

	bool use_static_port = false;
	bool using_client_settings = false;
	bool using_client_settings_recently_updated = false;

	String password;
	String custom_input_scene;
	bool custom_input_scene_was_updated = false;
	bool auto_adjust_scale = false;

	bool custom_input_pck_compressed = true;
	ENUM_ARG(Compression::Mode)
	custom_input_pck_compression_type = ENUM_CONV(Compression::Mode) 0;
	const String custom_input_scene_regex_resource_finder_pattern = "\\\"(res://.*?)\\\"";
	Ref<RegEx> custom_input_scene_regex_resource_finder;
	PoolByteArray preview_image_data_jpg_buffer;
	PoolByteArray preview_image_data;
	PoolByteArray project_icon_image_data;
	int64_t project_icon_image_flags = 4; // only filtering

	float prev_avg_fps = 0;
	void _adjust_viewport_scale();

	void _load_settings(bool force_hide_notifications = false);
	void _update_settings_from_client(const std::map<int, Variant> settings);

	virtual void _reset_counters() override;

#ifdef GODOT_REMOTE_AUTO_CONNECTION_ENABLED
	void _thread_udp_connection(Variant p_userdata);
#endif
	void _thread_listen(Variant p_userdata);
	void _thread_connection(Variant p_userdata);

	AuthResult _auth_client(RefStd(PacketPeerStream) ppeer, Dictionary &ret_data, bool refuse_connection DEF_ARG(= false));

	std::shared_ptr<GRPacketCustomInputScene> _create_custom_input_pack(String _scene_path, bool compress DEF_ARG(= true), ENUM_ARG(Compression::Mode) compression_type DEF_ARG(= ENUM_CONV(Compression::Mode) 0));
	void _scan_resource_for_dependencies_recursive(String _dir, std::vector<String> &_arr);

protected:
	virtual void _internal_call_only_deffered_start() override;
	virtual void _internal_call_only_deffered_stop() override;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	void set_auto_adjust_scale(bool _val);
	bool is_auto_adjust_scale();
	void set_password(String _pass);
	String get_password();
	void set_custom_input_scene(String _scn);
	String get_custom_input_scene();
	void set_custom_input_scene_compressed(bool _is_compressed);
	bool is_custom_input_scene_compressed();
	void set_custom_input_scene_compression_type(int _type);
	int get_custom_input_scene_compression_type();

	bool set_target_fps(int value);
	int get_target_fps();

	// VIEWPORT
	bool set_video_stream_enabled(bool val);
	bool is_video_stream_enabled();
	bool set_compression_type(ImageCompressionType _type);
	ImageCompressionType get_compression_type();
	bool set_stream_quality(int _quality);
	int get_stream_quality();
	bool set_skip_frames(int fps);
	bool set_render_scale(float _scale);
	float get_render_scale();
	int get_skip_frames();
	bool set_encoder_threads_count(int count);
	int get_encoder_threads_count();
	// NOT VIEWPORT

	GRSViewport *get_gr_viewport();
	void force_update_custom_input_scene();

	virtual uint16_t get_port() override;
	virtual void set_port(uint16_t _port) override;

	void _init();
	void _deinit();
};

class GRSViewport : public Viewport {
	GD_CLASS(GRSViewport, Viewport);
	friend GRServer;

	Mutex_define(stream_mutex, "Stream Manager Mutex");

private:
	float prev_screen_aspect_ratio = 0.0000001f;
	GRServer *server = nullptr;

	void _start_encoder();
	void _stop_encoder();

protected:
	Viewport *main_vp = nullptr;
	Node2D *renderer = nullptr;
	bool video_stream_enabled = true;
	float rendering_scale = 0.3f;
	float auto_scale = 0.5f;
	int stream_quality = 80;
	int skip_frames = 0;
	GRDevice::ImageCompressionType compression_type = GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED;
	std::shared_ptr<GRStreamEncodersManager> stream_manager;

	uint16_t frames_from_prev_image = 0;
	bool is_empty_image_sended = false;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);
	void _update_size();
	void _draw_main_vp();

public:
	void force_get_image();
	bool has_data_to_send();
	std::shared_ptr<GRPacket> pop_data_to_send();

	void set_video_stream_enabled(bool val);
	bool is_video_stream_enabled();
	void set_rendering_scale(float val);
	float get_rendering_scale();
	void set_compression_type(GRDevice::ImageCompressionType val);
	GRDevice::ImageCompressionType get_compression_type();
	void set_stream_quality(int _quality);
	int get_stream_quality();
	void set_skip_frames(int skip);
	int get_skip_frames();
	void set_encoder_threads_count(int count);
	int get_encoder_threads_count();

	void _init();
	void _deinit();
};

#endif // NO_GODOTREMOTE_SERVER
