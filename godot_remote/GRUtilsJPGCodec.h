/* GRUtilsJPGCodec.h */
#pragma once

#include "GRUtils.h"

class GRUtilsJPGCodec {
public:

	static Error compress_image(Ref<Image> img, PoolByteArray &ret, PoolByteArray &jpg_buffer, int quality);
	static Error decompress_image(Ref<Image> &img, PoolByteArray &img_buffer, PoolByteArray &jpg_buffer);

#ifdef GODOT_REMOTE_LIBJPEG_TURBO_ENABLED
	static Error _decompress_jpg_turbo(PoolByteArray &img_data, PoolByteArray &jpg_buffer, PoolByteArray *out_data, int *out_width, int *out_height);
	static Error _compress_jpg_turbo(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
#else
	//////////////////////////////////////////////////////////////////////////
	// JPG Encoder fallback

	static Error _compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
#endif
};
