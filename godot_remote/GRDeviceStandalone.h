/* GRDeviceStandalone.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRDevice.h"
#include "core/io/ip_address.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/main/node.h"

class GRDeviceStandalone : public GRDevice {
	GDCLASS(GRDeviceStandalone, GRDevice);

public:
	enum StretchMode {
		KeepAspect = 0,
		Fill = 1,
	};

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

	ImgProcessingStorage *ips = nullptr;
	class Node *settings_menu_node = nullptr;
	class Thread *thread_connection = nullptr;
	class Thread *thread_image_decoder = nullptr;
	class Control *control_to_show_in = nullptr;
	class TextureRect *tex_shows_stream = nullptr;
	class GRInputCollector *input_collector = nullptr;
	class Ref<StreamPeerTCP> peer;

	bool stop_device = false;
	bool break_connection = false;

	IP_Address server_address = String("127.0.0.1");
	const String ip_validator_pattern = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
	Ref<class RegEx> ip_validator;

	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	StretchMode stretch_mode = StretchMode::KeepAspect;

	int send_data_fps = 60;
	uint32_t prev_display_image_time = 60;

	// NO SIGNAL screen
	uint32_t prev_valid_connection_time = 0;
	bool signal_connection_state = false;
	Ref<class ImageTexture> custom_no_signal_texture;
	Ref<class Material> custom_no_signal_material;

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	Ref<class ImageTexture> no_signal_texture;
	Ref<class ShaderMaterial> no_signal_mat;
#endif

	void _update_texture_from_iamge(Ref<Image> img);
	void _update_stream_texture_state(bool is_has_signal);
	virtual void _reset_counters() override;

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
	void set_custom_no_signal_texture(Ref<Texture> custom_tex);
	void set_custom_no_signal_material(Ref<Material> custom_mat);

	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);
	void set_connection_type(int type);
	int get_connection_type();
	void set_target_send_fps(int fps);
	int get_target_send_fps();
	void set_stretch_mode(int stretch);
	int get_stretch_mode();

	bool is_stream_active();
	String get_ip();
	bool set_ip(String ip, bool ipv4 = true);
	bool set_address(String ip, uint16_t _port, bool ipv4 = true);

	virtual bool start() override;
	virtual void stop() override;

	GRDeviceStandalone();
	~GRDeviceStandalone();
};

class GRInputCollector : public Node {
	GDCLASS(GRInputCollector, Node);

private:
	GRDeviceStandalone *grdev = nullptr;
	class TextureRect *texture_rect = nullptr;
	Array collected_input;
	PoolVector3Array sensors;
	class Control *parent;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	Rect2 keep_aspect_rect_because_other_ways_to_get_all_time_fails;

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
	void set_tex_rect(class TextureRect *tr);

	Rect2 get_keep_aspect_rect();
	Array get_collected_input();
	PoolVector3Array get_sensors();

	GRInputCollector();
	~GRInputCollector();
};

#endif // !NO_GODOTREMOTE_CLIENT
