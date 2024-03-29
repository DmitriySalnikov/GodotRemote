/* GRDevice.cpp */
#include "GRDevice.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY
#else
using namespace godot;
#endif

using namespace GRUtils;

#ifndef GDNATIVE_LIBRARY

void GRDevice::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_internal_call_only_deffered_start)), &GRDevice::_internal_call_only_deffered_start);
	ClassDB::bind_method(D_METHOD(NAMEOF(_internal_call_only_deffered_stop)), &GRDevice::_internal_call_only_deffered_stop);
	ClassDB::bind_method(D_METHOD(NAMEOF(_internal_call_only_deffered_restart)), &GRDevice::_internal_call_only_deffered_restart);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_avg_ping)), &GRDevice::get_avg_ping);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_min_ping)), &GRDevice::get_min_ping);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_max_ping)), &GRDevice::get_max_ping);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_avg_fps)), &GRDevice::get_avg_fps);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_min_fps)), &GRDevice::get_min_fps);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_max_fps)), &GRDevice::get_max_fps);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_avg_recv_mbyte)), &GRDevice::get_avg_recv_mbyte);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_min_recv_mbyte)), &GRDevice::get_min_recv_mbyte);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_max_recv_mbyte)), &GRDevice::get_max_recv_mbyte);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_avg_send_mbyte)), &GRDevice::get_avg_send_mbyte);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_min_send_mbyte)), &GRDevice::get_min_send_mbyte);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_max_send_mbyte)), &GRDevice::get_max_send_mbyte);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_total_received_mbytes)), &GRDevice::get_total_received_mbytes);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_total_sended_mbytes)), &GRDevice::get_total_sended_mbytes);

	ClassDB::bind_method(D_METHOD(NAMEOF(get_port)), &GRDevice::get_port);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_port), "port"), &GRDevice::set_port, DEFVAL(PORT_STATIC_CONNECTION));
	ClassDB::bind_method(D_METHOD(NAMEOF(get_auto_connection_port)), &GRDevice::get_auto_connection_port);
	ClassDB::bind_method(D_METHOD(NAMEOF(set_auto_connection_port), "port"), &GRDevice::set_auto_connection_port, DEFVAL(PORT_AUTO_CONNECTION));

	//ClassDB::bind_method(D_METHOD(NAMEOF(send_packet), "packet"), &GRDevice::send_packet);
	ClassDB::bind_method(D_METHOD(NAMEOF(send_user_data), "packet_id", "user_data", "full_objects"), &GRDevice::send_user_data, DEFVAL(false));

	ClassDB::bind_method(D_METHOD(NAMEOF(restart)), &GRDevice::restart);
	ClassDB::bind_method(D_METHOD(NAMEOF(start)), &GRDevice::start);
	ClassDB::bind_method(D_METHOD(NAMEOF(stop)), &GRDevice::stop);
	ClassDB::bind_method(D_METHOD(NAMEOF(get_status)), &GRDevice::get_status);

	ADD_SIGNAL(MethodInfo("status_changed", PropertyInfo(Variant::INT, "status")));
	ADD_SIGNAL(MethodInfo("user_data_received", PropertyInfo(Variant::NIL, "packet_id"), PropertyInfo(Variant::NIL, "user_data")));

	ADD_PROPERTY(PropertyInfo(Variant::INT, "port"), NAMEOF(set_port), NAMEOF(get_port));
	ADD_PROPERTY(PropertyInfo(Variant::INT, "auto_connection_port"), NAMEOF(set_auto_connection_port), NAMEOF(get_auto_connection_port));

	BIND_ENUM_CONSTANT(STATUS_STARTING);
	BIND_ENUM_CONSTANT(STATUS_STOPPING);
	BIND_ENUM_CONSTANT(STATUS_WORKING);
	BIND_ENUM_CONSTANT(STATUS_STOPPED);

	BIND_ENUM_CONSTANT(SERVER_SETTINGS_USE_INTERNAL);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_VIDEO_STREAM_ENABLED);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_COMPRESSION_TYPE);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_STREAM_QUALITY);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_SKIP_FRAMES);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_RENDER_SCALE);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_TARGET_FPS);
	BIND_ENUM_CONSTANT(SERVER_SETTINGS_THREADS_NUMBER);

	BIND_ENUM_CONSTANT(COMPRESSION_JPG);
	BIND_ENUM_CONSTANT(COMPRESSION_H264);
}

