/* GRJPGCodec.h */
#ifndef GRJPGCODEC_H
#define GRJPGCODEC_H

#include "GRProfiler.h"
#include "GRUtils.h"

class GRJPGCodec {
public:
#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED
	static Error _decompress_jpg_turbo(PoolByteArray &img_data, PoolByteArray &jpg_buffer, Ref<Image> *out_img);
	static Error _compress_jpg_turbo(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
#else
	//////////////////////////////////////////////////////////////////////////
	// JPG Encoder fallback

	static Error _compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
#endif
};

#endif // !GRJPGCODEC_H
