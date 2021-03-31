/* GRStreamEncoderImageSequence.h */
#pragma once

#ifndef NO_GODOTREMOTE_SERVER

#include "GRPacket.h"
#include "GRStreamEncoders.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY
#else
using namespace godot;
#endif

const int GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS = 16;

class GRSViewport;

class GRStreamEncoderImageSequence : public GRStreamEncoder {
	GD_CLASS(GRStreamEncoderImageSequence, GRStreamEncoder);

private:
	class BufferedImage {
	public:
		std::shared_ptr<GRPacketStreamDataImage> pack;
		bool is_ready = false;
	};

	std::vector<Ref<_Thread> > threads;
	std::queue<std::shared_ptr<BufferedImage> > buffer;

	PoolByteArray ret_data;
	bool is_threads_active = true;

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
