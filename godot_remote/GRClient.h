/* GRClient.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRDevice.h"
#include "core/io/ip_address.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/node.h"

class GRClient : public GRDevice {
	GDCLASS(GRClient, GRDevice);

	friend class GRTextureRect;

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
		uint64_t framerate = 0;
		int format = 0;
		GRUtils::ImageCompressionType compression_type = GRUtils::ImageCompressionType::Uncompressed;
		Size2 size;
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
		Ref<StreamPeerTCP> peer;
		Ref<PacketPeerStream> ppeer;
		class Thread *thread_ref = nullptr;
		bool break_connection = false;
		bool stop_thread = false;
		bool finished = false;

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
			close_thread();
			if (peer.is_valid()) {
				peer.unref();
			}
			if (ppeer.is_valid()) {
				ppeer.unref();
			}
		};
	};

	bool is_deleting = false;
	bool is_connection_working = false;
	Node *settings_menu_node = nullptr;
	class Control *control_to_show_in = nullptr;
	class GRTextureRect *tex_shows_stream = nullptr;
	class GRInputCollector *input_collector = nullptr;
	Ref<ConnectionThreadParams> thread_connection;

	String device_id = "UNKNOWN";
	String server_address = String("127.0.0.1");

	String password;
	bool is_filtering_enabled = true;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	StretchMode stretch_mode = StretchMode::STRETCH_KEEP_ASPECT;

	Mutex *send_queue_mutex = nullptr;
	Mutex *connection_mutex = nullptr;
	List<Ref<class GRPacket> > send_queue;
	ConnectionType con_type = ConnectionType::CONNECTION_WiFi;
	int input_buffer_size_in_mb = 4;
	int send_data_fps = 60;

	uint64_t sync_time_client = 0;
	uint64_t sync_time_server = 0;

	// NO SIGNAL screen
	uint64_t prev_valid_connection_time = 0;
	bool signal_connection_state = false;
	bool no_signal_is_vertical = false;
	Ref<class Texture> custom_no_signal_texture;
	Ref<class Texture> custom_no_signal_vertical_texture;
	Ref<class Material> custom_no_signal_material;

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	Ref<class Image> no_signal_image;
	Ref<class Image> no_signal_vertical_image;
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

	static void _connection_loop(Ref<ConnectionThreadParams> con_thread);
	static GRDevice::AuthResult _auth_on_server(GRClient *dev, Ref<PacketPeerStream> &con);

protected:
	virtual void _internal_call_only_deffered_start() override;
	virtual void _internal_call_only_deffered_stop() override;

	static void _bind_methods();
	void _notification(int p_notification);

public:
	void set_control_to_show_in(class Control *ctrl, int position_in_node = 0);
	void set_custom_no_signal_texture(Ref<Texture> custom_tex);
	void set_custom_no_signal_vertical_texture(Ref<Texture> custom_tex);
	void set_custom_no_signal_material(Ref<Material> custom_mat);

	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);
	void set_connection_type(int type);
	int get_connection_type();
	void set_skip_frames(int fps);
	int get_skip_frames();
	void set_stretch_mode(int stretch);
	int get_stretch_mode();
	void set_texture_filtering(bool is_filtering);
	bool get_texture_filtering();
	void set_password(String _pass);
	String get_password();
	void set_device_id(String _id);
	String get_device_id();

	void send_packet(Ref<GRPacket> packet);
	bool is_stream_active();
	bool is_connected_to_host();
	String get_ip();
	bool set_ip(String ip, bool ipv4 = true);
	bool set_address(String ip, uint16_t _port, bool ipv4 = true);
	void set_input_buffer(int mb);

	void set_server_setting(int param, Variant value);
	void disable_overriding_server_settings();

	GRClient();
	~GRClient();
};

class GRInputCollector : public Node {
	GDCLASS(GRInputCollector, Node);
	friend GRClient;

	_THREAD_SAFE_CLASS_
private:
	GRClient *dev = nullptr;
	GRInputCollector **this_in_client = nullptr; //somebody help

	class TextureRect *texture_rect = nullptr;
	Vector<Ref<GRInputData> > collected_input_data;
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

	void set_tex_rect(class TextureRect *tr);

	Ref<class GRPacketInputData> get_collected_input_data();

	GRInputCollector();
	~GRInputCollector();
};

class GRTextureRect : public TextureRect {
	GDCLASS(GRTextureRect, TextureRect);
	friend GRClient;

	GRClient *dev = nullptr;
	GRTextureRect **this_in_client = nullptr;
	void _tex_size_changed();

protected:
	static void _bind_methods();

public:
	GRTextureRect();
	~GRTextureRect();
};

#endif // !NO_GODOTREMOTE_CLIENT

VARIANT_ENUM_CAST(GRClient::ConnectionType)
VARIANT_ENUM_CAST(GRClient::StretchMode)
