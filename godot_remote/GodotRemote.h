/* GodotRemote.h */
#pragma once

#include "GRUtils.h"
#include "core/image.h"
#include "core/pool_vector.h"
#include "core/reference.h"

class GodotRemote : public Reference {
	GDCLASS(GodotRemote, Reference);

	static GodotRemote *singleton;
	PoolVector<uint8_t> compress_buffer;

private:
	class GRDevice *device = nullptr;

protected:
	static void _bind_methods();
	void _notification(int p_notification);
	class InputDefault *id;

public:
	const static String ps_autoload_name;

	enum Subsampling {
		SUBSAMPLING_Y_ONLY = 0,
		SUBSAMPLING_H1V1 = 1,
		SUBSAMPLING_H2V1 = 2,
		SUBSAMPLING_H2V2 = 3
	};

	enum DeviceType {
		DEVICE_Auto = 0,
		DEVICE_Development = 1,
		DEVICE_Standalone = 2,
	};

	void set_log_level(GRUtils::LogLevel lvl);

	PoolVector<uint8_t> compress_jpg(Ref<Image> orig_img, int quality, float scale = 1.f, int subsampling = Subsampling::SUBSAMPLING_H2V2);

	void set_gravity(const Vector3 &p_gravity) const;
	void set_accelerometer(const Vector3 &p_accel) const;
	void set_magnetometer(const Vector3 &p_magnetometer) const;
	void set_gyroscope(const Vector3 &p_gyroscope) const;

	class GRDevice *get_device() const;
	// must be call_deffered
	bool start_remote_device(DeviceType type = DeviceType::DEVICE_Auto);
	bool stop_remote_device();

	static GodotRemote *get_singleton();
	GodotRemote();
};

VARIANT_ENUM_CAST(GodotRemote::Subsampling)
VARIANT_ENUM_CAST(GodotRemote::DeviceType)










//
//#include "core/io/stream_peer_tcp.h"
//#include "core/io/tcp_server.h"
//#include "scene/main/node.h"
//
//class TestMultithread : public Node {
//	GDCLASS(TestMultithread, Node);
//
//protected:
//	static void _bind_methods();
//	void _notification(int p_notification);
//
//public:
//	struct StartArgs {
//		StreamPeerTCP *con;
//		String name;
//	};
//
//	class Thread *thread_server;
//	class Thread *thread_client;
//	static bool is_working;
//
//	static void _server(void *_data);
//	static void _client(void *_data);
//
//	static void _send_data(void *_data);
//	static void _recv_data(void *_data);
//
//	TestMultithread();
//};
