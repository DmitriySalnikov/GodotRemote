/* GRStreamDecoders.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRPacket.h"
#include "GRProfiler.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY

#include "core/image.h"
#else

#include <Image.hpp>
#include <Thread.hpp>

using namespace godot;
#endif

const int GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS = 16;

class GRClient;
class GRStreamDecoder;
class GRStreamDecoderImageSequence;

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
	void _start_encoder(Ref<GRPacket> packet);

public:
	void update();
	void set_gr_client(GRClient *client);
	void push_packet_to_decode(Ref<GRPacket> packet);
	void set_threads_count(int count);
	int get_threads_count();
	void set_active(bool state);
	bool is_active() { return active; };

	void _init();
	void _deinit();
};

//////////////////////////////////////////////////////////////////////////
// Encoders

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
	std::queue<Ref<GRPacket> > images;

public:
	void set_gr_client(GRClient *client);
	void set_decoders_manager(GRStreamDecodersManager *manager) { decoders_manager = manager; };
	virtual void push_packet_to_decode(Ref<GRPacket> packet);
	virtual void update(){};
	virtual int get_max_queued_frames() { return 16; }
	virtual void start_decoder_threads(int count){};
	virtual void stop_decoder_threads(){};

	void _init();
	void _deinit();
};

//////////////////////////////////////////////////////////////////////////
// Image Sequence

class GRStreamDecoderImageSequence : public GRStreamDecoder {
	GD_CLASS(GRStreamDecoderImageSequence, GRStreamDecoder);

private:
	class BufferedImage {
	public:
		Ref<Image> img;
		uint64_t frametime;
		uint64_t frame_send_time;
		bool is_ready = false;
		bool is_end = false;
		BufferedImage(uint64_t time_of_frame, uint64_t frame_added) {
			frametime = time_of_frame;
			frame_send_time = frame_added;
		}
	};

	std::vector<Thread_define_type> threads;
	GRUtils::iterable_queue<std::shared_ptr<BufferedImage> > buffer;

	PoolByteArray ret_data;
	bool is_threads_active = true;
	bool video_stream_enabled = true;
	uint64_t prev_shown_frame_time = 0;

	void _processing_thread(Variant p_userdata);

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	void _notification(int p_notification);

public:
	virtual void push_packet_to_decode(Ref<GRPacket> packet) override;
	virtual void update() override;
	virtual int get_max_queued_frames() override;
	virtual void start_decoder_threads(int count) override;
	virtual void stop_decoder_threads() override;

	void _init();
	void _deinit();
};

#endif // NO_GODOTREMOTE_CLIENT
