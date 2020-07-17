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
	class ImgProcessingStorage : public Reference {
		GDCLASS(ImgProcessingStorage, Reference);
		_THREAD_SAFE_CLASS_
	private:
		Thread *_thread_process = nullptr;
		bool finished = false;
		PoolByteArray img_data;
		int width, height, bytes_in_color, jpg_quality;

		void _get_texture_data_from_main_thread();
		static void _processing_thread(void *p_user);

	protected:
		static void _bind_methods();

	public:
		Ref<ViewportTexture> tex;
		PoolByteArray ret_data;
		bool is_new_data = false;

		void start(Ref<ViewportTexture> _tex, int _jpg_quality);
		void close();

		~ImgProcessingStorage();
	};

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

	String password;
	bool auto_adjust_scale = false;
	int jpg_quality = 75;
	int target_send_fps = 60;

	float prev_avg_fps = 0;
	void _adjust_viewport_scale();

	void _load_settings();
	void _update_settings_from_client(const Dictionary settings);
	void _remove_resize_viewport(Node *vp);

	virtual void _reset_counters() override;

	static void _thread_listen(void *p_userdata);
	static void _thread_connection(void *p_userdata);

	static AuthResult _auth_client(GRServer *dev, Ref<PacketPeerStream> &ppeer, bool refuse_connection = false);
	static bool _parse_input_data(const PoolByteArray &p_data);
	static const uint8_t *_read_abstract_input_data(class InputEvent *ie, const Vector2 &vs, const uint8_t *data);

protected:
	virtual void _internal_call_only_deffered_start() override;
	virtual void _internal_call_only_deffered_stop() override;

	static void _bind_methods();
	void _notification(int p_notification);

public:
	void set_auto_adjust_scale(bool _val);
	int get_auto_adjust_scale();
	void set_jpg_quality(int _quality);
	int get_jpg_quality();
	void set_target_send_fps(int fps);
	int get_target_send_fps();
	void set_render_scale(float _scale);
	float get_render_scale();
	void set_password(String _pass);
	String get_password();

	GRSViewport *get_gr_viewport();
	class Node *get_settings_node();

	GRServer();
	~GRServer();
};

class GRSViewport : public Viewport {
	GDCLASS(GRSViewport, Viewport);
	friend GRServer;

protected:
	Viewport *main_vp = nullptr;
	class GRSViewportRenderer *renderer = nullptr;
	float rendering_scale = 0.3f;
	float auto_scale = 0.5f;

	static void _bind_methods();
	void _notification(int p_notification);
	void _update_size();

public:
	void set_rendering_scale(float val);
	float get_rendering_scale();

	GRSViewport();
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
