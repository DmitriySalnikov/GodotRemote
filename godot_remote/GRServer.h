/* GRServer.h */
#pragma once

#ifndef NO_GODOTREMOTE_SERVER

#include "GRDevice.h"

#ifndef GDNATIVE_LIBRARY

#include "core/io/compression.h"
#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "modules/regex/regex.h"
#include "scene/gui/control.h"
#include "scene/main/viewport.h"
#else

#include <Control.hpp>
#include <PacketPeerStream.hpp>
#include <RegEx.hpp>
#include <RegExMatch.hpp>
#include <StreamPeerTCP.hpp>
#include <TCP_Server.hpp>
#include <Thread.hpp>
#include <Viewport.hpp>
#include <ViewportTexture.hpp>
using namespace godot;
#endif

class GRServer : public GRDevice {
	GD_S_CLASS(GRServer, GRDevice);

private:
#ifndef GDNATIVE_LIBRARY
#else
public:
#endif
	class ListenerThreadParamsServer : public Object {
		GD_CLASS(ListenerThreadParamsServer, Object);

	public:
		GRServer *dev = nullptr;
		Thread_define(thread_ref);

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
		GRServer *dev = nullptr;
		Ref<PacketPeerStream> ppeer;

		Thread_define(thread_ref);

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
			if (ppeer.is_valid()) {
				ppeer.unref();
			}
		}
	};

private:
	Mutex_define(connection_mutex);
	ListenerThreadParamsServer *server_thread_listen = nullptr;
	Ref<TCP_Server> tcp_server;
	class GRSViewport *resize_viewport = nullptr;
	int client_connected = 0;

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
	Ref<class RegEx> custom_input_scene_regex_resource_finder;

	float prev_avg_fps = 0;
	void _adjust_viewport_scale();

	void _load_settings();
	void _update_settings_from_client(const std::map<int, Variant> settings);
	void _remove_resize_viewport(Node *vp);
	void _on_grviewport_deleting();

	virtual void _reset_counters() override;

	THREAD_FUNC void _thread_listen(THREAD_DATA p_userdata);
	THREAD_FUNC void _thread_connection(THREAD_DATA p_userdata);

	static AuthResult _auth_client(GRServer *dev, Ref<PacketPeerStream> &ppeer, Dictionary &ret_data, bool refuse_connection DEF_ARG(= false));

	Ref<GRPacketCustomInputScene> _create_custom_input_pack(String _scene_path, bool compress DEF_ARG(= true), ENUM_ARG(Compression::Mode) compression_type DEF_ARG(= ENUM_CONV(Compression::Mode) 0));
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

	// VIEWPORT
	bool set_video_stream_enabled(bool val);
	bool is_video_stream_enabled();
	bool set_compression_type(ImageCompressionType _type);
	ImageCompressionType get_compression_type();
	bool set_jpg_quality(int _quality);
	int get_jpg_quality();
	bool set_skip_frames(int fps);
	int get_skip_frames();
	bool set_render_scale(float _scale);
	float get_render_scale();
	// NOT VIEWPORT

	GRSViewport *get_gr_viewport();
	void force_update_custom_input_scene();

	void _init();
	void _deinit();
};

class GRSViewport : public Viewport {
	GD_CLASS(GRSViewport, Viewport);
	friend GRServer;
	friend class ImgProcessingViewportStorage;
	_TS_CLASS_;

public:
	class ImgProcessingViewportStorage : public Object {
		GD_CLASS(ImgProcessingViewportStorage, Object);

	public:
		PoolByteArray ret_data;
		GRDevice::ImageCompressionType compression_type = GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED;
		int width, height, format;
		int bytes_in_color, jpg_quality;
		bool is_empty = false;

		static void _register_methods(){};
		void _init() {
			LEAVE_IF_EDITOR();
			ret_data = PoolByteArray();
		};

		~ImgProcessingViewportStorage() {
			LEAVE_IF_EDITOR();
			ret_data.resize(0);
		}
	};

private:
	Thread_define(_thread_process);

	Ref<Image> last_image;
	ImgProcessingViewportStorage *last_image_data = nullptr;

	void _close_thread() { Thread_close(_thread_process); }
	void _set_img_data(ImgProcessingViewportStorage *_data);
	void _on_renderer_deleting();

	THREAD_FUNC void _processing_thread(THREAD_DATA p_user);

protected:
	Viewport *main_vp = nullptr;
	class GRSViewportRenderer *renderer = nullptr;
	bool video_stream_enabled = true;
	float rendering_scale = 0.3f;
	float auto_scale = 0.5f;
	int jpg_quality = 80;
	int skip_frames = 0;
	GRDevice::ImageCompressionType compression_type = GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED;

	uint16_t frames_from_prev_image = 0;
	bool is_empty_image_sended = false;
	bool is_thread_active = false;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);
	void _update_size();

public:
	ImgProcessingViewportStorage *get_last_compressed_image_data();
	bool has_compressed_image_data();
	void force_get_image();

	void set_video_stream_enabled(bool val);
	bool is_video_stream_enabled();
	void set_rendering_scale(float val);
	float get_rendering_scale();
	void set_compression_type(GRDevice::ImageCompressionType val);
	GRDevice::ImageCompressionType get_compression_type();
	void set_jpg_quality(int _quality);
	int get_jpg_quality();
	void set_skip_frames(int skip);
	int get_skip_frames();

	void _init();
	void _deinit();
};

class GRSViewportRenderer : public Control {
	GD_CLASS(GRSViewportRenderer, Control);

protected:
	Viewport *vp = nullptr;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	Ref<ViewportTexture> tex;

	void _init();
	void _deinit();
};

#endif // !NO_GODOTREMOTE_SERVER
