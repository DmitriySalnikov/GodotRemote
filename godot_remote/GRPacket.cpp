/* GRPacket.cpp */
#include "GRPacket.h"
#include "GRInputData.h"
#include "core/os/os.h"

using namespace GRUtils;

Ref<GRPacket> GRPacket::create(const PoolByteArray &bytes) {
	if (bytes.empty()) {
		ERR_FAIL_V_MSG(Ref<GRPacket>(), "Can't create GRPacket from empty data!");
	}

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
		case PacketType::SyncTime:
			CREATE(GRPacketSyncTime);
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
// SYNC TIME

Ref<StreamPeerBuffer> GRPacketSyncTime::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var(OS::get_singleton()->get_ticks_usec());
	return buf;
}

bool GRPacketSyncTime::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	time = buf->get_var();
	return true;
}

uint64_t GRPacketSyncTime::get_time() {
	return time;
}

//////////////////////////////////////////////////////////////////////////
// IMAGE DATA
Ref<StreamPeerBuffer> GRPacketImageData::_get_data() {
	auto buf = GRPacket::_get_data();
	buf->put_var((int)compression);
	buf->put_var(size);
	buf->put_var(format);
	buf->put_var(img_data);
	buf->put_var(start_time); // send time
	buf->put_var(frametime);
	return buf;
}

bool GRPacketImageData::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	compression = (ImageCompressionType)(int)buf->get_var();
	size = buf->get_var();
	format = buf->get_var();
	img_data = buf->get_var();
	start_time = buf->get_var(); // send time
	frametime = buf->get_var();
	return true;
}

PoolByteArray GRPacketImageData::get_image_data() {
	return img_data;
}

int GRPacketImageData::get_compression_type() {
	return (int)compression;
}

uint64_t GRPacketImageData::get_start_time() {
	return start_time;
}

uint64_t GRPacketImageData::get_frametime() {
	return frametime;
}

Size2 GRPacketImageData::get_size() {
	return size;
}

int GRPacketImageData::get_format() {
	return format;
}

void GRPacketImageData::set_compression_type(int type) {
	compression = (ImageCompressionType)type;
}

void GRPacketImageData::set_start_time(uint64_t time) {
	start_time = time;
}

void GRPacketImageData::set_image_data(PoolByteArray &buf) {
	img_data = buf;
}

void GRPacketImageData::set_frametime(uint64_t _frametime) {
	frametime = _frametime;
}

void GRPacketImageData::set_size(Size2 _size) {
	size = _size;
}

void GRPacketImageData::set_format(int _format) {
	format = _format;
}

//////////////////////////////////////////////////////////////////////////
// INPUT DATA
Ref<StreamPeerBuffer> GRPacketInputData::_get_data() {
	auto buf = GRPacket::_get_data();
	int count = 0;

	for (int i = 0; i < inputs.size(); i++) {
		if (inputs[i].is_valid()) {
			count++;
		} else {
			inputs.remove(i);
			i--;
		}
	}
	buf->put_32(count);

	for (int i = 0; i < inputs.size(); i++) {
		buf->put_var(((Ref<GRInputData>)inputs[i])->get_data());
	}
	return buf;
}

bool GRPacketInputData::_create(Ref<StreamPeerBuffer> buf) {
	GRPacket::_create(buf);
	int size = buf->get_32(); // get size
	for (int i = 0; i < size; i++) {
		Ref<GRInputData> id = GRInputData::create(buf->get_var());
		if (id.is_null())
			return false;
		inputs.push_back(id);
	}
	return true;
}

int GRPacketInputData::get_inputs_count() {
	return inputs.size();
}

Ref<GRInputData> GRPacketInputData::get_input_data(int idx) {
	ERR_FAIL_INDEX_V(idx, inputs.size(), Ref<GRInputData>());
	return inputs[idx];
}

void GRPacketInputData::remove_input_data(int idx) {
	ERR_FAIL_INDEX(idx, inputs.size());
	inputs.remove(idx);
}

void GRPacketInputData::add_input_data(Ref<class GRInputData> &input) {
	inputs.push_back(input);
}

void GRPacketInputData::set_input_data(Vector<Ref<class GRInputData> > &_inputs) {
	inputs = _inputs;
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

Dictionary GRPacketServerSettings::get_settings() {
	return settings;
}

void GRPacketServerSettings::set_settings(Dictionary &_settings) {
	settings = _settings;
}

void GRPacketServerSettings::add_setting(int _setting, Variant value) {
	settings[_setting] = value;
}
