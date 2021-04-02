/* GRPacket.h */
#pragma once

#include "GRInputData.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#include "core/io/stream_peer.h"
#else
#include <StreamPeer.hpp>
#include <StreamPeerBuffer.hpp>
#endif

// TODO checking the checksum would be a good idea

class GRPacket {

public:
	enum PacketType : int {
		NonePacket = 0,
		SyncTime = 1,
		StreamDataImage = 2,
		InputData = 3,
		ServerSettings = 4,
		MouseModeSync = 5,
		CustomInputScene = 6,
		ClientStreamOrientation = 7,
		StreamAspectRatio = 8,
		CustomUserData = 9,
		StreamDataH264 = 10,
		StreamData = 11, // abstract
		ServerStreamQualityHint = 12,

		// Requests
		Ping = 128,

		// Responses
		Pong = 192,
	};

protected:
	virtual Ref<StreamPeerBuffer> _get_data() {
		Ref<StreamPeerBuffer> buf = newref(StreamPeerBuffer);
		buf->put_8((uint8_t)get_type());
		return buf;
	};
	virtual bool _create(Ref<StreamPeerBuffer> buf) {
		buf->get_8();
		return true;
	};

public:
	virtual PacketType get_type() { return PacketType::NonePacket; };
	static std::shared_ptr<GRPacket> create(const PoolByteArray &bytes);
	PoolByteArray get_data() {
		return _get_data()->get_data_array();
	};
};

//////////////////////////////////////////////////////////////////////////
// SyncTime
class GRPacketSyncTime : public GRPacket {
	friend GRPacket;

	uint64_t time = 0;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::SyncTime; };

	uint64_t get_time() {
		return time;
	}
};

//////////////////////////////////////////////////////////////////////////
// STREAM DATA
class GRPacketStreamData : public GRPacket {
	friend GRPacket;

	/*GRDevice::ImageCompressionType*/ int compression = 1 /*GRDevice::ImageCompressionType::JPG*/;
	bool is_stream_end = false;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::StreamData; };

	int get_compression_type() {
		return (int)compression;
	}
	bool get_is_stream_end() {
		return is_stream_end;
	}

	void set_compression_type(int type) {
		compression = type;
	}
	void set_is_stream_end(bool _empty) {
		is_stream_end = _empty;
	}
};

//////////////////////////////////////////////////////////////////////////
// STREAM DATA IMAGE
class GRPacketStreamDataImage : public GRPacketStreamData {
	friend GRPacket;

	Size2 size;
	int format = 0; // TODO can be removed in next minor version
	PoolByteArray img_data;
	uint64_t start_time = 0;
	uint64_t frametime = 0;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::StreamDataImage; };

	PoolByteArray get_image_data() {
		return img_data;
	}
	Size2 get_size() {
		return size;
	}
	uint64_t get_frametime() {
		return frametime;
	}
	uint64_t get_start_time() {
		return start_time;
	}

	void set_image_data(PoolByteArray &buf) {
		img_data = buf;
	}
	void set_size(Size2 _size) {
		size = _size;
	}
	void set_frametime(uint64_t _frametime) {
		frametime = _frametime;
	}
	void set_start_time(uint64_t _start_time) {
		start_time = _start_time;
	}
};

//////////////////////////////////////////////////////////////////////////
// STREAM DATA H264
class GRPacketStreamDataH264 : public GRPacketStreamData {
	friend GRPacket;

	std::vector<PoolByteArray> data_layers;
	uint64_t start_time = 0;
	uint8_t frame_type = 0;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::StreamDataH264; };

	std::vector<PoolByteArray> get_image_data() {
		return data_layers;
	}
	uint64_t get_start_time() {
		return start_time;
	}
	uint8_t get_frame_type() {
		return frame_type;
	}

	void add_image_data(uint8_t *buf, uint64_t size) {
		PoolByteArray img_data;
		img_data.resize((int)size);
		auto w = img_data.write();
		memcpy(w.ptr(), buf, size);
		data_layers.push_back(img_data);
	}

	void add_image_data(PoolByteArray buf) {
		data_layers.push_back(buf);
	}
	void set_start_time(uint64_t _start_time) {
		start_time = _start_time;
	}
	void set_frame_type(uint8_t type) {
		frame_type = type;
	}
};

//////////////////////////////////////////////////////////////////////////
// INPUT DATA
class GRPacketInputData : public GRPacket {
	friend GRPacket;

