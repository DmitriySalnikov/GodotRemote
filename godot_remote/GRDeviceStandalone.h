/* GRDeviceStandalone.h */
#pragma once

#include "GRDevice.h"
#include "core/io/ip_address.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/main/node.h"

class GRDeviceStandalone : public GRDevice {
	GDCLASS(GRDeviceStandalone, GRDevice);

private:
	class ImgProcessingStorage {
	public:
		GRDeviceStandalone *dev = nullptr;
		PoolByteArray tex_data;
		bool is_new_data = false;

		ImgProcessingStorage(GRDeviceStandalone *d) {
			dev = d;
		}
		~ImgProcessingStorage() {
			tex_data.resize(0);
		}
	};

	class StartThreadArgs {
	public:
		GRDeviceStandalone *dev = nullptr;

		StartThreadArgs(GRDeviceStandalone *d) {
			dev = d;
		}
	};

	class Node *settings_menu_node = nullptr;
	class Thread *thread_connection = nullptr;
	class Thread *thread_image_decoder = nullptr;
	class Ref<StreamPeerTCP> peer;

	const String ip_validator_pattern = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
	Ref<class RegEx> ip_validator;

	bool stop_device = false;
	bool break_connection = false;
	IP_Address server_address = String("127.0.0.1");
	int send_data_fps = 60;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	ImgProcessingStorage *ips = nullptr;
	class Control *control_to_show_in = nullptr;
	class Sprite *sprite_shows_stream = nullptr;
	class GRInputCollector *input_collector = nullptr;

	void _update_texture_from_iamge(Ref<Image> img);

	static void _thread_connection(void *p_userdata);
	static void _thread_image_decoder(void *p_userdata);

	static void _connection_loop(GRDeviceStandalone *dev, Ref<StreamPeerTCP> con);
	static bool _auth_on_server(Ref<StreamPeerTCP> con);
	static PoolByteArray _process_input_data(GRDeviceStandalone *dev);

protected:
	static void _bind_methods();
	void _notification(int p_notification);

public:
	void set_control_to_show_in(class Control *ctrl, int position_in_node = 0);

	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);
	String get_ip();
	void set_ip(String ip, bool ipv4 = true);
	void set_address(String ip, uint16_t port, bool ipv4 = true);

	virtual bool start() override;
	virtual void stop() override;

	GRDeviceStandalone();
	~GRDeviceStandalone();
};

class GRInputCollector : public Node {
	GDCLASS(GRInputCollector, Node);

private:
	GRDeviceStandalone *grdev = nullptr;
	Array collected_input;
	PoolVector3Array sensors;
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
	PoolVector3Array get_sensors();

	GRInputCollector();
	~GRInputCollector();
};
