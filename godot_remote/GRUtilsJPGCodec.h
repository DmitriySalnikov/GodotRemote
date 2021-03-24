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
		uint8_t **buf = nullptr;

		void update_size(int width, int height);
		void update_size(int y, int u, int v);
		void free_mem();
		~yuv_buffer_data_to_encoder() {
			free_mem();
		}
	};

	class yuv_buffer_data_to_decoder {
	public:
		uint8_t *buf[3] = { nullptr, nullptr, nullptr };
		void free_mem();
		~yuv_buffer_data_to_decoder() {
			free_mem();
		}
	};

	static int _get_yuv_comp_size(int componentID, int width, int height);
	static Error _encode_image_to_yuv(Ref<Image> img, int width, int height, int bytes_in_color, uint8_t **buf);
	static Error _decode_yuv_to_image(Ref<Image> &img, int width, int height, uint8_t *buf_y, uint8_t *buf_u, uint8_t *buf_v);

#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED
	static Error _decompress_jpg_turbo(PoolByteArray &img_data, PoolByteArray &jpg_buffer, Ref<Image> *out_img);
	static Error _compress_jpg_turbo(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
#else
	//////////////////////////////////////////////////////////////////////////
	// JPG Encoder fallback

	static bool rgb2yuv(const uint8_t *RgbBuf, int w, int h, int pixel_size, uint8_t **yuvBuf);
	static bool yuv2rgb(const uint8_t **yuv, int w, int h, uint8_t *rgb);

	static Error _compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color = 4, int quality = 75);
	static unsigned long tjPlaneSizeYUV(int componentID, int width, int stride, int height, int subsamp);
	static int tjPlaneWidth(int componentID, int width, int subsamp);
	static int tjPlaneHeight(int componentID, int height, int subsamp);
#endif
};
