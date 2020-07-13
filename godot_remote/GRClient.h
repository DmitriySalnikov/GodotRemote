/* GRClient.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRDevice.h"
#include "core/io/ip_address.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/main/node.h"

class GRClient : public GRDevice {
	GDCLASS(GRClient, GRDevice);

public:
	enum ConnectionType {
		CONNECTION_WiFi = 0,
		CONNECTION_ADB = 1,
	};

	enum StretchMode {
		STRETCH_KEEP_ASPECT = 0,
		STRETCH_FILL = 1,
	};

private:
	class ImgProcessingStorage {
	public:
		GRClient *dev = nullptr;
		PoolByteArray tex_data;
		bool *_is_processing_img = nullptr;

		ImgProcessingStorage(GRClient *_dev) {
			dev = _dev;
		}

		~ImgProcessingStorage() {
			tex_data.resize(0);
		}
	};

	class ConnectionThreadParams : public Reference {
		GDCLASS(ConnectionThreadParams, Reference);

	public:
		GRClient *dev = nullptr;
		class Ref<StreamPeerTCP> peer;
		class Thread *thread_ref = nullptr;
		bool break_connection = false;
		bool stop_thread = false;

		void close_thread() {
			break_connection = true;
			stop_thread = true;
			if (thread_ref) {
				Thread::wait_to_finish(thread_ref);
				memdelete(thread_ref);
				thread_ref = nullptr;
			}
		}

		~ConnectionThreadParams() {
			dev = nullptr;
			close_thread();
			if (peer.is_valid()) {
				peer.unref();
			}
		}
	};

	bool is_deleting = false;
	bool is_connection_working = false;
	Node *settings_menu_node = nullptr;
	class Control *control_to_show_in = nullptr;
	class TextureRect *tex_shows_stream = nullptr;
	class GRInputCollector *input_collector = nullptr;
	Ref<ConnectionThreadParams> thread_connection = nullptr;

	IP_Address server_address = String("127.0.0.1");
	const String ip_validator_pattern = "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$";
	Ref<class RegEx> ip_validator;

	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	StretchMode stretch_mode = StretchMode::STRETCH_KEEP_ASPECT;

	Mutex *send_queue_mutex = nullptr;
	Mutex *connection_mutex = nullptr;
	List<Ref<class GRPacket> > send_queue;
	ConnectionType con_type = ConnectionType::CONNECTION_WiFi;
	int input_buffer_size_in_mb = 4;
	int send_data_fps = 60;
	uint32_t prev_display_image_time = 60;

	// NO SIGNAL screen
	uint32_t prev_valid_connection_time = 0;
	bool signal_connection_state = false;
	Ref<class ImageTexture> custom_no_signal_texture;
	Ref<class Material> custom_no_signal_material;

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	Ref<class Image> no_signal_image;
	Ref<class ShaderMaterial> no_signal_mat;
#endif

	template <class T>
	T _find_queued_packet_by_type() {
		for (auto e = send_queue.front(); e; e = e->next()) {
			T o = e->get();
			if (o.is_valid()) {
				return o;
			}
		}
		return T();
	}

	void _update_texture_from_iamge(Ref<Image> img);
	void _update_stream_texture_state(bool is_has_signal);
	virtual void _reset_counters() override;

	static void _thread_connection(void *p_userdata);
	static void _thread_image_decoder(void *p_userdata);

	static void _connection_loop(Ref<ConnectionThreadParams> con_thread, class Ref<StreamPeerTCP> connection);
	static bool _auth_on_server(Ref<StreamPeerTCP> con);

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
	bool is_connected_to_host();
	String get_ip();
	bool set_ip(String ip, bool ipv4 = true);
	bool set_address(String ip, uint16_t _port, bool ipv4 = true);
	void set_input_buffer(int mb);

	void set_server_setting(int param, Variant value);
	void disable_overriding_server_settings();

	virtual void _internal_call_only_deffered_start() override;
	virtual void _internal_call_only_deffered_stop() override;

	GRClient();
	~GRClient();
};

class GRInputCollector : public Node {
	GDCLASS(GRInputCollector, Node);

private:
	GRClient *dev = nullptr;
	class TextureRect *texture_rect = nullptr;
	PoolByteArray collected_input_data;
	class Control *parent;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	Rect2 stream_rect;
	PoolVector3Array sensors;

protected:
	void _update_stream_rect();

	static void _bind_methods();

	void _input(Ref<InputEvent> ie);
	void _notification(int p_notification);

public:
	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);

	void set_gr_device(GRClient *_dev);
	void set_tex_rect(class TextureRect *tr);

	PoolByteArray get_collected_input_data();

	GRInputCollector();
	~GRInputCollector();
};

#endif // !NO_GODOTREMOTE_CLIENT

VARIANT_ENUM_CAST(GRClient::ConnectionType)
VARIANT_ENUM_CAST(GRClient::StretchMode)
