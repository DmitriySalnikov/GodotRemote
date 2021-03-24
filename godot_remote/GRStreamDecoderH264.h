/* GRStreamDecoderH264.h */
#pragma once

#ifndef NO_GODOTREMOTE_CLIENT

#include "GRPacket.h"
#include "GRProfiler.h"
#include "GRStreamDecoders.h"
#include "GRUtils.h"
#include "GRUtilsH264Codec.h"

#ifndef GDNATIVE_LIBRARY

#include "core/image.h"
#else

#include <Image.hpp>
#include <Thread.hpp>

using namespace godot;
#endif

const int GR_STREAM_DECODER_H264_MAX_THREADS = 16;

//////////////////////////////////////////////////////////////////////////
// H264

class GRStreamDecoderH264 : public GRStreamDecoder {
	GD_CLASS(GRStreamDecoderH264, GRStreamDecoder);

private:
	class BufferedImage {
	public:
		Ref<Image> img;
		uint64_t frame_send_time;
		bool is_end = false;
		BufferedImage(uint64_t frame_added) {
			frame_send_time = frame_added;
		}
	};

	Thread_define(thread);
	std::queue<BufferedImage> buffer;

	PoolByteArray ret_data;
	bool video_stream_enabled = true;
	uint64_t prev_shown_frame_time = 0;
	bool is_thread_active = false;
	int threads_number = 2;

	float en_max_frame_rate = 0;
	int en_pic_width = 0;
	int en_pic_height = 0;
	int en_target_bitrate = 0;
	int en_threads_count = 0;
	void FlushFrames(ISVCDecoder *h264_decoder, int64_t &iTotal, uint64_t &uiTimeStamp, int32_t &iWidth, int32_t &iHeight, uint64_t start_time);
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