	std::vector<std::shared_ptr<GRInputData> > inputs;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::InputData; };

	int get_inputs_count() {
		return (int)inputs.size();
	}

	std::shared_ptr<GRInputData> get_input_data(int idx) {
		ERR_FAIL_INDEX_V(idx, (int)inputs.size(), std::shared_ptr<GRInputData>());
		return inputs[idx];
	}

	void remove_input_data(int idx) {
		ERR_FAIL_INDEX(idx, (int)inputs.size());
		inputs.erase(inputs.begin() + idx);
	}

	void add_input_data(std::shared_ptr<GRInputData> &input) {
		inputs.push_back(input);
	}

	void set_input_data(std::vector<std::shared_ptr<GRInputData> > &_inputs) {
		inputs = _inputs;
	}
};

//////////////////////////////////////////////////////////////////////////
// SERVER SETTINGS
class GRPacketServerSettings : public GRPacket {
	friend GRPacket;

	std::map<int, Variant> settings;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::ServerSettings; };

	std::map<int, Variant> get_settings() {
		return settings;
	}

	void set_settings(std::map<int, Variant> &_settings) {
		settings = _settings;
	}

	void add_setting(int _setting, Variant value) {
		settings[_setting] = value;
	}
};

//////////////////////////////////////////////////////////////////////////
// MOUSE MODE SYNC
class GRPacketMouseModeSync : public GRPacket {
	friend GRPacket;

	Input::MouseMode mouse_mode;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::MouseModeSync; };

	Input::MouseMode get_mouse_mode() {
		return mouse_mode;
	}

	void set_mouse_mode(Input::MouseMode _mode) {
		mouse_mode = _mode;
	}
};

//////////////////////////////////////////////////////////////////////////
// CUSTOM INPUT SCENE
class GRPacketCustomInputScene : public GRPacket {
	friend GRPacket;

	String scene_path;
	bool compressed;
	int compression_type;
	int original_data_size;
	PoolByteArray scene_data;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::CustomInputScene; };

	PoolByteArray get_scene_data() {
		return scene_data;
	}
	String get_scene_path() {
		return scene_path;
	}
	int get_original_size() {
		return original_data_size;
	}
	bool is_compressed() {
		return compressed;
	}
	int get_compression_type() {
		return compression_type;
	}

	void set_scene_data(PoolByteArray _data) {
		scene_data = _data;
	}
	void set_scene_path(String _path) {
		scene_path = _path;
	}
	void set_original_size(int val) {
		original_data_size = val;
	}
	void set_compressed(bool val) {
		compressed = val;
	}
	void set_compression_type(int val) {
		compression_type = val;
	}
};

//////////////////////////////////////////////////////////////////////////
// CLIENT DEVICE ROTATION
class GRPacketClientStreamOrientation : public GRPacket {
	friend GRPacket;

	bool vertical;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::ClientStreamOrientation; };

	bool is_vertical() {
		return vertical;
	}

	void set_vertical(bool val) {
		vertical = val;
	}
};

//////////////////////////////////////////////////////////////////////////
// CLIENT SCREEN ASCPECT
class GRPacketStreamAspectRatio : public GRPacket {
	friend GRPacket;

	float stream_aspect;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::StreamAspectRatio; };

	float get_aspect() {
		return stream_aspect;
	}

	void set_aspect(float val) {
		stream_aspect = val;
	}
};

//////////////////////////////////////////////////////////////////////////
// SERVER STREAM QUALITY HINT
class GRPacketServerStreamQualityHint : public GRPacket {
	friend GRPacket;

	String quality_hint;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::ServerStreamQualityHint; };

	String get_hint() {
		return quality_hint;
	}

	void set_hint(String val) {
		quality_hint = val;
	}
};

//////////////////////////////////////////////////////////////////////////
// CUSTOM USER DATA
class GRPacketCustomUserData : public GRPacket {
	friend GRPacket;

	Variant packet_id;
	bool full_objects = false;
	Variant user_data;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::CustomUserData; };

	Variant get_packet_id() {
		return packet_id;
	}

	void set_packet_id(Variant val) {
		packet_id = val;
	}

	bool get_send_full_objects() {
		return full_objects;
	}

	void set_send_full_objects(bool val) {
		full_objects = val;
	}

	Variant get_user_data() {
		return user_data;
	}

	void set_user_data(Variant val) {
		user_data = val;
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// REQUESTS AND RESPONSES

#define BASIC_PACKET(_name, _type)                                                            \
	class _name : public GRPacket {                                                           \
		friend GRPacket;                                                                      \
                                                                                              \
	protected:                                                                                \
		virtual Ref<StreamPeerBuffer> _get_data() override { return GRPacket::_get_data(); }; \
		virtual bool _create(Ref<StreamPeerBuffer> buf) override { return true; };            \
                                                                                              \
	public:                                                                                   \
		virtual PacketType get_type() override { return _type; };                             \
	}

BASIC_PACKET(GRPacketPing, PacketType::Ping);
BASIC_PACKET(GRPacketPong, PacketType::Pong);

#undef BASIC_PACKET
