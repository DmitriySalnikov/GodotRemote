/* GRServer.h */
#pragma once

#ifndef NO_GODOTREMOTE_SERVER

#include "GRDevice.h"
#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "scene/gui/control.h"
#include "scene/main/viewport.h"

class GRServer : public GRDevice {
	GDCLASS(GRServer, GRDevice);

private:
	class ListenerThreadParams : public Reference {
		GDCLASS(ListenerThreadParams, Reference);

	public:
		GRServer *dev = nullptr;
		class Thread *thread_ref = nullptr;
		bool stop_thread = false;
		bool finished = false;

		void close_thread() {
			stop_thread = true;
			if (thread_ref) {
				Thread::wait_to_finish(thread_ref);
				memdelete(thread_ref);
				thread_ref = nullptr;
			}
		}

		~ListenerThreadParams() {
			close_thread();
		}
	};

	class ConnectionThreadParams : public Reference {
		GDCLASS(ConnectionThreadParams, Reference);

	public:
		String device_id = "";
		GRServer *dev = nullptr;
		Ref<PacketPeerStream> ppeer;
		class Thread *thread_ref = nullptr;
		bool break_connection = false;
		bool finished = false;

		void close_thread() {
			break_connection = true;
			if (thread_ref) {
				Thread::wait_to_finish(thread_ref);
				memdelete(thread_ref);
				thread_ref = nullptr;
			}
		}

		~ConnectionThreadParams() {
			close_thread();
			if (ppeer.is_valid()) {
				ppeer.unref();
			}
		}
	};

	Node *settings_menu_node = nullptr;
	Mutex *connection_mutex = nullptr;
	Ref<ListenerThreadParams> server_thread_listen;
	Ref<TCP_Server> tcp_server;
	class GRSViewport *resize_viewport = nullptr;
	int client_connected = 0;

	String password;
	String custom_input_scene;
	bool custom_input_scene_was_updated = true;
	bool auto_adjust_scale = false;

	float prev_avg_fps = 0;
	void _adjust_viewport_scale();

	void _load_settings();
	void _update_settings_from_client(const Dictionary settings);
	void _remove_resize_viewport(Node *vp);

	virtual void _reset_counters() override;

	static void _thread_listen(void *p_userdata);
	static void _thread_connection(void *p_userdata);

	static AuthResult _auth_client(GRServer *dev, Ref<PacketPeerStream> &ppeer, Dictionary &ret_data, bool refuse_connection = false);
	static Ref<class GRPacketCustomInputScene> _create_custom_input_pack(String _scene_path);
	static void _scan_dir_for_files_recursive(String _dir, PoolStringArray &_arr);

protected:
	virtual void _internal_call_only_deffered_start() override;
	virtual void _internal_call_only_deffered_stop() override;

	static void _bind_methods();
	void _notification(int p_notification);

public:
	void set_auto_adjust_scale(bool _val);
	int get_auto_adjust_scale();
	void set_password(String _pass);
	String get_password();
	void set_custom_input_scene(String _scn);
	String get_custom_input_scene();

	// VIEWPORT
	bool set_video_stream_enabled(bool val);
	bool is_video_stream_enabled();
	bool set_compression_type(int _type);
	int get_compression_type();
	bool set_jpg_quality(int _quality);
	int get_jpg_quality();
	bool set_skip_frames(int fps);
	int get_skip_frames();
	bool set_render_scale(float _scale);
	float get_render_scale();
	// NOT VIEWPORT

	GRSViewport *get_gr_viewport();
	class Node *get_settings_node();
	void force_update_custom_input_scene();

	GRServer();
	~GRServer();
};

class GRSViewport : public Viewport {
	GDCLASS(GRSViewport, Viewport);
	friend GRServer;
	friend class ImgProcessingStorage;
	_THREAD_SAFE_CLASS_;

public:
	class ImgProcessingStorage : public Reference {
		GDCLASS(ImgProcessingStorage, Reference);
	public:
		PoolByteArray ret_data;
		GRUtils::ImageCompressionType compression_type = GRUtils::ImageCompressionType::Uncompressed;
		int width, height, format;
		int bytes_in_color, jpg_quality;
		bool is_empty = false;

		~ImgProcessingStorage() {
			ret_data.resize(0);
		}
	};

private:
	Thread *_thread_process = nullptr;
	Ref<Image> last_image;
	Ref<ImgProcessingStorage> last_image_data;

	void _close_thread();
	void _set_img_data(Ref<ImgProcessingStorage> _data);
	static void _processing_thread(void *p_user);

protected:
	Viewport *main_vp = nullptr;
	class GRSViewportRenderer *renderer = nullptr;
	bool video_stream_enabled = true;
	float rendering_scale = 0.3f;
	float auto_scale = 0.5f;
	int jpg_quality = 80;
	int skip_frames = 0;
	GRUtils::ImageCompressionType compression_type = GRUtils::ImageCompressionType::Uncompressed;

	uint16_t frames_from_prev_image = 0;
	bool is_empty_image_sended = false;

	static void _bind_methods();
	void _notification(int p_notification);
	void _update_size();

public:
	Ref<ImgProcessingStorage> get_last_compressed_image_data();
	bool has_compressed_image_data();
	void force_get_image();

	void set_video_stream_enabled(bool val);
	bool is_video_stream_enabled();
	void set_rendering_scale(float val);
	float get_rendering_scale();
	void set_compression_type(int val);
	int get_compression_type();
	void set_jpg_quality(int _quality);
	int get_jpg_quality();
	void set_skip_frames(int skip);
	int get_skip_frames();

	GRSViewport();
	~GRSViewport();
};

class GRSViewportRenderer : public Control {
	GDCLASS(GRSViewportRenderer, Control);

protected:
	Viewport *vp = nullptr;

	static void _bind_methods();
	void _notification(int p_notification);

public:
	Ref<ViewportTexture> tex;

	GRSViewportRenderer();
};

#endif // !NO_GODOTREMOTE_SERVER
