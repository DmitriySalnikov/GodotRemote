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

public:
	static Error _encoder_create(ISVCEncoder **encoder);
	static Error _encoder_free(ISVCEncoder *encoder);

	static Error _decoder_create(ISVCDecoder **decoder);
	static Error _decoder_free(ISVCDecoder *decoder);
};