#else

void GRDevice::_register_methods() {
	METHOD_REG(GRDevice, _notification);

	METHOD_REG(GRDevice, _internal_call_only_deffered_start);
	METHOD_REG(GRDevice, _internal_call_only_deffered_stop);

	METHOD_REG(GRDevice, _internal_call_only_deffered_restart);

	METHOD_REG(GRDevice, get_avg_ping);
	METHOD_REG(GRDevice, get_min_ping);
	METHOD_REG(GRDevice, get_max_ping);
	METHOD_REG(GRDevice, get_avg_fps);
	METHOD_REG(GRDevice, get_min_fps);
	METHOD_REG(GRDevice, get_max_fps);

	METHOD_REG(GRDevice, get_avg_recv_mbyte);
	METHOD_REG(GRDevice, get_min_recv_mbyte);
	METHOD_REG(GRDevice, get_max_recv_mbyte);
	METHOD_REG(GRDevice, get_avg_send_mbyte);
	METHOD_REG(GRDevice, get_min_send_mbyte);
	METHOD_REG(GRDevice, get_max_send_mbyte);

	METHOD_REG(GRDevice, get_total_received_mbytes);
	METHOD_REG(GRDevice, get_total_sended_mbytes);

	METHOD_REG(GRDevice, get_port);
	METHOD_REG(GRDevice, set_port);

	//METHOD_REG(GRDevice, send_packet);
	METHOD_REG(GRDevice, send_user_data);

	METHOD_REG(GRDevice, restart);
	METHOD_REG(GRDevice, start);
	METHOD_REG(GRDevice, stop);
	METHOD_REG(GRDevice, get_status);

	register_signal<GRDevice>("status_changed", "status", GODOT_VARIANT_TYPE_INT);
	register_signal<GRDevice>("user_data_received", "packet_id", GODOT_VARIANT_TYPE_NIL, "user_data", GODOT_VARIANT_TYPE_NIL);

	register_property<GRDevice, uint16_t>("port", &GRDevice::set_port, &GRDevice::get_port, PORT_STATIC_CONNECTION);
	register_property<GRDevice, uint16_t>("auto_connection_port", &GRDevice::set_auto_connection_port, &GRDevice::get_auto_connection_port, PORT_AUTO_CONNECTION);
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
	fps_counter.reset();
	ping_counter.reset();
	traffic_recv_counter.reset();
	traffic_send_counter.reset();

	total_bytes_received = 0;
	total_bytes_sended = 0;
}

Error GRDevice::send_data_to(RefStd(PacketPeerStream) ppeer, PoolByteArray data) {
	Error e = ppeer->put_var(data);
	if (e == Error::OK) {
		_update_avg_traffic(data.size(), 0);
		total_bytes_sended += data.size();
	} else {
		_update_avg_traffic(0, 0);
	}
	return e;
}

Error GRDevice::recv_data_from(RefStd(PacketPeerStream) ppeer, PoolByteArray *data) {
	Error e;
#ifndef GDNATIVE_LIBRARY
	Variant v;
	e = ppeer->get_var(v);
	*data = v;
#else
	*data = ppeer->get_var();
	e = ppeer->get_packet_error();
#endif

	if (e == Error::OK) {
		_update_avg_traffic(0, data->size());
		total_bytes_received += data->size();
	} else {
		_update_avg_traffic(0, 0);
	}
	return e;
}

void GRDevice::_update_avg_ping(uint64_t ping) {
	ping_counter.update(ping);
}

void GRDevice::_update_avg_fps(uint64_t frametime) {
	fps_counter.update(frametime, (int)round(Engine::get_singleton()->get_frames_per_second()));
}

