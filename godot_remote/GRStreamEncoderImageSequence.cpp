/* GRStreamEncoderImageSequence.cpp */

#ifndef NO_GODOTREMOTE_SERVER

#include "GRStreamEncoderImageSequence.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRServer.h"
#include "GRStreamEncoders.h"
#include "GRUtilsJPGCodec.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY

#include "core/os/os.h"
#include "core/os/thread_safe.h"
#include "scene/main/node.h"

#else

#include <Node.hpp>
#include <OS.hpp>

using namespace godot;
#endif
using namespace GRUtils;

///////////////////////////////////////////////////////////////////////////////
// IMAGE SEQUENCE ENCODER

#ifndef GDNATIVE_LIBRARY

void GRStreamEncoderImageSequence::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamEncoderImageSequence::_processing_thread);
	//ADD_PROPERTY(PropertyInfo(Variant::INT, "password"), "set_password", "get_password");
	//ADD_SIGNAL(MethodInfo("client_connected", PropertyInfo(Variant::STRING, "device_id")));
}

#else

void GRStreamEncoderImageSequence::_register_methods() {
	METHOD_REG(GRStreamEncoderImageSequence, _processing_thread);
	//register_property<GRStreamEncoders, String>("password", &GRStreamEncoders::set_password, &GRStreamEncoders::get_password, "");
	//register_signal<GRStreamEncoders>("client_connected", "device_id", GODOT_VARIANT_TYPE_STRING);
}

#endif

void GRStreamEncoderImageSequence::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			GRStreamEncoder::_deinit();
			break;
	}
}

void GRStreamEncoderImageSequence::commit_stream_end() {
	Scoped_lock(ts_lock);
	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();

	
	auto b = shared_new(BufferedImage);
	auto p = shared_new(GRPacketStreamDataImage);
	p->set_is_stream_end(true);

	b->is_ready = true;
	shared_cast_var(GRPacketStreamDataImage, b->pack, p);
	buffer.push(b);
}

bool GRStreamEncoderImageSequence::has_data_to_send() {
	Scoped_lock(ts_lock);
	return buffer.size() && buffer.front()->is_ready;
}

std::shared_ptr<GRPacketStreamData> GRStreamEncoderImageSequence::pop_data_to_send() {
	Scoped_lock(ts_lock);
	auto buf_img = buffer.front();
	buffer.pop();
	return buf_img->pack;
}

int GRStreamEncoderImageSequence::get_max_queued_frames() {
	Scoped_lock(ts_lock);
	return (int)threads.size() * 2;
}

// TODO 0 - auto mode
void GRStreamEncoderImageSequence::start_encoder_threads(int count) {
	if (threads.size() == count)
		return;

	stop_encoder_threads();
	is_threads_active = true;
	ZoneScopedNC("Start Encoder Threads", tracy::Color::Firebrick);

	if (count < 0) {
		_log("threads count < 0. Disabling encoder.", LogLevel::LL_DEBUG);
		return;
	}

	if (count > GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS) {
		_log("threads count " + str(count) + " > " + str(GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS) + ". Clamping.", LogLevel::LL_ERROR);
		count = GR_STREAM_ENCODER_IMAGE_SEQUENCE_MAX_THREADS;
	}

	threads.resize(count);
	for (int i = 0; i < count; i++) {
		Thread_start(threads[i], this, _processing_thread, i);
	}
}

void GRStreamEncoderImageSequence::stop_encoder_threads() {
	ZoneScopedNC("Stop Encoder Threads", tracy::Color::Firebrick3);
	is_threads_active = false;
	for (int i = 0; i < threads.size(); i++) {
		Thread_close(threads[i]);
	}
	threads.resize(0);

	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();
}

void GRStreamEncoderImageSequence::_init() {
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRStreamEncoder::_init();
#endif
}

void GRStreamEncoderImageSequence::_deinit() {
	LEAVE_IF_EDITOR();
	stop_encoder_threads();
	while (buffer.size()) {
		buffer.pop();
	}
}

void GRStreamEncoderImageSequence::_processing_thread(Variant p_userdata) {
	int thread_idx = p_userdata;
	OS *os = OS::get_singleton();

	PoolByteArray jpg_buffer;
	jpg_buffer.resize((1024 * 1024) * ((int)GET_PS(GodotRemote::ps_server_jpg_buffer_mb_size_name)));

	Thread_set_name(("Stream Encoder: Image Sequence " + str(thread_idx)).ascii().get_data());

	while (is_threads_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		int jpg_quality = viewport->jpg_quality;
		CommitedImage com_image = images.front();
		images.pop();

		while (buffer.size() > get_max_queued_frames()) {
			buffer.pop();
		}

		if (buffer.size()) {
			shared_cast_def(GRPacketStreamDataImage, im, buffer.front()->pack);
			if (im && im->get_is_stream_end()) {
				ts_lock.unlock();
				continue;
			}
		}

		auto buf_img = std::make_shared<BufferedImage>();
		buffer.push(buf_img);

		ts_lock.unlock();

		Ref<Image> img = com_image.img;
		PoolByteArray img_data;
		auto pack = shared_new(GRPacketStreamDataImage);
		int bytes_in_color = img->get_format() == Image::FORMAT_RGB8 ? 3 : 4;

		pack->set_compression_type(compression_type);
		pack->set_size(Size2((float)img->get_width(), (float)img->get_height()));
		pack->set_format(img->get_format());
		pack->set_frametime(com_image.frametime);
		pack->set_start_time(com_image.time_added);
		pack->set_is_stream_end(false);

		if (img->get_data().size() == 0)
			goto end;

		switch (pack->get_compression_type()) {
			case GRDevice::ImageCompressionType::COMPRESSION_UNCOMPRESSED: {
				ZoneScopedNC("Image processed: Uncompressed", tracy::Color::VioletRed3);
				img_data = img->get_data();
				break;
			}
			case GRDevice::ImageCompressionType::COMPRESSION_JPG: {
				ZoneScopedNC("Image processed: JPG", tracy::Color::VioletRed3);
				if (!img_is_empty(img)) {
					Error err = Error::OK;

#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED
					err = GRUtilsJPGCodec::_compress_jpg_turbo(img_data, img->get_data(), jpg_buffer, (int)img->get_width(), (int)img->get_height(), bytes_in_color, jpg_quality);
#else
					err = GRUtilsJPGCodec::_compress_jpg(img_data, img->get_data(), jpg_buffer, (int)img->get_width(), (int)img->get_height(), bytes_in_color, jpg_quality);
#endif

					if ((int)err) {
						_log("Can't compress stream image JPG. Code: " + str((int)err), LogLevel::LL_ERROR);
						GRNotifications::add_notification("Stream Error", "Can't compress stream image to JPG. Code: " + str((int)err), GRNotifications::NotificationIcon::ICON_ERROR, true, 1.f);
						goto end;
					}
				}
				break;
			}
			default:
				_log("Not implemented compression type: " + str(pack->get_compression_type()), LogLevel::LL_ERROR);
				goto end;
				break;
		}
		pack->set_image_data(img_data);
	end:
		ZoneScopedNC("Push Image to Buffer", tracy::Color::Violet);
		ts_lock.lock();
		buf_img->pack = pack;
		buf_img->is_ready = true;
		ts_lock.unlock();
	}
}

#endif // !NO_GODOTREMOTE_SERVER
