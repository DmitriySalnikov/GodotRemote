/* GRClient.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRDevice.h"
#include "GRStreamDecoders.h"

#ifndef GDNATIVE_LIBRARY
#include "core/io/ip_address.h"
#include "core/io/stream_peer_tcp.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/node.h"

#else

#include <Node.hpp>
#include <PacketPeerStream.hpp>
#include <ShaderMaterial.hpp>
#include <StreamPeerTCP.hpp>
#include <TextureRect.hpp>
#include <Thread.hpp>
using namespace godot;
#endif

class GRClient : public GRDevice {
	GD_CLASS(GRClient, GRDevice);

	friend class GRTextureRect;
	friend class GRStreamDecoderImageSequence;
	friend class GRStreamDecoderH264;

	enum class ScreenOrientation : int {
		NONE = 0,
		VERTICAL = 1,
		HORIZONTAL = 2,
	};

public:
	enum ConnectionType : int {
		CONNECTION_WiFi = 0,
		CONNECTION_ADB = 1,
	};

	enum StretchMode : int {
		STRETCH_KEEP_ASPECT = 0,
		STRETCH_FILL = 1,
	};

	enum StreamState : int {
		STREAM_NO_SIGNAL = 0,
		STREAM_ACTIVE = 1,
		STREAM_NO_IMAGE = 2,
	};

#ifndef GDNATIVE_LIBRARY
private:
#else
public:
#endif

	class ImgProcessingStorageClient : public Object {
		GD_CLASS(ImgProcessingStorageClient, Object);

	public:
		PoolByteArray tex_data;
		uint64_t framerate = 0;
		int format = 0;
		ImageCompressionType compression_type = ImageCompressionType::COMPRESSION_UNCOMPRESSED;
		Size2 size;
		bool _is_processing_img = false;
		bool _thread_closing = false;

		static void _register_methods(){};
		void _init() {
			LEAVE_IF_EDITOR();
			tex_data = PoolByteArray();
		};

		~ImgProcessingStorageClient() {
			LEAVE_IF_EDITOR();
			tex_data.resize(0);
		}
	};

	class ConnectionThreadParamsClient : public Object {
		GD_CLASS(ConnectionThreadParamsClient, Object);

	public:
		Ref<StreamPeerTCP> peer;
		Ref<PacketPeerStream> ppeer;

		Ref<_Thread> thread_ref;

		bool break_connection = false;
		bool stop_thread = false;
		bool finished = false;

		void close_thread() {
			break_connection = true;
			stop_thread = true;
			Thread_close(thread_ref);
		}

		static void _register_methods(){};
		void _init(){};

		~ConnectionThreadParamsClient() {
			LEAVE_IF_EDITOR();
			close_thread();
			if (peer.is_valid()) {
				peer.unref();
			}
			if (ppeer.is_valid()) {
				ppeer.unref();
			}
		};
	};

private:
	const int default_decoder_threads_count = 3;

	bool is_deleting = false;
	bool is_connection_working = false;
	Node *settings_menu_node = nullptr;
	class Control *control_to_show_in = nullptr;
	class GRTextureRect *tex_shows_stream = nullptr;
	class GRInputCollector *input_collector = nullptr;
	ConnectionThreadParamsClient *thread_connection = nullptr;
	GRStreamDecodersManager *stream_manager = nullptr;
	uint64_t prev_shown_frame_time = 0;

	String device_id = "UNKNOWN";
	String server_address = String("127.0.0.1");

	StretchMode stretch_mode = StretchMode::STRETCH_KEEP_ASPECT;
	ScreenOrientation is_vertical = ScreenOrientation::NONE;
	ConnectionType connection_type = ConnectionType::CONNECTION_WiFi;
	StreamState signal_connection_state = StreamState::STREAM_NO_SIGNAL;

	String password;
	bool is_filtering_enabled = true;
	bool _viewport_orientation_syncing = true;
	bool _viewport_aspect_ratio_syncing = true;
	bool _server_settings_syncing = false;

	Mutex_define(connection_mutex, "Connection Lock");
	Mutex_define(stream_mutex, "Stream Manager Mutex");
	int input_buffer_size_in_mb = 4;
	int send_data_fps = 60;
	float _prev_stream_aspect_ratio = 0;

	uint64_t sync_time_client = 0;
	uint64_t sync_time_server = 0;
	GRAVGCounter<uint64_t, float> delay_counter = GRAVGCounter<uint64_t, float>([](float i) -> float {if (i > 0) return float(1000000.0 / i); else return 0; });

	// NO SIGNAL screen
	uint64_t prev_valid_connection_time = 0;
	bool no_signal_is_vertical = false;
	Ref<class Texture> custom_no_signal_texture;
	Ref<class Texture> custom_no_signal_vertical_texture;
	Ref<class Material> custom_no_signal_material;

#ifndef NO_GODOTREMOTE_DEFAULT_RESOURCES
	Ref<class Image> no_signal_image;
	Ref<class Image> no_signal_vertical_image;
	Ref<class ShaderMaterial> no_signal_mat;
#endif

	Node *custom_input_scene = nullptr;
	String custom_input_scene_tmp_pck_file = "user://custom_input_scene.pck";

	void _stop_decoder();
	void _push_pack_to_decoder(std::shared_ptr<GRPacketStreamData> pack);
	void _image_lost();
	void _display_new_image(Ref<Image> img, uint64_t delay);

	void _force_update_stream_viewport_signals();
	void _load_custom_input_scene(String path, PoolByteArray scene_data, int orig_size, bool is_compressed, int compression_type);
	void _remove_custom_input_scene();
	void _viewport_size_changed();
	void _on_node_deleting(int var_name);

	void _update_texture_from_image(Ref<Image> img);
	void _update_stream_texture_state(ENUM_ARG(StreamState) _stream_state);

	void _update_avg_delay(uint64_t delay);
	virtual void _reset_counters() override;

	void _thread_connection(Variant p_userdata);

	void _connection_loop(ConnectionThreadParamsClient *con_thread);
	GRDevice::AuthResult _auth_on_server(Ref<PacketPeerStream> &con);

protected:
	virtual void _internal_call_only_deffered_start() override;
	virtual void _internal_call_only_deffered_stop() override;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	void set_control_to_show_in(class Control *ctrl, int position_in_node DEF_ARG(= 0));
	void set_custom_no_signal_texture(Ref<Texture> custom_tex);
	void set_custom_no_signal_vertical_texture(Ref<Texture> custom_tex);
	void set_custom_no_signal_material(Ref<Material> custom_mat);

	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);
	bool is_capture_pointer();
	void set_capture_pointer(bool value);
	bool is_capture_input();
	void set_capture_input(bool value);
	void set_connection_type(ENUM_ARG(ConnectionType) type);
	ENUM_ARG(ConnectionType)
	get_connection_type();
	void set_target_send_fps(int fps);
	int get_target_send_fps();
	void set_stretch_mode(ENUM_ARG(StretchMode) stretch);
	ENUM_ARG(StretchMode)
	get_stretch_mode();
	void set_texture_filtering(bool is_filtering);
	bool get_texture_filtering();
	void set_viewport_orientation_syncing(bool is_syncing);
	bool is_viewport_orientation_syncing();
	void set_viewport_aspect_ratio_syncing(bool is_syncing);
	bool is_viewport_aspect_ratio_syncing();
	void set_server_settings_syncing(bool is_syncing);
	bool is_server_settings_syncing();
	void set_password(String _pass);
	String get_password();
	void set_device_id(String _id);
	String get_device_id();

	ENUM_ARG(StreamState)
	get_stream_state();
	bool is_stream_active();
	bool is_connected_to_host();
	Node *get_custom_input_scene();
	String get_address();
	bool set_address(String ip);
	bool set_address_port(String ip, uint16_t _port);
	void set_input_buffer_size(int mb);

	float get_avg_delay();
	float get_min_delay();
	float get_max_delay();
	float get_stream_aspect_ratio();

	void set_decoder_threads_count(int count);
	int get_decoder_threads_count();

	void set_server_setting(ENUM_ARG(TypesOfServerSettings) param, Variant value);
	void disable_overriding_server_settings();

	void _init();
	void _deinit();
};

class GRInputCollector : public Node {
	GD_CLASS(GRInputCollector, Node);
	friend GRClient;

	Mutex_define(ts_lock, "GRInputCollector Lock");

private:
	GRClient *dev = nullptr;
	GRInputCollector **this_in_client = nullptr; //somebody help

	class TextureRect *texture_rect = nullptr;
	std::vector<std::shared_ptr<GRInputData> > collected_input_data;
	class Control *parent;
	bool capture_only_when_control_in_focus = false;
	bool capture_pointer_only_when_hover_control = true;
	bool dont_capture_pointer = false;

	Rect2 stream_rect;
	PoolVector3Array sensors;

	Dictionary mouse_buttons;
	Dictionary screen_touches;

protected:
	void _collect_input(Ref<InputEvent> ie);
	void _update_stream_rect();
	void _release_pointers();

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _input(Ref<InputEvent> ie);
	void _notification(int p_notification);

public:
	bool is_capture_on_focus();
	void set_capture_on_focus(bool value);
	bool is_capture_when_hover();
	void set_capture_when_hover(bool value);
	bool is_capture_pointer();
	void set_capture_pointer(bool value);
	bool is_capture_input();
	void set_capture_input(bool value);

	void set_tex_rect(class TextureRect *tr);

	std::shared_ptr<GRPacketInputData> get_collected_input_data();

	void _init();
	void _deinit();
};

class GRTextureRect : public TextureRect {
	GD_CLASS(GRTextureRect, TextureRect);
	friend GRClient;

	GRClient *dev = nullptr;
	GRTextureRect **this_in_client = nullptr;
	void _tex_size_changed();

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	void _init();
	void _deinit();
};

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(GRClient::ConnectionType)
VARIANT_ENUM_CAST(GRClient::StretchMode)
VARIANT_ENUM_CAST(GRClient::StreamState)
#endif

#endif // NO_GODOTREMOTE_CLIENT