void GRDevice::_update_avg_traffic(uint32_t bytes_sended, uint32_t bytes_received) {
	current_sec_bytes_received += bytes_received;
	current_sec_bytes_sended += bytes_sended;

	if (get_time_usec() - prev_traffic_counter_time > 1000_ms) {
		traffic_recv_counter.update(current_sec_bytes_received);
		traffic_send_counter.update(current_sec_bytes_sended);
		current_sec_bytes_received = 0;
		current_sec_bytes_sended = 0;
		prev_traffic_counter_time = get_time_usec();
	}
}

void GRDevice::send_user_data(Variant packet_id, Variant user_data, bool full_objects) {
	Scoped_lock(send_queue_mutex);
	auto packet = shared_new(GRPacketCustomUserData);
	send_packet(packet);

	packet->set_packet_id(packet_id);
	packet->set_send_full_objects(full_objects);
	packet->set_user_data(user_data);
}

void GRDevice::_send_queue_resize(int new_size) {
	Scoped_lock(send_queue_mutex);
	send_queue.resize(new_size);
}

std::shared_ptr<GRPacket> GRDevice::_send_queue_pop_front() {
	Scoped_lock(send_queue_mutex);
	std::shared_ptr<GRPacket> packet;
	if (send_queue.size() > 0) {
		packet = send_queue.front();
		send_queue.erase(send_queue.begin());
	}
	return packet;
}

void GRDevice::set_status(WorkingStatus status) {
	working_status = status;
	emit_signal("status_changed", working_status);
}

real_t GRDevice::get_avg_ping() {
	return ping_counter.get_avg();
}

real_t GRDevice::get_min_ping() {
	return ping_counter.get_min();
}

real_t GRDevice::get_max_ping() {
	return ping_counter.get_max();
}

real_t GRDevice::get_avg_fps() {
	return fps_counter.get_avg();
}

real_t GRDevice::get_min_fps() {
	return fps_counter.get_min();
}

real_t GRDevice::get_max_fps() {
	return fps_counter.get_max();
}

real_t GRDevice::get_avg_recv_mbyte() {
	return traffic_recv_counter.get_avg();
}

real_t GRDevice::get_min_recv_mbyte() {
	return traffic_recv_counter.get_min();
}

real_t GRDevice::get_max_recv_mbyte() {
	return traffic_recv_counter.get_max();
}

real_t GRDevice::get_avg_send_mbyte() {
	return traffic_send_counter.get_avg();
}

real_t GRDevice::get_min_send_mbyte() {
	return traffic_send_counter.get_min();
}

real_t GRDevice::get_max_send_mbyte() {
	return traffic_send_counter.get_max();
}

real_t GRDevice::get_total_sended_mbytes() {
	return real_t(total_bytes_sended / (double)MB_SIZE);
}

real_t GRDevice::get_total_received_mbytes() {
	return real_t(total_bytes_received / (double)MB_SIZE);
}

uint16_t GRDevice::get_port() {
	return static_port;
}

void GRDevice::set_port(uint16_t _port) {
	static_port = _port;
}

uint16_t GRDevice::get_auto_connection_port() {
	return auto_connection_port;
}

void GRDevice::set_auto_connection_port(uint16_t _port) {
	auto_connection_port = _port;
}

void GRDevice::send_packet(std::shared_ptr<GRPacket> packet) {
	ERR_FAIL_COND(!packet);
	Scoped_lock(send_queue_mutex);
	if (send_queue.size() > 10000)
		send_queue.resize(0);

	send_queue.push_back(packet);
}

void GRDevice::start() {
	call_deferred(NAMEOF(_internal_call_only_deffered_start));
}

void GRDevice::stop() {
	call_deferred(NAMEOF(_internal_call_only_deffered_stop));
}

void GRDevice::restart() {
	call_deferred(NAMEOF(_internal_call_only_deffered_restart));
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
	static_port = GET_PS(GodotRemote::ps_general_port_name);
	auto_connection_port = GET_PS(GodotRemote::ps_general_auto_connection_port_name);
}

void GRDevice::_deinit() {
	LEAVE_IF_EDITOR();
	if (GodotRemote::get_singleton()) {
		GodotRemote::get_singleton()->device = nullptr;
	}
}
