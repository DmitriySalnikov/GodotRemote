/* GRPacket.h */
#pragma once

#include "GRInputData.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY

#include "core/io/stream_peer.h"
#include "core/reference.h"
#else

#include <Array.hpp>
#include <Godot.hpp>
#include <PoolArrays.hpp>
#include <Ref.hpp>
#include <Reference.hpp>
#include <StreamPeer.hpp>
#include <StreamPeerBuffer.hpp>
#include <String.hpp>
#endif

class GRPacket : public Reference {
	GD_CLASS(GRPacket, Reference);

public:
	enum PacketType : int {
		NonePacket = 0,
		SyncTime = 1,
		ImageData = 2,
		InputData = 3,
		ServerSettings = 4,
		MouseModeSync = 5,
		CustomInputScene = 6,
		ClientStreamOrientation = 7,
		ClientStreamAspect = 8,
		CustomUserData = 9,

		// Requests
		Ping = 128,

		// Responses
		Pong = 192,
	};

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods() {
		BIND_ENUM_CONSTANT(NonePacket);
		BIND_ENUM_CONSTANT(SyncTime);
		BIND_ENUM_CONSTANT(ImageData);
		BIND_ENUM_CONSTANT(InputData);
		BIND_ENUM_CONSTANT(ServerSettings);
		BIND_ENUM_CONSTANT(MouseModeSync);
		BIND_ENUM_CONSTANT(CustomInputScene);
		BIND_ENUM_CONSTANT(ClientStreamOrientation);
		BIND_ENUM_CONSTANT(ClientStreamAspect);
		BIND_ENUM_CONSTANT(CustomUserData);
		BIND_ENUM_CONSTANT(Ping);
		BIND_ENUM_CONSTANT(Pong);
	}
#else
public:
	void _init(){};
	static void _register_methods(){};
protected:
#endif

	virtual Ref<StreamPeerBuffer> _get_data() {
		Ref<StreamPeerBuffer> buf(memnew(StreamPeerBuffer));
		buf->put_8((uint8_t)get_type());
		return buf;
	};
	virtual bool _create(Ref<StreamPeerBuffer> buf) {
		buf->get_8();
		return true;
	};

public:
	virtual PacketType get_type() { return PacketType::NonePacket; };
	static Ref<GRPacket> create(const PoolByteArray &bytes);
	PoolByteArray get_data() {
		return _get_data()->get_data_array();
	};
};

//////////////////////////////////////////////////////////////////////////
// SyncTime
class GRPacketSyncTime : public GRPacket {
	GD_S_CLASS(GRPacketSyncTime, GRPacket);
	friend GRPacket;

	uint64_t time = 0;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::SyncTime; };

	uint64_t get_time();
};

//////////////////////////////////////////////////////////////////////////
// IMAGE DATA
class GRPacketImageData : public GRPacket {
	GD_S_CLASS(GRPacketImageData, GRPacket);
	friend GRPacket;

	/*GRDevice::ImageCompressionType*/int compression = 1 /*GRDevice::ImageCompressionType::JPG*/;
	Size2 size;
	int format = 0;
	PoolByteArray img_data;
	uint64_t start_time = 0;
	uint64_t frametime = 0;
	bool is_empty = false;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::ImageData; };

	PoolByteArray get_image_data();
	int get_compression_type();
	Size2 get_size();
	int get_format();
	uint64_t get_start_time();
	uint64_t get_frametime();
	bool get_is_empty();

	void set_image_data(PoolByteArray &buf);
	void set_compression_type(int type);
	void set_size(Size2 _size);
	void set_format(int _format);
	void set_start_time(uint64_t time);
	void set_frametime(uint64_t _frametime);
	void set_is_empty(bool _empty);

	~GRPacketImageData() {
		//img_data.resize(0);
	}
};

//////////////////////////////////////////////////////////////////////////
// INPUT DATA
class GRPacketInputData : public GRPacket {
	GD_S_CLASS(GRPacketInputData, GRPacket);
	friend GRPacket;

	std::vector<Ref<GRInputData> > inputs;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::InputData; };

	int get_inputs_count();
	Ref<class GRInputData> get_input_data(int idx);
	void remove_input_data(int idx);
	void add_input_data(Ref<GRInputData> &input);
	void set_input_data(std::vector<Ref<GRInputData> > &_inputs); // Ref<GRInputData>

	~GRPacketInputData() {
		//inputs.clear();
	}
};

