/* GRUtilsH264Codec.cpp */

#include "GRUtilsH264Codec.h"
using namespace GRUtils;

#if defined(_MSC_VER)
#define LIB_PREFIX L""
#else
#define LIB_PREFIX L"lib"
#endif
// when updating just need to replace this string                              \/
#define OPENH264_LIB const wchar_t *GRUtilsH264Codec::lib_name = LIB_PREFIX L"openh264-2.1.1-"

#if defined(_MSC_VER)
#include "windows.h"
#if _WIN64
OPENH264_LIB L"win64.dll";
#else
OPENH264_LIB L"win32.dll";
#endif
// TODO mb that branches needed only on windows. Need tests.
#elif defined(__ANDROID__)
#if defined(__arm__) && defined(__ARM_ARCH_7A__)
OPENH264_LIB "android-arm.so";
#elif defined(__aarch64__)
OPENH264_LIB "android-arm64.so";
#elif defined(__i386__)
OPENH264_LIB "android-x86.so";
#elif defined(__x86_64__)
OPENH264_LIB "android-x64.so";
#endif
#elif defined(__linux__)
#if defined(__i386__)
OPENH264_LIB "linux32.6.so";
#elif defined(__x86_64__)
OPENH264_LIB "linux64.6.so";
#elif defined(__arm__)
OPENH264_LIB "linux-arm.6.so";
#elif defined(__aarch64__)
OPENH264_LIB "linux-arm64.6.so";
#endif
#endif

#undef OPENH264_LIB
#undef LIB_PREFIX

HMODULE openh264_handle_DLL;

GRUtilsH264Codec::WelsCreateSVCEncoderFunc GRUtilsH264Codec::CreateEncoderFunc = nullptr;
GRUtilsH264Codec::WelsDestroySVCEncoderFunc GRUtilsH264Codec::DestroyEncoderFunc = nullptr;
GRUtilsH264Codec::WelsCreateDecoderFunc GRUtilsH264Codec::CreateDecoderFunc = nullptr;
GRUtilsH264Codec::WelsDestroyDecoderFunc GRUtilsH264Codec::DestroyDecoderFunc = nullptr;

Error GRUtilsH264Codec::_encoder_create(ISVCEncoder **encoder) {
	String error_str = "";
	ISVCEncoder *enc = nullptr;
	*encoder = nullptr;
	int err = 0;

	openh264_handle_DLL = LoadLibraryW(lib_name);
	if (openh264_handle_DLL == NULL) {
		error_str = "";
		goto failed;
	}

	CreateEncoderFunc = (WelsCreateSVCEncoderFunc)GetProcAddress(openh264_handle_DLL, "WelsCreateSVCEncoder");
	if (CreateEncoderFunc == NULL) {
		error_str = "\"WelsCreateSVCEncoder\" not found.";
		goto failed;
	}
	DestroyEncoderFunc = (WelsDestroySVCEncoderFunc)GetProcAddress(openh264_handle_DLL, "WelsDestroySVCEncoder");
	if (DestroyEncoderFunc == NULL) {
		error_str = "\"WelsDestroySVCEncoder\" not found.";
		goto failed;
	}

	err = CreateEncoderFunc(&enc);
	if (err != 0) {
		error_str = "\"WelsCreateSVCEncoder\" return error code: " + str(err);
		goto failed;
	}

	*encoder = enc;
	_log("[OpenH264] Successfully created encoder from " + str(lib_name), LogLevel::LL_DEBUG);
	return Error::OK;

failed:
	_log("[OpenH264] No valid library named " + str(lib_name) + " was found. " + error_str, LogLevel::LL_WARNING);
	_clear();
	return Error::FAILED;
}

Error GRUtilsH264Codec::_encoder_free(ISVCEncoder *encoder) {
	if (encoder) {
		encoder->Uninitialize();
		DestroyEncoderFunc(encoder);

		_clear();
		return Error::OK;
	}
	_clear();
	return Error::FAILED;
}

Error GRUtilsH264Codec::_decoder_create(ISVCDecoder **decoder) {
	String error_str = "";
	ISVCDecoder *dec = nullptr;
	*decoder = nullptr;
	int err = 0;

	openh264_handle_DLL = LoadLibraryW(lib_name);
	if (openh264_handle_DLL == NULL) {
		error_str = "";
		goto failed;
	}

	CreateDecoderFunc = (WelsCreateDecoderFunc)GetProcAddress(openh264_handle_DLL, "WelsCreateDecoder");
	if (CreateDecoderFunc == NULL) {
		error_str = "\"WelsCreateSVCEncoder\" not found.";
		goto failed;
	}
	DestroyDecoderFunc = (WelsDestroyDecoderFunc)GetProcAddress(openh264_handle_DLL, "WelsDestroyDecoder");
	if (DestroyDecoderFunc == NULL) {
		error_str = "\"WelsDestroySVCEncoder\" not found.";
		goto failed;
	}

	err = CreateDecoderFunc(&dec);
	if (err != 0) {
		error_str = "\"WelsCreateSVCEncoder\" return error code: " + str(err);
		goto failed;
	}

	*decoder = dec;
	_log("[OpenH264] Successfully created decoder from " + str(lib_name), LogLevel::LL_DEBUG);
	return Error::OK;

failed:
	_log("[OpenH264] No valid library named " + str(lib_name) + " was found. " + error_str, LogLevel::LL_WARNING);
	_clear();
	return Error::FAILED;
}

Error GRUtilsH264Codec::_decoder_free(ISVCDecoder *decoder) {
	if (decoder) {
		decoder->Uninitialize();
		DestroyDecoderFunc(decoder);

		_clear();
		return Error::OK;
	}
	_clear();
	return Error::FAILED;
}

void GRUtilsH264Codec::_clear() {
	CreateEncoderFunc = nullptr;
	DestroyEncoderFunc = nullptr;
	CreateDecoderFunc = nullptr;
	DestroyDecoderFunc = nullptr;

	if (openh264_handle_DLL != NULL)
		FreeLibrary(openh264_handle_DLL);
	openh264_handle_DLL = NULL;
}
