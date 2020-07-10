/* GRPacket.h */
#pragma once

#include "GRUtils.h"
#include "core/io/stream_peer.h"
#include "core/reference.h"

class GRPacket : public Reference {
	GDCLASS(GRPacket, Reference);

public:
	enum class PacketType {
		None = 0,
		InitData = 1,
		ImageData = 2,
		InputData = 3,
		ServerSettings = 4,
		ServerSettingsRequest = 253,
		Ping = 254,
		Pong = 255,
	};

protected:
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
	static Ref<GRPacket> create(const PoolByteArray &bytes);
	PoolByteArray get_data() {
		return _get_data()->get_data_array();
	};
	virtual PacketType get_type() { return PacketType::None; };
};

//////////////////////////////////////////////////////////////////////////
// ImageData
class GRPacketImageData : public GRPacket {
	GDCLASS(GRPacketImageData, GRPacket);
	friend GRPacket;

	PoolByteArray img_data;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	PoolByteArray &get_image_data();
	void set_image_data(PoolByteArray &buf);

	virtual PacketType get_type() override { return PacketType::ImageData; };
};

//////////////////////////////////////////////////////////////////////////
// InputData
class GRPacketInputData : public GRPacket {
	GDCLASS(GRPacketInputData, GRPacket);
	friend GRPacket;

	PoolByteArray input_data;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	PoolByteArray &get_input_data();
	void set_input_data(PoolByteArray &buf);

	virtual PacketType get_type() override { return PacketType::InputData; };
};

//////////////////////////////////////////////////////////////////////////
// ServerSettings
class GRPacketServerSettings : public GRPacket {
	GDCLASS(GRPacketServerSettings, GRPacket);
	friend GRPacket;

	Dictionary settings;

protected:
	virtual Ref<StreamPeerBuffer> _get_data() override;
	virtual bool _create(Ref<StreamPeerBuffer> buf) override;

public:
	Dictionary &get_settings();
	void set_settings(Dictionary &_settings);
	void add_setting(int _setting, Variant value);

	virtual PacketType get_type() override { return PacketType::ServerSettings; };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// REQUESTS!

#define BASIC_PACKET(_name, _type)                                                            \
	class _name : public GRPacket {                                                           \
		GDCLASS(_name, GRPacket);                                                             \
		friend GRPacket;                                                                      \
                                                                                              \
	protected:                                                                                \
		virtual Ref<StreamPeerBuffer> _get_data() override { return GRPacket::_get_data(); }; \
		virtual bool _create(Ref<StreamPeerBuffer> buf) override { return true; };            \
                                                                                              \
	public:                                                                                   \
		virtual PacketType get_type() override { return _type; };                             \
	}

BASIC_PACKET(GRPacketServerSettingsRequest, PacketType::ServerSettingsRequest);
BASIC_PACKET(GRPacketPing, PacketType::Ping);
BASIC_PACKET(GRPacketPong, PacketType::Pong);

#undef BASIC_PACKET
