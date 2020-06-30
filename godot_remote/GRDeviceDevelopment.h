/* GRDeviceDevelopment.h */
#pragma once

#include "GRDevice.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/main/viewport.h"
#include "scene/gui/control.h"

class GRDeviceDevelopment : public GRDevice {
	GDCLASS(GRDeviceDevelopment, GRDevice);

private:
	class StartThreadArgs {
	public:
		GRDeviceDevelopment *dev = nullptr;
		Ref<StreamPeerTCP> con;
		StartThreadArgs(GRDeviceDevelopment* d, Ref<StreamPeerTCP> c) {
			dev = d;
			con = c;
		}
	};

	enum class AuthResult {
		Error,
		VersionMismatch,
		SendClient,
		RecvClient,
	};

	class Node *settings_menu_node = nullptr;
	class Thread *server_thread_listen = nullptr;
	class TCP_Server *tcp_server = nullptr;
	class GRDDViewport *resize_viewport = nullptr;

	bool stop_device = false;
	bool break_connection = false;
	int target_send_fps = 25;

	static AuthResult _auth_client(GRDeviceDevelopment *dev, Ref<StreamPeerTCP> con);
	static void _process_input_data(const PoolByteArray &p_data);
	static const uint8_t *_read_abstract_input_data(class InputEvent *ie, const Vector2 vs, const uint8_t *data);

	static void _thread_listen(void *p_userdata);
	static void _thread_send_data(void *p_userdata);
	static void _thread_recieve_input(void *p_userdata);

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