//////////////////////////////////////////////////////////////////////////
// SERVER SETTINGS
class GRPacketServerSettings : public GRPacket {
	GD_S_CLASS(GRPacketServerSettings, GRPacket);
	friend GRPacket;

	std::map<int, Variant> settings;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::ServerSettings; };

	std::map<int, Variant> get_settings();
	void set_settings(std::map<int, Variant> &_settings);
	void add_setting(int _setting, Variant value);

	~GRPacketServerSettings() {
		//settings.clear();
	}
};

//////////////////////////////////////////////////////////////////////////
// MOUSE MODE SYNC
class GRPacketMouseModeSync : public GRPacket {
	GD_S_CLASS(GRPacketMouseModeSync, GRPacket);
	friend GRPacket;

	Input::MouseMode mouse_mode;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::MouseModeSync; };

	Input::MouseMode get_mouse_mode();
	void set_mouse_mode(Input::MouseMode _mode);
};

//////////////////////////////////////////////////////////////////////////
// CUSTOM INPUT SCENE
class GRPacketCustomInputScene : public GRPacket {
	GD_S_CLASS(GRPacketCustomInputScene, GRPacket);
	friend GRPacket;

	String scene_path;
	bool compressed;
	int compression_type;
	int original_data_size;
	PoolByteArray scene_data;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::CustomInputScene; };

	String get_scene_path();
	void set_scene_path(String _path);
	PoolByteArray get_scene_data();
	void set_scene_data(PoolByteArray _data);
	bool is_compressed();
	void set_compressed(bool val);
	int get_original_size();
	void set_original_size(int val);
	int get_compression_type();
	void set_compression_type(int val);

	~GRPacketCustomInputScene() {
		//scene_data.resize(0);
	}
};

//////////////////////////////////////////////////////////////////////////
// CLIENT DEVICE ROTATION
class GRPacketClientStreamOrientation : public GRPacket {
	GD_S_CLASS(GRPacketClientStreamOrientation, GRPacket);
	friend GRPacket;

	bool vertical;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::ClientStreamOrientation; };

	bool is_vertical();
	void set_vertical(bool val);
};

//////////////////////////////////////////////////////////////////////////
// CLIENT SCREEN ASCPECT
class GRPacketClientStreamAspect : public GRPacket {
	GD_S_CLASS(GRPacketClientStreamAspect, GRPacket);
	friend GRPacket;

	float stream_aspect;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::ClientStreamAspect; };

	float get_aspect();
	void set_aspect(float val);
};

//////////////////////////////////////////////////////////////////////////
// CUSTOM USER DATA
class GRPacketCustomUserData : public GRPacket {
	GD_S_CLASS(GRPacketCustomUserData, GRPacket);
	friend GRPacket;

	Variant packet_id;
	bool full_objects = false;
	Variant user_data;

protected:
	GDNATIVE_BASIC_REGISTER;

	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	virtual PacketType get_type() override { return PacketType::CustomUserData; };

	Variant get_packet_id();
	void set_packet_id(Variant val);
	bool get_send_full_objects();
	void set_send_full_objects(bool val);
	Variant get_user_data();
	void set_user_data(Variant val);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// REQUESTS AND RESPONSES

#define BASIC_PACKET(_name, _type)                                                            \
	class _name : public GRPacket {                                                           \
		GD_S_CLASS(_name, GRPacket);                                                          \
		friend GRPacket;                                                                      \
                                                                                              \
	protected:                                                                                \
		GDNATIVE_BASIC_REGISTER;                                                              \
		virtual Ref<StreamPeerBuffer> _get_data() override { return GRPacket::_get_data(); }; \
		virtual bool _create(Ref<StreamPeerBuffer> buf) override { return true; };            \
                                                                                              \
	public:                                                                                   \
		virtual PacketType get_type() override { return _type; };                             \
	}

BASIC_PACKET(GRPacketPing, PacketType::Ping);
BASIC_PACKET(GRPacketPong, PacketType::Pong);

#undef BASIC_PACKET

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(GRPacket::PacketType)
#endif
