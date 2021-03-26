/* GRUtilsJPGCodec.h */
#pragma once

#include "GRProfiler.h"
#include "GRUtils.h"

class GRUtilsJPGCodec {
public:
	class yuv_buffer_data_to_encoder {
	public:
		int y_size = 0;
		int u_size = 0;
		int v_size = 0;
		uint8_t *buf[3] = { nullptr };

		void update_size(int width, int height);
		void update_size(int y, int u, int v);
		void free_mem();
		~yuv_buffer_data_to_encoder() {
			free_mem();
		}
	};

	static int _get_yuv_comp_size(int componentID, const int width, const int height);
	static Error _encode_image_to_yuv(Ref<Image> img, const int width, const int height, const int bytes_in_color, uint8_t *buf[3]);
	static Error _decode_yuv_to_image(Ref<Image> img, const int width, const int height, uint8_t *buf_y, uint8_t *buf_u, uint8_t *buf_v);

#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED
	static Error _decompress_jpg_turbo(PoolByteArray &img_data, PoolByteArray &jpg_buffer, Ref<Image> *out_img);
	static Error _compress_jpg_turbo(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
#else
	//////////////////////////////////////////////////////////////////////////
	// JPG Encoder fallback

	static Error _compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
	static unsigned long tjPlaneSizeYUV(int componentID, int width, int stride, int height, int subsamp);
	static int tjPlaneWidth(int componentID, int width, int subsamp);
	static int tjPlaneHeight(int componentID, int height, int subsamp);
#endif
};
