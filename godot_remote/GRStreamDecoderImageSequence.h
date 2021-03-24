/* GRStreamDecoderImageSequence.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRPacket.h"
#include "GRProfiler.h"
#include "GRStreamDecoders.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY

#include "core/image.h"
#else

#include <Image.hpp>
#include <Thread.hpp>

using namespace godot;
#endif

const int GR_STREAM_DECODER_IMAGE_SEQUENCE_MAX_THREADS = 16;

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
	virtual void push_packet_to_decode(std::shared_ptr<GRPacket> packet) override;
	virtual void update() override;
	virtual int get_max_queued_frames() override;
	virtual void start_decoder_threads(int count) override;
	virtual void stop_decoder_threads() override;

	void _init();
	void _deinit();
};

#endif // NO_GODOTREMOTE_CLIENT
