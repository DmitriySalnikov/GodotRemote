/* GRServer.h */
#pragma once

#ifndef NO_GODOTREMOTE_SERVER

#include "GRPacket.h"
#include "GRUtils.h"

#ifndef GDNATIVE_LIBRARY

#include "core/reference.h"
image
#else

#include <Image.hpp>
#include <Reference.hpp>
#include <Thread.hpp>

using namespace godot;
#endif

class GRSViewport;
class GRStreamEncoder;
class GRStreamEncoderImageSequence;

class GRStreamEncodersManager : public Reference {
	GD_CLASS(GRStreamEncodersManager, Reference);

protected:
#ifndef GDNATIVE_LIBRARY
	static void _bind_methods();
#else
public:
	static void _register_methods();

protected:
#endif

	Ref<GRStreamEncoder> encoder;

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

class GRStreamEncoder : public Reference {
	GD_CLASS(GRStreamEncoder, Reference);

protected:
	_TS_CLASS_;

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
	virtual bool has_data_to_send() const { return false; }
	virtual Ref<GRPacket> pop_data_to_send() { return Ref<GRPacket>(); }

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

	THREAD_FUNC void _processing_thread(THREAD_DATA p_user);

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

	virtual bool has_data_to_send() const override { return buffer.size(); }
	virtual Ref<GRPacket> pop_data_to_send() override;

	void _init();
	void _deinit();
};

#endif // !NO_GODOTREMOTE_SERVER
