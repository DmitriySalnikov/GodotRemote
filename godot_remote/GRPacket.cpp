/* GRPacket.cpp */
#include "GRPacket.h"

using namespace GRUtils;

Ref<GRPacket> GRPacket::create(const PoolByteArray &bytes) {
	PacketType type = (PacketType)bytes[0];
	Ref<StreamPeerBuffer> buf(memnew(StreamPeerBuffer));
	buf->set_data_array(bytes);

#define CREATE(type)                    \
	{                                   \
		Ref<type> packet(memnew(type)); \
		if (packet->_create(buf)) {     \
			return packet;              \
		} else {                        \
			return Ref<GRPacket>();     \
		}                               \
	}

	switch (type) {
		case PacketType::None:
			ERR_FAIL_V_MSG(Ref<GRPacket>(), "Can't create abstract GRPacket!");
		case PacketType::InitData:
			break;
		case PacketType::ImageData:
			CREATE(GRPacketImageData);
		case PacketType::InputData:
			CREATE(GRPacketInputData);
		case PacketType::ServerSettings:
			CREATE(GRPacketServerSettings);
		case PacketType::ServerSettingsRequest:
			CREATE(GRPacketServerSettingsRequest);
		case PacketType::Ping:
			CREATE(GRPacketPing);
		case PacketType::Pong:
			CREATE(GRPacketPong);
		default:
			ERR_FAIL_V_MSG(Ref<GRPacket>(), "Can't create unknown GRPacket!");
	}
#undef CREATE
	return Ref<GRPacket>();
}

//////////////////////////////////////////////////////////////////////////
// IMAGE DATA
Ref<StreamPeerBuffer> GRPacketImageData::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var(img_data);
	return buf;
}

bool GRPacketImageData::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);

	img_data = buf->get_var();
	return true;
}

PoolByteArray &GRPacketImageData::get_image_data() {
	return img_data;
}

void GRPacketImageData::set_image_data(PoolByteArray &buf) {
	img_data = buf;
}

//////////////////////////////////////////////////////////////////////////
// INPUT DATA
Ref<StreamPeerBuffer> GRPacketInputData::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var(input_data);
	return buf;
}

bool GRPacketInputData::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	input_data = buf->get_var();
	return true;
}

PoolByteArray &GRPacketInputData::get_input_data() {
	return input_data;
}

void GRPacketInputData::set_input_data(PoolByteArray &buf) {
	input_data = buf;
}

//////////////////////////////////////////////////////////////////////////
// SERVER SETTINGS
Ref<StreamPeerBuffer> GRPacketServerSettings::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var(settings);
	return buf;
}

bool GRPacketServerSettings::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	settings = buf->get_var();
	return true;
}

Dictionary &GRPacketServerSettings::get_settings() {
	return settings;
}

void GRPacketServerSettings::set_settings(Dictionary &_settings) {
	settings = _settings;
}

void GRPacketServerSettings::add_setting(int _setting, Variant value) {
	settings[_setting] = value;
}
