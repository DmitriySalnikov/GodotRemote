/* GRStreamEncoders.h */
#pragma once

#ifndef NO_GODOTREMOTE_SERVER

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

const int GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS = 16;

class GRSViewport;
class GRStreamEncoder;
class GRStreamEncoderImageSequence;

class GRStreamEncodersManager : public Object {
	GD_CLASS(GRStreamEncodersManager, Object);
	Mutex_define(ts_lock, "GRStreamEncodersManager Lock");

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	GRStreamEncoder *encoder = nullptr;
	GRSViewport *viewport = nullptr;
	int threads_count = 1;
	bool active = false;

	void _notification(int p_notification);

public:
	/*GRDevice::ImageCompressionType
	*/
	void start(int compression_type, GRSViewport *vp);
	void commit_image(Ref<Image> img, uint64_t frametime);
	void commit_stream_end();
	bool has_data_to_send();
	Ref<GRPacket> pop_data_to_send();
	void set_threads_count(int count);
	int get_threads_count();
	void set_active(bool state);
	bool is_active() { return active; };

	void _init();
	void _deinit();
};

//////////////////////////////////////////////////////////////////////////
// Encoders

class GRStreamEncoder : public Object {
	GD_CLASS(GRStreamEncoder, Object);

protected:
	Mutex_define(ts_lock, "GRStreamEncoder Lock");

	class CommitedImage {
	public:
		Ref<Image> img;
		uint64_t time;
		uint64_t frametime;
		CommitedImage(Ref<Image> image, uint64_t time_added, uint64_t frame_time) {
			img = image;
			time = time_added;
			frametime = frame_time;
		}
	};
	std::queue<CommitedImage> images;

#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif
	GRSViewport *viewport = nullptr;

	void _notification(int p_notification);

public:
	void set_viewport(GRSViewport *vp) { viewport = vp; }
	void commit_image(Ref<Image> img, uint64_t frametime);
	virtual void commit_stream_end(){};
	virtual bool has_data_to_send() { return false; }
	virtual Ref<GRPacket> pop_data_to_send() { return Ref<GRPacket>(); }
	virtual int get_max_queued_frames() { return 16; }
	virtual void start_encoder_threads(int count){};
	virtual void stop_encoder_threads(){};

	void _init();
	void _deinit();
};

//////////////////////////////////////////////////////////////////////////
// Image Sequence

class GRStreamEncoderImageSequence : public GRStreamEncoder {
	GD_CLASS(GRStreamEncoderImageSequence, GRStreamEncoder);

private:
	class BufferedImage {
	public:
		Ref<GRPacket> pack;
		uint64_t time;
		bool is_ready = false;
		BufferedImage(uint64_t time_added) {
			time = time_added;
		}
	};

	std::vector<Thread_define_type> threads;
	std::queue<std::shared_ptr<BufferedImage> > buffer;

	PoolByteArray ret_data;
	int compression_type = 0;
	bool is_threads_active = true;
	bool video_stream_enabled = true;

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
	void set_compression_type(int comp) { compression_type = comp; };

	virtual void commit_stream_end() override;
	virtual bool has_data_to_send() override;
	virtual Ref<GRPacket> pop_data_to_send() override;
	virtual int get_max_queued_frames() override;
	virtual void start_encoder_threads(int count) override;
	virtual void stop_encoder_threads() override;

	void _init();
	void _deinit();
};

#endif // !NO_GODOTREMOTE_SERVER
