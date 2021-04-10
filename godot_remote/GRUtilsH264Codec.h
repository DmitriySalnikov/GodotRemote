/* GRUtilsH264Codec.h */
#pragma once

#include "GRUtils.h"

#include "openh264/include/codec_api.h"
#include "openh264/include/codec_app_def.h"
#include "openh264/include/codec_def.h"
#include "openh264/include/codec_ver.h"

/* Based on secile's OpenH264Lib.NET
* https://github.com/secile/OpenH264Lib.NET/
*/

class GRUtilsH264Codec {
private:
	// DLL Name
	static const char *lib_name;

	// Encoder
	typedef int(__stdcall *WelsCreateSVCEncoderFunc)(ISVCEncoder **ppEncoder);
	typedef void(__stdcall *WelsDestroySVCEncoderFunc)(ISVCEncoder *ppEncoder);
	static WelsCreateSVCEncoderFunc CreateEncoderFunc;
	static WelsDestroySVCEncoderFunc DestroyEncoderFunc;

	// Decoder
	typedef int(__stdcall *WelsCreateDecoderFunc)(ISVCDecoder **ppDecoder);
	typedef void(__stdcall *WelsDestroyDecoderFunc)(ISVCDecoder *ppDecoder);
	static WelsCreateDecoderFunc CreateDecoderFunc;
	static WelsDestroyDecoderFunc DestroyDecoderFunc;

	static void _clear();

	// YUV to RGB conversion
	static unsigned long get_PlaneSizeYUV(int componentID, int width, int height);
	static int get_PlaneWidth(int componentID, int width);
	static int get_PlaneHeight(int componentID, int height);

public:
	static Error _encoder_create(ISVCEncoder **encoder);
	static Error _encoder_free(ISVCEncoder *encoder);

	static Error _decoder_create(ISVCDecoder **decoder);
	static Error _decoder_free(ISVCDecoder *decoder);

	// YUV to RGB conversion
	class yuv_buffer_data_to_encoder {
	public:
		int y_size = 0;
		int uv_size = 0;
		uint8_t *buf[3] = { nullptr };

		void update_size(int width, int height);
		void free_mem();
		~yuv_buffer_data_to_encoder() {
			free_mem();
		}
	};

	static int _get_yuv_comp_size(int componentID, const int width, const int height);
	static Error _encode_image_to_yuv(PoolByteArray img_data, const int width, const int height, const int bytes_in_color, uint8_t *buf[3]);
	static Error _decode_yuv_to_image(PoolByteArray *data, const int width, const int height, uint8_t *buf_y, uint8_t *buf_u, uint8_t *buf_v, int stride[2]);
};
