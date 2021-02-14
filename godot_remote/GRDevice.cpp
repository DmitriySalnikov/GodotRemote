/* GRDevice.cpp */
#include "GRDevice.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY
#else
#include <ClassDB.hpp>
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY

void GRDevice::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_internal_call_only_deffered_start"), &GRDevice::_internal_call_only_deffered_start);
	ClassDB::bind_method(D_METHOD("_internal_call_only_deffered_stop"), &GRDevice::_internal_call_only_deffered_stop);

	ClassDB::bind_method(D_METHOD("_internal_call_only_deffered_restart"), &GRDevice::_internal_call_only_deffered_restart);

	ClassDB::bind_method(D_METHOD("get_avg_ping"), &GRDevice::get_avg_ping);
	ClassDB::bind_method(D_METHOD("get_avg_fps"), &GRDevice::get_avg_fps);

	ClassDB::bind_method(D_METHOD("get_port"), &GRDevice::get_port);
	ClassDB::bind_method(D_METHOD("set_port", "port"), &GRDevice::set_port, DEFVAL(52341));

	//ClassDB::bind_method(D_METHOD("send_packet", "packet"), &GRDevice::send_packet);
	ClassDB::bind_method(D_METHOD("send_user_data", "packet_id", "user_data", "full_objects"), &GRDevice::send_user_data, DEFVAL(false));

	ClassDB::bind_method(D_METHOD("start"), &GRDevice::start);
	ClassDB::bind_method(D_METHOD("stop"), &GRDevice::stop);
	ClassDB::bind_method(D_METHOD("get_status"), &GRDevice::get_status);

	ADD_SIGNAL(MethodInfo("status_changed", PropertyInfo(Variant::INT, "status")));
	ADD_SIGNAL(MethodInfo("user_data_received", PropertyInfo(Variant::NIL, "packet_id"), PropertyInfo(Variant::NIL, "user_data")));

	ADD_PROPERTY(PropertyInfo(Variant::INT, "port"), "set_port", "get_port");

	BIND_ENUM_CONSTANT(STATUS_STARTING);
	BIND_ENUM_CONSTANT(STATUS_STOPPING);
	BIND_ENUM_CONSTANT(STATUS_WORKING);
	BIND_ENUM_CONSTANT(STATUS_STOPPED);

	BIND_ENUM_CONSTANT(SERVER_SETTINGS_USE_INTERNAL);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_VIDEO_STREAM_ENABLED);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_COMPRESSION_TYPE);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_JPG_QUALITY);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_SKIP_FRAMES);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_RENDER_SCALE);

	BIND_ENUM_CONSTANT(SUBSAMPLING_Y_ONLY);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H1V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V1);
	BIND_ENUM_CONSTANT(SUBSAMPLING_H2V2);

	BIND_ENUM_CONSTANT(COMPRESSION_UNCOMPRESSED);
	BIND_ENUM_CONSTANT(COMPRESSION_JPG);
	BIND_ENUM_CONSTANT(COMPRESSION_PNG);
}

#else

void GRDevice::_register_methods() {
	METHOD_REG(GRDevice, _notification);

	METHOD_REG(GRDevice, _internal_call_only_deffered_start);
	METHOD_REG(GRDevice, _internal_call_only_deffered_stop);

	METHOD_REG(GRDevice, _internal_call_only_deffered_restart);

	METHOD_REG(GRDevice, get_avg_ping);
	METHOD_REG(GRDevice, get_avg_fps);

	METHOD_REG(GRDevice, get_port);
	METHOD_REG(GRDevice, set_port);

	//METHOD_REG(GRDevice, send_packet);
	METHOD_REG(GRDevice, send_user_data);

	METHOD_REG(GRDevice, start);
	METHOD_REG(GRDevice, stop);
	METHOD_REG(GRDevice, get_status);

	register_signal<GRDevice>("status_changed", "status", GODOT_VARIANT_TYPE_INT);
	register_signal<GRDevice>("user_data_received", "packet_id", GODOT_VARIANT_TYPE_NIL, "user_data", GODOT_VARIANT_TYPE_NIL);

	register_property<GRDevice, uint16_t>("port", &GRDevice::set_port, &GRDevice::get_port, 52341);
}

#endif

void GRDevice::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			break;
	}
}

void GRDevice::_reset_counters() {
	avg_fps = 0;
	avg_ping = 0;
}

void GRDevice::_update_avg_ping(uint64_t ping) {
	if (!ping) {
		avg_ping = 0;
		return;
	}
	avg_ping = (avg_ping * avg_ping_smoothing) + ((float)(ping / 1000.f) * (1.f - avg_ping_smoothing));
}

void GRDevice::_update_avg_fps(uint64_t frametime) {
	if (!frametime) {
		avg_fps = 0;
		return;
	}
	avg_fps = (avg_fps * avg_fps_smoothing) + ((float)(1000000.f / frametime) * (1.f - avg_fps_smoothing));
}

void GRDevice::send_user_data(Variant packet_id, Variant user_data, bool full_objects) {
	send_queue_mutex->lock();
	Ref<GRPacketCustomUserData> packet = newref(GRPacketCustomUserData);
	send_packet(packet);

	packet->set_packet_id(packet_id);
	packet->set_send_full_objects(full_objects);
	packet->set_user_data(user_data);

	send_queue_mutex->unlock();
}

void GRDevice::_send_queue_resize(int new_size) {
	send_queue_mutex->lock();
	send_queue.resize(new_size);
	send_queue_mutex->unlock();
}

Ref<GRPacket> GRDevice::_send_queue_pop_front() {
	send_queue_mutex->lock();
	Ref<GRPacket> packet;
	if (send_queue.size() > 0) {
		packet = send_queue.front();
		send_queue.erase(send_queue.begin());
	}
	send_queue_mutex->unlock();
	return packet;
}

void GRDevice::set_status(WorkingStatus status) {
	working_status = status;
	emit_signal("status_changed", working_status);
}

float GRDevice::get_avg_ping() {
	return avg_ping;
}

float GRDevice::get_avg_fps() {
	return avg_fps;
}

uint16_t GRDevice::get_port() {
	return port;
}

void GRDevice::set_port(uint16_t _port) {
	port = _port;
	restart();
}

void GRDevice::send_packet(Ref<GRPacket> packet) {
	ERR_FAIL_COND(packet.is_null());

	send_queue_mutex->lock();
	if (send_queue.size() > 10000)
		send_queue.resize(0);

	send_queue.push_back(packet);
	send_queue_mutex->unlock();
}

void GRDevice::start() {
	call_deferred("_internal_call_only_deffered_start");
}

void GRDevice::stop() {
	call_deferred("_internal_call_only_deffered_stop");
}

void GRDevice::restart() {
	call_deferred("_internal_call_only_deffered_restart");
}

void GRDevice::_internal_call_only_deffered_restart() {
	if (get_status() == (int)WorkingStatus::STATUS_WORKING) {
		_internal_call_only_deffered_stop();
		_internal_call_only_deffered_start();
	}
}

GRDevice::WorkingStatus GRDevice::get_status() {
	return working_status;
}

void GRDevice::_init() {
	LEAVE_IF_EDITOR();
	port = GET_PS(GodotRemote::ps_general_port_name);

	if (send_queue_mutex)
		memdelete(send_queue_mutex);
	send_queue_mutex = Mutex_create();
}

void GRDevice::_deinit() {
	LEAVE_IF_EDITOR();
	if (GodotRemote::get_singleton()) {
		GodotRemote::get_singleton()->device = nullptr;
	}
}
