/* GRServer.h */
#pragma once

#ifndef NO_GODOTREMOTE_SERVER

#include "GRPacket.h"
#include "GRUtils.h"
#include "GRProfiler.h"

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

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	GRStreamEncoder* encoder = nullptr;
	GRSViewport *viewport = nullptr;

	void _notification(int p_notification);

public:
	/*GRDevice::ImageCompressionType
	*/
	void start(int compression_type, GRSViewport *vp);
	void commit_image(Ref<Image> img, uint64_t frametime);
	bool has_data_to_send();
	Ref<GRPacket> pop_data_to_send();

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
	virtual bool has_data_to_send() { return false; }
	virtual Ref<GRPacket> pop_data_to_send() { return Ref<GRPacket>(); }
	virtual int get_max_queued_frames() { return 16; }

	void _init();
	void _deinit();
};

class GRStreamEncoderImageSequence : public GRStreamEncoder {
	GD_CLASS(GRStreamEncoderImageSequence, GRStreamEncoder);

private:
	class BufferedImage {
	public:
		Ref<GRPacket> pack;
		uint64_t time;
		BufferedImage(Ref<GRPacket> packet, uint64_t time_added) {
			pack = packet;
			time = time_added;
		}
		bool operator<(const BufferedImage &bi) const { return time < bi.time; }
	};

	std::vector<Thread_define_type> threads;
	std::vector<BufferedImage> buffer;
	PoolByteArray ret_data;
	int compression_type = 0;
	bool is_threads_active = true;
	bool video_stream_enabled = true;

	void _processing_thread(Variant p_userdata);
	Error compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, const PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75, int subsampling = 3 /*Subsampling ::SUBSAMPLING_H2V2*/);

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

	virtual bool has_data_to_send() override;
	virtual Ref<GRPacket> pop_data_to_send() override;
	virtual int get_max_queued_frames() override;

	void _init();
	void _deinit();
};

#endif // !NO_GODOTREMOTE_SERVER
