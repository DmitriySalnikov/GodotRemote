/* GRPacket.cpp */
#include "GRPacket.h"

#ifndef GDNATIVE_LIBRARY
#else
using namespace godot;
#endif

using namespace GRUtils;

std::shared_ptr<GRPacket> GRPacket::create(const PoolByteArray &bytes) {
	if (bytes.size() == 0) {
		ERR_FAIL_V_MSG(std::shared_ptr<GRPacket>(), "Can't create GRPacket from empty data!");
	}

	PacketType type = (PacketType)((PoolByteArray)bytes)[0];
	Ref<StreamPeerBuffer> buf = newref(StreamPeerBuffer);
	buf->set_data_array(bytes);

#define CREATE(type)                                     \
	{                                                    \
		std::shared_ptr<type> packet = shared_new(type); \
		if (packet->_create(buf)) {                      \
			return packet;                               \
		} else {                                         \
			return std::shared_ptr<GRPacket>();          \
		}                                                \
	}

	switch (type) {
		case PacketType::NonePacket:
		case PacketType::StreamData:
			ERR_FAIL_V_MSG(std::shared_ptr<GRPacket>(), "Can't create abstract GRPacket!");
		case PacketType::SyncTime:
			CREATE(GRPacketSyncTime);
		case PacketType::StreamDataImage:
			CREATE(GRPacketStreamDataImage);
		case PacketType::InputData:
			CREATE(GRPacketInputData);
		case PacketType::ServerSettings:
			CREATE(GRPacketServerSettings);
		case PacketType::MouseModeSync:
			CREATE(GRPacketMouseModeSync);
		case PacketType::CustomInputScene:
			CREATE(GRPacketCustomInputScene);
		case PacketType::ClientStreamOrientation:
			CREATE(GRPacketClientStreamOrientation);
		case PacketType::StreamAspectRatio:
			CREATE(GRPacketStreamAspectRatio);
		case PacketType::CustomUserData:
			CREATE(GRPacketCustomUserData);
		case PacketType::StreamDataH264:
			CREATE(GRPacketStreamDataH264);
		case PacketType::ServerStreamQualityHint:
			CREATE(GRPacketServerStreamQualityHint);

			// Requests
		case PacketType::Ping:
			CREATE(GRPacketPing);

			// Responses
		case PacketType::Pong:
			CREATE(GRPacketPong);
		default:
			ERR_FAIL_V_MSG(std::shared_ptr<GRPacket>(), "Can't create unknown GRPacket! Type: " + str((int)type));
	}
#undef CREATE
	return std::shared_ptr<GRPacket>();
}

//////////////////////////////////////////////////////////////////////////
// STREAM DATA
Ref<StreamPeerBuffer> GRPacketStreamData::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_8(is_stream_end);
	buf->put_32((int)compression);
	return buf;
}

bool GRPacketStreamData::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	is_stream_end = (bool)buf->get_8();
	compression = (int)buf->get_32();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// STREAM DATA IMAGE
Ref<StreamPeerBuffer> GRPacketStreamDataImage::_get_data() {
	auto buf = GRPacketStreamData::_get_data();
	buf->put_var(size);
	buf->put_var(format);
	buf->put_var(img_data);
	buf->put_var(start_time);
	buf->put_var(frametime);
	return buf;
}

