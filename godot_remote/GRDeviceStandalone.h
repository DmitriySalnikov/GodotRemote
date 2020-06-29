/* GRDeviceStandalone.h */
#pragma once

#include "GRDevice.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/main/node.h"

class GRDeviceStandalone : public GRDevice {
	GDCLASS(GRDeviceStandalone, GRDevice);

private:
	class StartThreadArgs {
	public:
		GRDeviceStandalone *dev = nullptr;
		Ref<StreamPeerTCP> con;
	};

	class Node *settings_menu_node = nullptr;
	class Thread *thread_connection_establisher = nullptr;
	class Ref<StreamPeerTCP> tcp_peer = nullptr;

	bool is_stopped = true;
	bool stop_device = false;
	bool break_connection = false;
	String server_address = "192.168.88.88";
	int server_port = 52341;
	int send_data_fps = 60;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	class Control *control_to_show_in = nullptr;
	class Sprite *sprite_shows_stream = nullptr;
	class GRInputCollector *input_collector = nullptr;

	void _update_texture_from_iamge(Ref<Image> img);

	static void _send_data(StartThreadArgs *p_userdata);
	static PoolByteArray _process_input_data(GRDeviceStandalone *dev);

	static void _thread_connection_establisher(void *p_userdata);
	static void _thread_recieve_data(void *p_userdata);

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

	Array get_collected_input();

	GRInputCollector();
};
