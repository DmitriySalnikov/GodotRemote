/* GRDeviceDevelopment.h */
#pragma once

#include "GRDevice.h"
#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "scene/gui/control.h"
#include "scene/main/viewport.h"

class GRDeviceDevelopment : public GRDevice {
	GDCLASS(GRDeviceDevelopment, GRDevice);

private:
	class ImgProcessingStorage {
	public:
		GRDeviceDevelopment *dev = nullptr;
		Ref<ViewportTexture> tex;
		PoolByteArray ret_data;
		bool is_new_data = false;

		ImgProcessingStorage(GRDeviceDevelopment *d) {
			dev = d;
		}
		~ImgProcessingStorage() {
			ret_data.resize(0);
			tex.unref();
		}
	};

	class StartThreadArgs {
	public:
		GRDeviceDevelopment *dev = nullptr;
		Ref<StreamPeerTCP> con;
		StartThreadArgs(GRDeviceDevelopment *d, Ref<StreamPeerTCP> c) {
			dev = d;
			con = c;
		}
	};

	enum class AuthResult {
		OK,
		Error,
		VersionMismatch,
	};

	ImgProcessingStorage *ips = nullptr;
	class Node *settings_menu_node = nullptr;
	class Thread *server_thread_listen = nullptr;
	class Thread *t_image_encode = nullptr;
	class Ref<TCP_Server> tcp_server;
	class GRDDViewport *resize_viewport = nullptr;

	bool stop_device = false;
	bool break_connection = false;
	int target_send_fps = 60;
	int jpg_quality = 75;

	static void _thread_listen(void *p_userdata);
	static void _thread_connection(void *p_userdata);
	static void _thread_image_processing(void *p_userdata);

	static AuthResult _auth_client(GRDeviceDevelopment *dev, Ref<StreamPeerTCP> con);
	static void _process_input_data(const PoolByteArray &p_data);
	static const uint8_t *_read_abstract_input_data(class InputEvent *ie, const Vector2 &vs, const uint8_t *data);

protected:
	static void _bind_methods();
	void _notification(int p_notification);

public:
	virtual bool start() override;
	virtual void stop() override;

	GRDDViewport *get_gr_viewport();
	class Node *get_settings_node();

	GRDeviceDevelopment();
	~GRDeviceDevelopment();
};

class GRDDViewport : public Viewport {
	GDCLASS(GRDDViewport, Viewport);

protected:
	Viewport *main_vp = nullptr;
	class GRDDViewportRenderer *renderer = nullptr;
	float rendering_scale = 0.5f;

	static void _bind_methods();
	void _notification(int p_notification);
	void _update_size();

public:
	void set_rendering_scale(float val);
	float get_rendering_scale();

	GRDDViewport();
};

class GRDDViewportRenderer : public Control {
	GDCLASS(GRDDViewportRenderer, Control);

protected:
	Viewport *vp = nullptr;

	static void _bind_methods();
	void _notification(int p_notification);

public:
	Ref<ViewportTexture> tex;

	GRDDViewportRenderer();
};
