/* GRStreamDecoders.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRPacket.h"
#include "GRProfiler.h"
#include "GRUtils.h"
#include "GRUtilsJPGCodec.h"

#ifndef GDNATIVE_LIBRARY

#include "core/image.h"
#else

#include <Image.hpp>
#include <Thread.hpp>

using namespace godot;
#endif

class GRClient;
class GRStreamDecoder;

class GRStreamDecodersManager : public Object {
	GD_CLASS(GRStreamDecodersManager, Object);
	Mutex_define(ts_lock, "GRStreamDecodersManager Lock");

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	GRClient *gr_client = nullptr;
	GRStreamDecoder *decoder = nullptr;
	int threads_count = 1;
	bool active = false;

	void _notification(int p_notification);
	void _start_decoder(std::shared_ptr<GRPacketStreamData> packet);

public:
	void update();
	void set_gr_client(GRClient *client);
	void push_packet_to_decode(std::shared_ptr<GRPacketStreamData> packet);
	void set_threads_count(int count);
	int get_threads_count();
	void set_active(bool state);
	bool is_active() { return active; };

	void _init();
	void _deinit();
};

//////////////////////////////////////////////////////////////////////////
// Decoders

class GRStreamDecoder : public Object {
	GD_CLASS(GRStreamDecoder, Object);

protected:
	Mutex_define(ts_lock, "GRStreamDecoder Lock");
	const double image_loss_time = 1.5;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif
	GRClient *gr_client = nullptr;
	GRStreamDecodersManager *decoders_manager = nullptr;
	void _notification(int p_notification);
	std::queue<std::shared_ptr<GRPacketStreamData> > images;

public:
	void set_gr_client(GRClient *client);
	void set_decoders_manager(GRStreamDecodersManager *manager) { decoders_manager = manager; };
	virtual void push_packet_to_decode(std::shared_ptr<GRPacketStreamData> packet);
	// TODO move to separate thread to stabilize fps
	virtual void update(){};
	virtual int get_max_queued_frames() { return 16; }
	virtual void start_decoder_threads(int count){};
	virtual void stop_decoder_threads(){};

	void _init();
	void _deinit();
};

#endif // NO_GODOTREMOTE_CLIENT
