/* GRStreamEncoderH264.h */
#pragma once

#if !defined(NO_GODOTREMOTE_SERVER) && defined(GODOTREMOTE_H264_ENABLED)

#include "GRPacket.h"
#include "GRStreamEncoders.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#else
using namespace godot;
#endif

const int GR_STREAM_ENCODER_H264_MAX_THREADS = 16;

class GRStreamEncoderH264 : public GRStreamEncoder {
	GD_CLASS(GRStreamEncoderH264, GRStreamEncoder);

public:
	class EncoderProperties {
	public:
		int max_frame_rate = 0;
		int pic_width = 0;
		int pic_height = 0;
		int target_bitrate = 0;
		int threads_count = 0;

		void operator=(EncoderProperties &other) {
			memcpy(this, &other, sizeof(EncoderProperties));
		}
		bool operator==(EncoderProperties &other) {
			return memcmp(&other, this, sizeof(EncoderProperties)) == 0;
		}
		bool operator!=(EncoderProperties &other) {
			return memcmp(&other, this, sizeof(EncoderProperties)) != 0;
		}
	};

private:
	class BufferedImage {
	public:
		std::shared_ptr<GRPacketStreamDataH264> pack;
		bool is_ready = false;
	};

	EncoderProperties encoder_props;
	Ref<_Thread> thread;
	std::queue<std::shared_ptr<BufferedImage> > buffer;

	bool is_thread_active = false;

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
	virtual void clear_buffers() override;
	virtual void commit_stream_end() override;
	virtual std::shared_ptr<GRPacketStreamData> create_stream_end_pack() override;
	virtual bool has_data_to_send() override;
	virtual std::shared_ptr<GRPacketStreamData> pop_data_to_send() override;
	virtual int get_max_queued_frames() override;
	virtual void start_encoder_threads(int count) override;
	virtual void stop_encoder_threads() override;

	void _init();
	void _deinit();
};

#endif // NO_GODOTREMOTE_SERVER