bool GRPacketStreamDataImage::_create(Ref<StreamPeerBuffer> buf) {
	GRPacketStreamData::_create(buf);
	size = buf->get_var();
	format = buf->get_var();
	img_data = buf->get_var();
	start_time = buf->get_var();
	frametime = buf->get_var();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// STREAM DATA H264
Ref<StreamPeerBuffer> GRPacketStreamDataH264::_get_data() {
	auto buf = GRPacketStreamData::_get_data();
	buf->put_var(start_time);
	buf->put_var(frametime);
	buf->put_var(frame_type);
	buf->put_var(format);
	buf->put_var(additional_data);

	// store array size
	buf->put_var((int)data_layers.size());
	// store all arrays data if exists
	for (auto i : data_layers) {
		buf->put_var(i);
	}
	return buf;
}

bool GRPacketStreamDataH264::_create(Ref<StreamPeerBuffer> buf) {
	GRPacketStreamData::_create(buf);
	start_time = buf->get_var();
	frametime = buf->get_var();
	frame_type = buf->get_var();
	format = buf->get_var();
	additional_data = buf->get_var();

	// get array size
	int count = buf->get_var();
	// get array data if count > 0
	for (int i = 0; i < count; i++) {
		data_layers.push_back(buf->get_var());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// SYNC TIME
Ref<StreamPeerBuffer> GRPacketSyncTime::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var(get_time_usec());
	return buf;
}

bool GRPacketSyncTime::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	time = buf->get_var();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// INPUT DATA
Ref<StreamPeerBuffer> GRPacketInputData::_get_data() {
	auto buf = GRPacket::_get_data();
	int count = 0;

	for (int i = 0; i < (int)inputs.size(); i++) {
		std::shared_ptr<GRInputData> inp = inputs[i];
		if (inp) {
			count++;
		} else {
			//inputs.remove(i);
			inputs.erase(inputs.begin() + i);
			i--;
		}
	}
	buf->put_32(count);

	for (int i = 0; i < (int)inputs.size(); i++) {
		buf->put_var(((std::shared_ptr<GRInputData>)inputs[i])->get_data());
	}
	return buf;
}

bool GRPacketInputData::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	int size = (int)buf->get_32(); // get size
	for (int i = 0; i < size; i++) {
		std::shared_ptr<GRInputData> id = GRInputData::create(buf->get_var());
		if (!id)
			return false;
		inputs.push_back(id);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// SERVER SETTINGS
Ref<StreamPeerBuffer> GRPacketServerSettings::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var(map_to_dict(settings));
	return buf;
}

bool GRPacketServerSettings::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	settings = dict_to_map<int, Variant>(buf->get_var());
	return true;
}

//////////////////////////////////////////////////////////////////////////
// MOUSE MODE SYNC

Ref<StreamPeerBuffer> GRPacketMouseModeSync::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_8(mouse_mode);
	return buf;
}

bool GRPacketMouseModeSync::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	mouse_mode = (Input::MouseMode)buf->get_8();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CUSTOM INPUT SCENE

Ref<StreamPeerBuffer> GRPacketCustomInputScene::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_string(scene_path);
	buf->put_8(compressed);
	buf->put_8(compression_type);
	buf->put_32(original_data_size);
	buf->put_var(scene_data);
	return buf;
}

bool GRPacketCustomInputScene::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	scene_path = buf->get_string();
	compressed = buf->get_8();
	compression_type = (uint8_t)buf->get_8();
	original_data_size = (int)buf->get_32();
	scene_data = buf->get_var();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CLIENT DEVICE ROTATION

Ref<StreamPeerBuffer> GRPacketClientStreamOrientation::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_8(vertical);
	return buf;
}

bool GRPacketClientStreamOrientation::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	vertical = buf->get_8();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CLIENT SCREEN ASCPECT

Ref<StreamPeerBuffer> GRPacketStreamAspectRatio::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_float(stream_aspect);
	return buf;
}

bool GRPacketStreamAspectRatio::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	stream_aspect = buf->get_float();
	return true;
}

//////////////////////////////////////////////////////////////////////////
// CUSTOM USER DATA

Ref<StreamPeerBuffer> GRPacketCustomUserData::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var(packet_id);
	buf->put_8(full_objects);
	buf->put_var(user_data, full_objects);
	return buf;
}

bool GRPacketCustomUserData::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	packet_id = buf->get_var();
	full_objects = buf->get_8();
	user_data = buf->get_var(full_objects);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// SERVER STREAM QUALITY HINT

Ref<StreamPeerBuffer> GRPacketServerStreamQualityHint::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_string(quality_hint);
	return buf;
}

bool GRPacketServerStreamQualityHint::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	quality_hint = buf->get_string();
	return true;
}
