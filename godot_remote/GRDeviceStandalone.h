/* GRDeviceStandalone.h */
#pragma once

#include "GRDevice.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/main/node.h"
#include "core/io/ip_address.h"

class GRDeviceStandalone : public GRDevice {
	GDCLASS(GRDeviceStandalone, GRDevice);

private:
	class StartThreadArgs {
	public:
		GRDeviceStandalone *dev = nullptr;
		bool is_recv = false;

		StartThreadArgs(GRDeviceStandalone* d, bool recv) {
			dev = d;
			is_recv = recv;
		}
	};

	class Node *settings_menu_node = nullptr;
	class Thread *thread_send_data = nullptr;
	class Thread *thread_recv_data = nullptr;
	class Ref<StreamPeerTCP> peer_send;
	class Ref<StreamPeerTCP> peer_recv;

	bool stop_device = false;
	bool break_connection = false;
	IP_Address server_address = String("192.168.88.88");
	int send_data_fps = 60;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	class Control *control_to_show_in = nullptr;
	class Sprite *sprite_shows_stream = nullptr;
	class GRInputCollector *input_collector = nullptr;

	void _update_texture_from_iamge(Ref<Image> img);

	static bool _auth_on_server(Ref<StreamPeerTCP> con, bool is_recv);
	static void _send_data(GRDeviceStandalone *dev, Ref<StreamPeerTCP> con);
	static void _recv_data(GRDeviceStandalone *dev, Ref<StreamPeerTCP> con);
	static PoolByteArray _process_input_data(GRDeviceStandalone *dev);

	static void _thread_send_recv_data(void *p_userdata);

protected:
	static void _bind_methods();
	void _notification(int p_notification);

public:
	void set_control_to_show_in(class Control *ctrl, int position_in_node = 0);

	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);

	virtual bool start() override;
	virtual void stop() override;

	GRDeviceStandalone();
};

class GRInputCollector : public Node {
	GDCLASS(GRInputCollector, Node);

private:

	GRDeviceStandalone *grdev = nullptr;
	Array collected_input;
	class Control *parent = nullptr;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;

protected:
	static void _bind_methods();

	void _input(Ref<InputEvent> ie);
	void _notification(int p_notification);

public:
	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);

	void set_gr_device(GRDeviceStandalone *dev);

	Array get_collected_input();

	GRInputCollector();
};
