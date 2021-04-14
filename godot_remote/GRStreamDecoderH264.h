/* GRStreamDecoderH264.h */
#pragma once

#if !defined(NO_GODOTREMOTE_CLIENT) && defined(GODOTREMOTE_H264_ENABLED)

#include "GRPacket.h"
#include "GRStreamDecoders.h"
#include "GRUtils.h"
#include "GRUtilsH264Codec.h"

#ifndef GDNATIVE_LIBRARY
#else
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
		PoolByteArray img_data;
		int img_width;
		int img_height;

		uint64_t frame_added_time;
		uint64_t frametime;
		bool is_end = false;
		BufferedImage(PoolByteArray data, int width, int height, uint64_t frame_added, uint64_t frame_time) {
			img_data = data;
			img_width = width;
			img_height = height;

			frame_added_time = frame_added;
			frametime = frame_time;
		}
	};

	Ref<_Thread> thread;
	Ref<_Thread> update_thread;
	std::queue<BufferedImage> buffer;

	PoolByteArray ret_data;
	bool video_stream_enabled = true;
	bool is_thread_active = false;
	bool is_update_thread_active = true;
	int threads_number = 2;

	float en_max_frame_rate = 0;
	int en_pic_width = 0;
	int en_pic_height = 0;
	int en_target_bitrate = 0;
	int en_threads_count = 0;
	void FlushFrames(ISVCDecoder *h264_decoder, uint64_t start_time, uint64_t frametime);
	bool ProcessFrame(SBufferInfo *info, uint64_t start_time, uint64_t frametime);

	void _commit_stream_end();
	void _update_thread(Variant p_userdata);
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
	virtual void push_packet_to_decode(std::shared_ptr<GRPacketStreamData> packet) override;
	virtual int get_max_queued_frames() override;
	virtual void start_decoder_threads(int count) override;
	virtual void stop_decoder_threads() override;

	void _init();
	void _deinit();
};

#endif // NO_GODOTREMOTE_CLIENT
