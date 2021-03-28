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

	class GRStreamEncoder *encoder = nullptr;
	class GRSViewport *viewport = nullptr;
	int threads_count = 1;
	bool active = false;

	void _delete_encoder();
	void _notification(int p_notification);

public:
	/*GRDevice::ImageCompressionType
	*/
	void start(int compression_type, GRSViewport *vp);
	void set_compression_type(int compression_type, GRSViewport *vp);
	void commit_image(Ref<Image> img, uint64_t frametime);
	void commit_stream_end();
	bool has_data_to_send();
	std::shared_ptr<GRPacketStreamData> pop_data_to_send();
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
	int compression_type = 0;

	class CommitedImage {
	public:
		Ref<Image> img;
		uint64_t time_added;
		uint64_t frametime;
		CommitedImage(Ref<Image> image, uint64_t _time_added, uint64_t frame_time) {
			img = image;
			time_added = _time_added;
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
	void set_compression_type(int comp) { compression_type = comp; };
	void set_viewport(GRSViewport *vp) { viewport = vp; }
	void commit_image(Ref<Image> img, uint64_t frametime);
	virtual void clear_buffers(){};
	virtual void commit_stream_end(){};
	virtual std::shared_ptr<GRPacketStreamData> create_stream_end_pack() { return std::shared_ptr<GRPacketStreamData>(); };
	virtual bool has_data_to_send() { return false; }
	virtual std::shared_ptr<GRPacketStreamData> pop_data_to_send() { return std::shared_ptr<GRPacketStreamData>(); }
	virtual int get_max_queued_frames() { return 16; }
	virtual void start_encoder_threads(int count){};
	virtual void stop_encoder_threads(){};

	void _init();
	void _deinit();
};

#endif // NO_GODOTREMOTE_SERVER
