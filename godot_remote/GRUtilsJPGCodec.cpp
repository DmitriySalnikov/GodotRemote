/* GRUtilsJPGCodec.cpp */

#include "GRUtilsJPGCodec.h"
using namespace GRUtils;

void GRUtilsJPGCodec::yuv_buffer_data_to_encoder::update_size(int width, int height) {
	int y = _get_yuv_comp_size(0, width, height),
		u = _get_yuv_comp_size(1, width, height),
		v = _get_yuv_comp_size(2, width, height);

	update_size(y, u, v);
}

void GRUtilsJPGCodec::yuv_buffer_data_to_encoder::update_size(int y, int u, int v) {
	if (buf && buf[0] && buf[1] && buf[2] &&
			y_size == y && u_size == u && v_size == v)
		return;
	free_mem();

	buf[0] = new uint8_t[y];
	buf[1] = new uint8_t[u];
	buf[2] = new uint8_t[v];

	y_size = y;
	u_size = u;
	v_size = v;
}

void GRUtilsJPGCodec::yuv_buffer_data_to_encoder::free_mem() {
	if (buf[0]) {
		delete[] buf[0];
		buf[0] = nullptr;
	}
	if (buf[1]) {
		delete[] buf[1];
		buf[1] = nullptr;
	}
	if (buf[2]) {
		delete[] buf[2];
		buf[2] = nullptr;
	}
}

#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED

#include "libjpeg-turbo/include/turbojpeg.h"

Error GRUtilsJPGCodec::_compress_jpg_turbo(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color, int quality) {
	PoolByteArray res;
	ERR_FAIL_COND_V(img_data.size() == 0, Error::ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(quality < 1 || quality > 100, Error::ERR_INVALID_PARAMETER);

	unsigned long size = jpg_buffer.size();
	auto wb = jpg_buffer.write();
	auto ri = img_data.read();

	tjhandle _jpegCompressor = tjInitCompress();
	uint8_t *p_wb = wb.ptr();

	int j_err = tjCompress2(_jpegCompressor, ri.ptr(), width, 0, height, bytes_for_color == 3 ? TJPF::TJPF_RGB : TJPF::TJPF_RGBA,
			&p_wb, &size, TJSAMP_420, quality,
			TJFLAG_FASTDCT | TJFLAG_NOREALLOC);

	if (!j_err) {
		tjDestroy(_jpegCompressor);

		{
			ZoneScopedNC("Copy Compressed JPG Data to result PoolArray", tracy::Color::HotPink3);
			release_pva_read(ri);
			res.resize(size);
			auto wr = res.write();
			memcpy(wr.ptr(), wb.ptr(), size);
			release_pva_read(wb);
			release_pva_write(wr);
		}

		_log("JPG size: " + str(res.size()), LogLevel::LL_DEBUG);
		ret = res;
		return Error::OK;
	} else {
		String error_str = tjGetErrorStr2(_jpegCompressor);
		tjDestroy(_jpegCompressor);
		_log("JPEG-Turbo: " + error_str, LogLevel::LL_ERROR);
	}

	return Error::FAILED;
}

int GRUtilsJPGCodec::_get_yuv_comp_size(int componentID, int width, int height) {
	return tjPlaneSizeYUV(componentID, width, 0, height, TJSAMP_420);
}

Error GRUtilsJPGCodec::_encode_image_to_yuv(Ref<Image> img, const int width, const int height, const int bytes_in_color, uint8_t **buf) {
	tjhandle _jpegCompressor = tjInitCompress();
	auto img_data = img->get_data().read();
	int err = tjEncodeYUVPlanes(_jpegCompressor, img_data.ptr(), width, 0, height, bytes_in_color == 3 ? TJPF_RGB : TJPF_RGBA,
			buf, nullptr, TJSAMP_420, TJFLAG_FASTDCT | TJFLAG_NOREALLOC);
	if (err) {
		tjDestroy(_jpegCompressor);
		return Error::FAILED;
	}
	tjDestroy(_jpegCompressor);

	return Error::OK;
}

Error GRUtilsJPGCodec::_decode_yuv_to_image(Ref<Image> img, const int width, const int height, uint8_t *buf_y, uint8_t *buf_u, uint8_t *buf_v) {
	tjhandle _jpegDecompressor = tjInitDecompress();
	const uint8_t *yuv[] = {
		buf_y,
		buf_u,
		buf_v,
	};
	int strides[3] = {
		width,
		(width + 1) / 2,
		(width + 1) / 2
	};

	PoolByteArray rgb;
	rgb.resize(width * height * 3);
	auto rw = rgb.write();

	int err = tjDecodeYUVPlanes(_jpegDecompressor, yuv, strides, TJSAMP_420, rw.ptr(), width, 0, height, TJPF_RGB, TJFLAG_FASTDCT | TJFLAG_NOREALLOC);
	if (err) {
		tjDestroy(_jpegDecompressor);
		return Error::FAILED;
	}
	tjDestroy(_jpegDecompressor);

	release_pva_write(rw);
#ifndef GDNATIVE_LIBRARY
	img->create(width, height, false, Image::Format::FORMAT_RGB8, rgb);
#else
	img->create_from_data(width, height, false, Image::Format::FORMAT_RGB8, rgb);
#endif

	return img_is_empty(img) ? Error::FAILED : Error::OK;
}

Error GRUtilsJPGCodec::_decompress_jpg_turbo(PoolByteArray &img_data, PoolByteArray &jpg_buffer, Ref<Image> *out_img) {
	ERR_FAIL_COND_V(img_data.size() == 0, Error::ERR_INVALID_PARAMETER);

	int jpegSubsamp, width, height;
	tjhandle _jpegDecompressor = tjInitDecompress();

	auto wi = img_data.write();

	int j_err = tjDecompressHeader2(_jpegDecompressor, wi.ptr(), img_data.size(), &width, &height, &jpegSubsamp);
	if (!j_err) {
		int size = width * height * 3;
		if (jpg_buffer.size() < size)
			jpg_buffer.resize(size);
		auto wb = jpg_buffer.write();

		j_err = tjDecompress2(_jpegDecompressor, wi.ptr(), img_data.size(), wb.ptr(), width, 0 /*pitch*/, height, TJPF_RGB, TJFLAG_FASTDCT | TJFLAG_NOREALLOC);

		if (!j_err) {
			tjDestroy(_jpegDecompressor);

			ZoneScopedNC("Copy Decopressed JPG Data to result Image", tracy::Color::HotPink3);
			Ref<Image> img;
			img.instance();

			PoolByteArray fin_data;
			fin_data.resize(size);
			auto wr = fin_data.write();
			memcpy(wr.ptr(), wb.ptr(), size);
			release_pva_read(wb);
			release_pva_write(wr);

#ifndef GDNATIVE_LIBRARY
			img->create(width, height, false, Image::Format::FORMAT_RGB8, fin_data);
#else
			img->create_from_data(width, height, false, Image::Format::FORMAT_RGB8, fin_data);
#endif
			if (!img_is_empty(img)) {
				*out_img = img;
				return Error::OK;
			}
		} else {
			String error_str = tjGetErrorStr2(_jpegDecompressor);
			tjDestroy(_jpegDecompressor);
			_log("JPEG-Turbo: " + error_str, LogLevel::LL_ERROR);
		}
	} else {
		String error_str = tjGetErrorStr2(_jpegDecompressor);
		tjDestroy(_jpegDecompressor);
		_log("JPEG-Turbo: " + error_str, LogLevel::LL_ERROR);
	}

	return Error::FAILED;
}

#else

//////////////////////////////////////////////////////////////////////////
// JPG Encoder fallback

void yuv420_rgb24(
		uint32_t width, uint32_t height,
		const uint8_t *y, const uint8_t *u, const uint8_t *v, uint32_t y_stride, uint32_t uv_stride,
		uint8_t *rgb, uint32_t rgb_stride);

void rgb24_yuv420(
		uint32_t width, uint32_t height,
		const uint8_t *rgb, uint32_t rgb_stride,
		uint8_t *y, uint8_t *u, uint8_t *v, uint32_t y_stride, uint32_t uv_stride);

void rgb32_yuv420(
		uint32_t width, uint32_t height,
		const uint8_t *rgba, uint32_t rgba_stride,
		uint8_t *y, uint8_t *u, uint8_t *v, uint32_t y_stride, uint32_t uv_stride);

// richgel999/jpeg-compressor: https://github.com/richgel999/jpeg-compressor
#include "jpge.h"

Error GRUtilsJPGCodec::_compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, const int w, const int h, const int bytes_for_color, int quality) {
	PoolByteArray res;
	ERR_FAIL_COND_V(img_data.size() == 0, Error::ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(quality < 1 || quality > 100, Error::ERR_INVALID_PARAMETER);

	jpge::params params;
	params.m_quality = quality;
	params.m_subsampling = jpge::subsampling_t::H2V2;

	ERR_FAIL_COND_V(!params.check(), Error::ERR_INVALID_PARAMETER);
	auto wb = jpg_buffer.write();
	auto ri = img_data.read();
	int size = jpg_buffer.size();

	ERR_FAIL_COND_V_MSG(!jpge::compress_image_to_jpeg_file_in_memory((void *)wb.ptr(), size, w, h, bytes_for_color,
								(const unsigned char *)ri.ptr(), params),
			Error::FAILED, "Can't compress image.");

	{
		ZoneScopedNC("Copy Compressed JPG Data to result PoolArray", tracy::Color::HotPink3);
		release_pva_read(ri);
		res.resize(size);
		auto wr = res.write();
		memcpy(wr.ptr(), wb.ptr(), size);
		release_pva_write(wb);
		release_pva_write(wr);
	}

	_log("JPG size: " + str(res.size()), LogLevel::LL_DEBUG);

	ret = res;
	return Error::OK;
}

int GRUtilsJPGCodec::_get_yuv_comp_size(int componentID, int width, int height) {
	return tjPlaneSizeYUV(componentID, width, 0, height, 2);
}

Error GRUtilsJPGCodec::_encode_image_to_yuv(Ref<Image> img, const int width, const int height, const int bytes_in_color, uint8_t *buf[3]) {
	auto img_data = img->get_data().read();
	if (bytes_in_color == 3) {
		rgb24_yuv420(width, height, img_data.ptr(), width * bytes_in_color, buf[0], buf[1], buf[2], width, (width + 1) / 2);
	} else {
		rgb32_yuv420(width, height, img_data.ptr(), width * bytes_in_color, buf[0], buf[1], buf[2], width, (width + 1) / 2);
	}

	return Error::OK;
}

Error GRUtilsJPGCodec::_decode_yuv_to_image(Ref<Image> img, const int width, const int height, uint8_t *buf_y, uint8_t *buf_u, uint8_t *buf_v) {
	PoolByteArray rgb;
	rgb.resize(width * height * 3);
	auto rw = rgb.write();

	yuv420_rgb24(width, height, buf_y, buf_u, buf_v, width, (width + 1) / 2, rw.ptr(), width * 3);

	release_pva_write(rw);
#ifndef GDNATIVE_LIBRARY
	img->create(width, height, false, Image::Format::FORMAT_RGB8, rgb);
#else
	img->create_from_data(width, height, false, Image::Format::FORMAT_RGB8, rgb);
#endif

	return img_is_empty(img) ? Error::FAILED : Error::OK;
}

//////////////////////////////////////////////////////////////////////////
// LibJPEG-turbo original code

// https://github.com/libjpeg-turbo/libjpeg-turbo/blob/8e895c79e1d4c2a6727ee5729fe88bba1051ce33/turbojpeg.c#L610
unsigned long GRUtilsJPGCodec::tjPlaneSizeYUV(int componentID, int width, int stride, int height, int subsamp) {
	unsigned long long retval = 0;
	int pw, ph;

	if (width < 1 || height < 1 || subsamp < 0 || subsamp >= 6) {
		_log("tjPlaneSizeYUV(): Invalid argument", LogLevel::LL_ERROR);
		return -1;
	}

	pw = tjPlaneWidth(componentID, width, subsamp);
	ph = tjPlaneHeight(componentID, height, subsamp);
	if (pw < 0 || ph < 0) return -1;

	if (stride == 0)
		stride = pw;
	else
		stride = abs(stride);

	retval = (unsigned long long)stride * (ph - 1) + pw;
	if (retval > (unsigned long long)((unsigned long)-1)) {
		_log("tjPlaneSizeYUV(): Image is too large", LogLevel::LL_ERROR);
		return -1;
	}

	return (unsigned long)retval;
}

// https://github.com/libjpeg-turbo/libjpeg-turbo/blob/8e895c79e1d4c2a6727ee5729fe88bba1051ce33/turbojpeg.c#L53
#define PAD(v, p) ((v + (p)-1) & (~((p)-1)))

// https://github.com/libjpeg-turbo/libjpeg-turbo/blob/8e895c79e1d4c2a6727ee5729fe88bba1051ce33/turbojpeg.c#L568
int GRUtilsJPGCodec::tjPlaneWidth(int componentID, int width, int subsamp) {
	int pw, retval = 0;

	if (width < 1 || subsamp < 0 || subsamp >= 6) {
		_log("tjPlaneWidth(): Invalid argument", LogLevel::LL_ERROR);
		return -1;
	}
	if (componentID < 0 || componentID >= 3) {
		_log("tjPlaneWidth(): Invalid argument", LogLevel::LL_ERROR);
		return -1;
	}

	pw = PAD(width, 16 / 8);
	if (componentID == 0)
		retval = pw;
	else
		retval = pw * 8 / 16;

	return retval;
}

// https://github.com/libjpeg-turbo/libjpeg-turbo/blob/8e895c79e1d4c2a6727ee5729fe88bba1051ce33/turbojpeg.c#L589
int GRUtilsJPGCodec::tjPlaneHeight(int componentID, int height, int subsamp) {
	int ph, retval = 0;

	if (height < 1 || subsamp < 0 || subsamp >= 5) {
		_log("tjPlaneHeight(): Invalid argument", LogLevel::LL_ERROR);
		return -1;
	}
	if (componentID < 0 || componentID >= 3) {
		_log("tjPlaneHeight(): Invalid argument", LogLevel::LL_ERROR);
		return -1;
	}

	ph = PAD(height, 16 / 8);
	if (componentID == 0)
		retval = ph;
	else
		retval = ph * 8 / 16;

	return retval;
}

#undef PAD

// https://github.com/descampsa/yuv2rgb/blob/master/yuv_rgb.c

#include <intrin.h>
#define clamp(value) value < 0 ? 0 : (value > 255 ? 255 : value)

// see above for description
namespace RGB2YUVParam {
const uint8_t r_factor = 77; // [Rf]
const uint8_t g_factor = 150; // [Rg]
const uint8_t b_factor = 29; // [Rb]
const uint8_t cb_factor = 144; // [CbRange/(255*CbNorm)]
const uint8_t cr_factor = 183; // [CrRange/(255*CrNorm)]
const uint8_t y_factor = 128; // [(YMax-YMin)/255]
const uint8_t y_offset = 0; // YMin
} // namespace RGB2YUVParam

namespace YUV2RGBParam {
const uint8_t cb_factor = 113; // [(255*CbNorm)/CbRange]
const uint8_t cr_factor = 90; // [(255*CrNorm)/CrRange]
const uint8_t g_cb_factor = 44; // [Bf/Gf*(255*CbNorm)/CbRange]
const uint8_t g_cr_factor = 91; // [Rf/Gf*(255*CrNorm)/CrRange]
const uint8_t y_factor = 128; // [(YMax-YMin)/255]
const uint8_t y_offset = 0; // YMin
} // namespace YUV2RGBParam

#if !(defined(GODOT_REMOTE_USE_SSE2) && (defined(_M_AMD64) || defined(_M_X64) || _M_IX86_FP == 2))

void rgb24_yuv420(
		uint32_t width, uint32_t height,
		const uint8_t *RGB, uint32_t RGB_stride,
		uint8_t *Y, uint8_t *U, uint8_t *V, uint32_t Y_stride, uint32_t UV_stride) {

	uint32_t x, y;
	for (y = 0; y < (height - 1); y += 2) {
		const uint8_t *rgb_ptr1 = RGB + y * RGB_stride,
					  *rgb_ptr2 = RGB + (y + 1) * RGB_stride;

		uint8_t *y_ptr1 = Y + y * Y_stride,
				*y_ptr2 = Y + (y + 1) * Y_stride,
				*u_ptr = U + (y / 2) * UV_stride,
				*v_ptr = V + (y / 2) * UV_stride;

		for (x = 0; x < (width - 1); x += 2) {
			// compute yuv for the four pixels, u and v values are summed
			uint8_t y_tmp;
			int16_t u_tmp, v_tmp;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr1[0] + RGB2YUVParam::g_factor * rgb_ptr1[1] + RGB2YUVParam::b_factor * rgb_ptr1[2]) >> 8;
			u_tmp = rgb_ptr1[2] - y_tmp;
			v_tmp = rgb_ptr1[0] - y_tmp;
			y_ptr1[0] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr1[3] + RGB2YUVParam::g_factor * rgb_ptr1[4] + RGB2YUVParam::b_factor * rgb_ptr1[5]) >> 8;
			u_tmp += rgb_ptr1[5] - y_tmp;
			v_tmp += rgb_ptr1[3] - y_tmp;
			y_ptr1[1] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr2[0] + RGB2YUVParam::g_factor * rgb_ptr2[1] + RGB2YUVParam::b_factor * rgb_ptr2[2]) >> 8;
			u_tmp += rgb_ptr2[2] - y_tmp;
			v_tmp += rgb_ptr2[0] - y_tmp;
			y_ptr2[0] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr2[3] + RGB2YUVParam::g_factor * rgb_ptr2[4] + RGB2YUVParam::b_factor * rgb_ptr2[5]) >> 8;
			u_tmp += rgb_ptr2[5] - y_tmp;
			v_tmp += rgb_ptr2[3] - y_tmp;
			y_ptr2[1] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			u_ptr[0] = (((u_tmp >> 2) * RGB2YUVParam::cb_factor) >> 8) + 128;
			v_ptr[0] = (((v_tmp >> 2) * RGB2YUVParam::cb_factor) >> 8) + 128;

			rgb_ptr1 += 6;
			rgb_ptr2 += 6;
			y_ptr1 += 2;
			y_ptr2 += 2;
			u_ptr += 1;
			v_ptr += 1;
		}
	}
}

void rgb32_yuv420(
		uint32_t width, uint32_t height,
		const uint8_t *RGBA, uint32_t RGBA_stride,
		uint8_t *Y, uint8_t *U, uint8_t *V, uint32_t Y_stride, uint32_t UV_stride) {

	uint32_t x, y;
	for (y = 0; y < (height - 1); y += 2) {
		const uint8_t *rgb_ptr1 = RGBA + y * RGBA_stride,
					  *rgb_ptr2 = RGBA + (y + 1) * RGBA_stride;

		uint8_t *y_ptr1 = Y + y * Y_stride,
				*y_ptr2 = Y + (y + 1) * Y_stride,
				*u_ptr = U + (y / 2) * UV_stride,
				*v_ptr = V + (y / 2) * UV_stride;

		for (x = 0; x < (width - 1); x += 2) {
			// compute yuv for the four pixels, u and v values are summed
			uint8_t y_tmp;
			int16_t u_tmp, v_tmp;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr1[0] + RGB2YUVParam::g_factor * rgb_ptr1[1] + RGB2YUVParam::b_factor * rgb_ptr1[2]) >> 8;
			u_tmp = rgb_ptr1[2] - y_tmp;
			v_tmp = rgb_ptr1[0] - y_tmp;
			y_ptr1[0] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr1[4] + RGB2YUVParam::g_factor * rgb_ptr1[5] + RGB2YUVParam::b_factor * rgb_ptr1[6]) >> 8;
			u_tmp += rgb_ptr1[6] - y_tmp;
			v_tmp += rgb_ptr1[4] - y_tmp;
			y_ptr1[1] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr2[0] + RGB2YUVParam::g_factor * rgb_ptr2[1] + RGB2YUVParam::b_factor * rgb_ptr2[2]) >> 8;
			u_tmp += rgb_ptr2[2] - y_tmp;
			v_tmp += rgb_ptr2[0] - y_tmp;
			y_ptr2[0] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			y_tmp = (RGB2YUVParam::r_factor * rgb_ptr2[4] + RGB2YUVParam::g_factor * rgb_ptr2[5] + RGB2YUVParam::b_factor * rgb_ptr2[6]) >> 8;
			u_tmp += rgb_ptr2[6] - y_tmp;
			v_tmp += rgb_ptr2[4] - y_tmp;
			y_ptr2[1] = ((y_tmp * RGB2YUVParam::y_factor) >> 7) + RGB2YUVParam::y_offset;

			u_ptr[0] = (((u_tmp >> 2) * RGB2YUVParam::cb_factor) >> 8) + 128;
			v_ptr[0] = (((v_tmp >> 2) * RGB2YUVParam::cb_factor) >> 8) + 128;

			rgb_ptr1 += 8;
			rgb_ptr2 += 8;
			y_ptr1 += 2;
			y_ptr2 += 2;
			u_ptr += 1;
			v_ptr += 1;
		}
	}
}

void yuv420_rgb24(
		uint32_t width, uint32_t height,
		const uint8_t *Y, const uint8_t *U, const uint8_t *V, uint32_t Y_stride, uint32_t UV_stride,
		uint8_t *RGB, uint32_t RGB_stride) {
	uint32_t x, y;
	for (y = 0; y < (height - 1); y += 2) {
		const uint8_t *y_ptr1 = Y + y * Y_stride,
					  *y_ptr2 = Y + (y + 1) * Y_stride,
					  *u_ptr = U + (y / 2) * UV_stride,
					  *v_ptr = V + (y / 2) * UV_stride;

		uint8_t *rgb_ptr1 = RGB + y * RGB_stride,
				*rgb_ptr2 = RGB + (y + 1) * RGB_stride;

		for (x = 0; x < (width - 1); x += 2) {
			int8_t u_tmp, v_tmp;
			u_tmp = u_ptr[0] - 128;
			v_tmp = v_ptr[0] - 128;

			//compute Cb Cr color offsets, common to four pixels
			int16_t b_cb_offset, r_cr_offset, g_cbcr_offset;
			b_cb_offset = (YUV2RGBParam::cb_factor * u_tmp) >> 6;
			r_cr_offset = (YUV2RGBParam::cr_factor * v_tmp) >> 6;
			g_cbcr_offset = (YUV2RGBParam::g_cb_factor * u_tmp + YUV2RGBParam::g_cr_factor * v_tmp) >> 7;

			int16_t y_tmp;
			y_tmp = (YUV2RGBParam::y_factor * (y_ptr1[0] - YUV2RGBParam::y_offset)) >> 7;
			rgb_ptr1[0] = clamp(y_tmp + r_cr_offset);
			rgb_ptr1[1] = clamp(y_tmp - g_cbcr_offset);
			rgb_ptr1[2] = clamp(y_tmp + b_cb_offset);

			y_tmp = (YUV2RGBParam::y_factor * (y_ptr1[1] - YUV2RGBParam::y_offset)) >> 7;
			rgb_ptr1[3] = clamp(y_tmp + r_cr_offset);
			rgb_ptr1[4] = clamp(y_tmp - g_cbcr_offset);
			rgb_ptr1[5] = clamp(y_tmp + b_cb_offset);

			y_tmp = (YUV2RGBParam::y_factor * (y_ptr2[0] - YUV2RGBParam::y_offset)) >> 7;
			rgb_ptr2[0] = clamp(y_tmp + r_cr_offset);
			rgb_ptr2[1] = clamp(y_tmp - g_cbcr_offset);
			rgb_ptr2[2] = clamp(y_tmp + b_cb_offset);

			y_tmp = (YUV2RGBParam::y_factor * (y_ptr2[1] - YUV2RGBParam::y_offset)) >> 7;
			rgb_ptr2[3] = clamp(y_tmp + r_cr_offset);
			rgb_ptr2[4] = clamp(y_tmp - g_cbcr_offset);
			rgb_ptr2[5] = clamp(y_tmp + b_cb_offset);

			rgb_ptr1 += 6;
			rgb_ptr2 += 6;
			y_ptr1 += 2;
			y_ptr2 += 2;
			u_ptr += 1;
			v_ptr += 1;
		}
	}
}

#else

#define UNPACK_RGB24_32_STEP(RS1, RS2, RS3, RS4, RS5, RS6, RD1, RD2, RD3, RD4, RD5, RD6) \
	RD1 = _mm_unpacklo_epi8(RS1, RS4);                                                   \
	RD2 = _mm_unpackhi_epi8(RS1, RS4);                                                   \
	RD3 = _mm_unpacklo_epi8(RS2, RS5);                                                   \
	RD4 = _mm_unpackhi_epi8(RS2, RS5);                                                   \
	RD5 = _mm_unpacklo_epi8(RS3, RS6);                                                   \
	RD6 = _mm_unpackhi_epi8(RS3, RS6);

#define RGB2YUV_16(R, G, B, Y, U, V)                                                   \
	Y = _mm_add_epi16(_mm_mullo_epi16(R, _mm_set1_epi16(RGB2YUVParam::r_factor)),      \
			_mm_mullo_epi16(G, _mm_set1_epi16(RGB2YUVParam::g_factor)));               \
	Y = _mm_add_epi16(Y, _mm_mullo_epi16(B, _mm_set1_epi16(RGB2YUVParam::b_factor)));  \
	Y = _mm_srli_epi16(Y, 8);                                                          \
	U = _mm_mullo_epi16(_mm_sub_epi16(B, Y), _mm_set1_epi16(RGB2YUVParam::cb_factor)); \
	U = _mm_add_epi16(_mm_srai_epi16(U, 8), _mm_set1_epi16(128));                      \
	V = _mm_mullo_epi16(_mm_sub_epi16(R, Y), _mm_set1_epi16(RGB2YUVParam::cr_factor)); \
	V = _mm_add_epi16(_mm_srai_epi16(V, 8), _mm_set1_epi16(128));                      \
	Y = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(Y, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));

#define RGB2YUV_32                                                                                                                                       \
	__m128i r_16, g_16, b_16;                                                                                                                            \
	__m128i y1_16, y2_16, cb1_16, cb2_16, cr1_16, cr2_16, Y, cb, cr;                                                                                     \
	__m128i tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;                                                                                                          \
	__m128i rgb1 = LOAD_SI128((const __m128i *)(rgb_ptr1)),                                                                                              \
			rgb2 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 16)),                                                                                         \
			rgb3 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 32)),                                                                                         \
			rgb4 = LOAD_SI128((const __m128i *)(rgb_ptr2)),                                                                                              \
			rgb5 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 16)),                                                                                         \
			rgb6 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 32));                                                                                         \
	/* unpack rgb24 data to r, g and b data in separate channels*/                                                                                       \
	/* see rgb.txt to get an idea of the algorithm, note that we only go to the next to last step*/                                                      \
	/* here, because averaging in horizontal direction is easier like this*/                                                                             \
	/* The last step is applied further on the Y channel only*/                                                                                          \
	UNPACK_RGB24_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6)                                                         \
	UNPACK_RGB24_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6)                                                         \
	UNPACK_RGB24_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6)                                                         \
	UNPACK_RGB24_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6)                                                         \
	/* first compute Y', (B-Y') and (R-Y'), in 16bits values, for the first line */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are saved*/                                         \
	r_16 = _mm_unpacklo_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb1_16 = _mm_sub_epi16(b_16, y1_16);                                                                                                                 \
	cr1_16 = _mm_sub_epi16(r_16, y1_16);                                                                                                                 \
	r_16 = _mm_unpacklo_epi8(rgb4, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb1_16 = _mm_add_epi16(cb1_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr1_16 = _mm_add_epi16(cr1_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr1), Y);                                                                                                                  \
	/* same for the second line, compute Y', (B-Y') and (R-Y'), in 16bits values */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are added to the previous values*/                  \
	r_16 = _mm_unpackhi_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb1_16 = _mm_add_epi16(cb1_16, _mm_sub_epi16(b_16, y1_16));                                                                                          \
	cr1_16 = _mm_add_epi16(cr1_16, _mm_sub_epi16(r_16, y1_16));                                                                                          \
	r_16 = _mm_unpackhi_epi8(rgb4, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb1_16 = _mm_add_epi16(cb1_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr1_16 = _mm_add_epi16(cr1_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr2), Y);                                                                                                                  \
	/* Rescale Cb and Cr to their final range */                                                                                                         \
	cb1_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cb1_16, 2), _mm_set1_epi16(RGB2YUVParam::cb_factor)), 8), _mm_set1_epi16(128)); \
	cr1_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cr1_16, 2), _mm_set1_epi16(RGB2YUVParam::cr_factor)), 8), _mm_set1_epi16(128)); \
                                                                                                                                                         \
	/* do the same again with next data */                                                                                                               \
	rgb1 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 48)),                                                                                                 \
	rgb2 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 64)),                                                                                                 \
	rgb3 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 80)),                                                                                                 \
	rgb4 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 48)),                                                                                                 \
	rgb5 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 64)),                                                                                                 \
	rgb6 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 80));                                                                                                 \
	/* unpack rgb24 data to r, g and b data in separate channels*/                                                                                       \
	/* see rgb.txt to get an idea of the algorithm, note that we only go to the next to last step*/                                                      \
	/* here, because averaging in horizontal direction is easier like this*/                                                                             \
	/* The last step is applied further on the Y channel only*/                                                                                          \
	UNPACK_RGB24_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6)                                                         \
	UNPACK_RGB24_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6)                                                         \
	UNPACK_RGB24_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6)                                                         \
	UNPACK_RGB24_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6)                                                         \
	/* first compute Y', (B-Y') and (R-Y'), in 16bits values, for the first line */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are saved*/                                         \
	r_16 = _mm_unpacklo_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb2_16 = _mm_sub_epi16(b_16, y1_16);                                                                                                                 \
	cr2_16 = _mm_sub_epi16(r_16, y1_16);                                                                                                                 \
	r_16 = _mm_unpacklo_epi8(rgb4, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb2_16 = _mm_add_epi16(cb2_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr2_16 = _mm_add_epi16(cr2_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr1 + 16), Y);                                                                                                             \
	/* same for the second line, compute Y', (B-Y') and (R-Y'), in 16bits values */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are added to the previous values*/                  \
	r_16 = _mm_unpackhi_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb2_16 = _mm_add_epi16(cb2_16, _mm_sub_epi16(b_16, y1_16));                                                                                          \
	cr2_16 = _mm_add_epi16(cr2_16, _mm_sub_epi16(r_16, y1_16));                                                                                          \
	r_16 = _mm_unpackhi_epi8(rgb4, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb2_16 = _mm_add_epi16(cb2_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr2_16 = _mm_add_epi16(cr2_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr2 + 16), Y);                                                                                                             \
	/* Rescale Cb and Cr to their final range */                                                                                                         \
	cb2_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cb2_16, 2), _mm_set1_epi16(RGB2YUVParam::cb_factor)), 8), _mm_set1_epi16(128)); \
	cr2_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cr2_16, 2), _mm_set1_epi16(RGB2YUVParam::cr_factor)), 8), _mm_set1_epi16(128)); \
	/* Pack and save Cb Cr */                                                                                                                            \
	cb = _mm_packus_epi16(cb1_16, cb2_16);                                                                                                               \
	cr = _mm_packus_epi16(cr1_16, cr2_16);                                                                                                               \
	SAVE_SI128((__m128i *)(u_ptr), cb);                                                                                                                  \
	SAVE_SI128((__m128i *)(v_ptr), cr);

void rgb24_yuv420(uint32_t width, uint32_t height,
		const uint8_t *RGB, uint32_t RGB_stride,
		uint8_t *Y, uint8_t *U, uint8_t *V, uint32_t Y_stride, uint32_t UV_stride) {
#define LOAD_SI128 _mm_load_si128
#define SAVE_SI128 _mm_stream_si128

	uint32_t x, y;
	for (y = 0; y < (height - 1); y += 2) {
		const uint8_t *rgb_ptr1 = RGB + y * RGB_stride,
					  *rgb_ptr2 = RGB + (y + 1) * RGB_stride;

		uint8_t *y_ptr1 = Y + y * Y_stride,
				*y_ptr2 = Y + (y + 1) * Y_stride,
				*u_ptr = U + (y / 2) * UV_stride,
				*v_ptr = V + (y / 2) * UV_stride;

		for (x = 0; x < (width - 31); x += 32) {
			RGB2YUV_32

			rgb_ptr1 += 96;
			rgb_ptr2 += 96;
			y_ptr1 += 32;
			y_ptr2 += 32;
			u_ptr += 16;
			v_ptr += 16;
		}
	}
#undef LOAD_SI128
#undef SAVE_SI128
}

// see rgba.txt
#define UNPACK_RGB32_32_STEP(RS1, RS2, RS3, RS4, RS5, RS6, RS7, RS8, RD1, RD2, RD3, RD4, RD5, RD6, RD7, RD8) \
	RD1 = _mm_unpacklo_epi8(RS1, RS5);                                                                       \
	RD2 = _mm_unpackhi_epi8(RS1, RS5);                                                                       \
	RD3 = _mm_unpacklo_epi8(RS2, RS6);                                                                       \
	RD4 = _mm_unpackhi_epi8(RS2, RS6);                                                                       \
	RD5 = _mm_unpacklo_epi8(RS3, RS7);                                                                       \
	RD6 = _mm_unpackhi_epi8(RS3, RS7);                                                                       \
	RD7 = _mm_unpacklo_epi8(RS4, RS8);                                                                       \
	RD8 = _mm_unpackhi_epi8(RS4, RS8);

#define RGBA2YUV_32                                                                                                                                      \
	__m128i r_16, g_16, b_16;                                                                                                                            \
	__m128i y1_16, y2_16, cb1_16, cb2_16, cr1_16, cr2_16, Y, cb, cr;                                                                                     \
	__m128i tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;                                                                                              \
	__m128i rgb1 = LOAD_SI128((const __m128i *)(rgb_ptr1)),                                                                                              \
			rgb2 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 16)),                                                                                         \
			rgb3 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 32)),                                                                                         \
			rgb4 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 48)),                                                                                         \
			rgb5 = LOAD_SI128((const __m128i *)(rgb_ptr2)),                                                                                              \
			rgb6 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 16)),                                                                                         \
			rgb7 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 32)),                                                                                         \
			rgb8 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 48));                                                                                         \
	/* unpack rgb24 data to r, g and b data in separate channels*/                                                                                       \
	/* see rgb.txt to get an idea of the algorithm, note that we only go to the next to last step*/                                                      \
	/* here, because averaging in horizontal direction is easier like this*/                                                                             \
	/* The last step is applied further on the Y channel only*/                                                                                          \
	UNPACK_RGB32_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8)                                 \
	UNPACK_RGB32_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8)                                 \
	UNPACK_RGB32_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8)                                 \
	UNPACK_RGB32_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8)                                 \
	/* first compute Y', (B-Y') and (R-Y'), in 16bits values, for the first line */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are saved*/                                         \
	r_16 = _mm_unpacklo_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb1_16 = _mm_sub_epi16(b_16, y1_16);                                                                                                                 \
	cr1_16 = _mm_sub_epi16(r_16, y1_16);                                                                                                                 \
	r_16 = _mm_unpacklo_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb7, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb1_16 = _mm_add_epi16(cb1_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr1_16 = _mm_add_epi16(cr1_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr1), Y);                                                                                                                  \
	/* same for the second line, compute Y', (B-Y') and (R-Y'), in 16bits values */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are added to the previous values*/                  \
	r_16 = _mm_unpackhi_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb1_16 = _mm_add_epi16(cb1_16, _mm_sub_epi16(b_16, y1_16));                                                                                          \
	cr1_16 = _mm_add_epi16(cr1_16, _mm_sub_epi16(r_16, y1_16));                                                                                          \
	r_16 = _mm_unpackhi_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb7, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb1_16 = _mm_add_epi16(cb1_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr1_16 = _mm_add_epi16(cr1_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr2), Y);                                                                                                                  \
	/* Rescale Cb and Cr to their final range */                                                                                                         \
	cb1_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cb1_16, 2), _mm_set1_epi16(RGB2YUVParam::cb_factor)), 8), _mm_set1_epi16(128)); \
	cr1_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cr1_16, 2), _mm_set1_epi16(RGB2YUVParam::cr_factor)), 8), _mm_set1_epi16(128)); \
                                                                                                                                                         \
	/* do the same again with next data */                                                                                                               \
	rgb1 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 64)),                                                                                                 \
	rgb2 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 80)),                                                                                                 \
	rgb3 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 96)),                                                                                                 \
	rgb4 = LOAD_SI128((const __m128i *)(rgb_ptr1 + 112)),                                                                                                \
	rgb5 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 64)),                                                                                                 \
	rgb6 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 80)),                                                                                                 \
	rgb7 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 96)),                                                                                                 \
	rgb8 = LOAD_SI128((const __m128i *)(rgb_ptr2 + 112));                                                                                                \
	/* unpack rgb24 data to r, g and b data in separate channels*/                                                                                       \
	/* see rgb.txt to get an idea of the algorithm, note that we only go to the next to last step*/                                                      \
	/* here, because averaging in horizontal direction is easier like this*/                                                                             \
	/* The last step is applied further on the Y channel only*/                                                                                          \
	UNPACK_RGB32_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8)                                 \
	UNPACK_RGB32_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8)                                 \
	UNPACK_RGB32_32_STEP(rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8)                                 \
	UNPACK_RGB32_32_STEP(tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, rgb1, rgb2, rgb3, rgb4, rgb5, rgb6, rgb7, rgb8)                                 \
	/* first compute Y', (B-Y') and (R-Y'), in 16bits values, for the first line */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are saved*/                                         \
	r_16 = _mm_unpacklo_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb2_16 = _mm_sub_epi16(b_16, y1_16);                                                                                                                 \
	cr2_16 = _mm_sub_epi16(r_16, y1_16);                                                                                                                 \
	r_16 = _mm_unpacklo_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpacklo_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpacklo_epi8(rgb7, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb2_16 = _mm_add_epi16(cb2_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr2_16 = _mm_add_epi16(cr2_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr1 + 16), Y);                                                                                                             \
	/* same for the second line, compute Y', (B-Y') and (R-Y'), in 16bits values */                                                                      \
	/* Y is saved for each pixel, while only sums of (B-Y') and (R-Y') for pairs of adjacents pixels are added to the previous values*/                  \
	r_16 = _mm_unpackhi_epi8(rgb1, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb2, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb3, _mm_setzero_si128());                                                                                                 \
	y1_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y1_16 = _mm_add_epi16(y1_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y1_16 = _mm_srli_epi16(y1_16, 8);                                                                                                                    \
	cb2_16 = _mm_add_epi16(cb2_16, _mm_sub_epi16(b_16, y1_16));                                                                                          \
	cr2_16 = _mm_add_epi16(cr2_16, _mm_sub_epi16(r_16, y1_16));                                                                                          \
	r_16 = _mm_unpackhi_epi8(rgb5, _mm_setzero_si128());                                                                                                 \
	g_16 = _mm_unpackhi_epi8(rgb6, _mm_setzero_si128());                                                                                                 \
	b_16 = _mm_unpackhi_epi8(rgb7, _mm_setzero_si128());                                                                                                 \
	y2_16 = _mm_add_epi16(_mm_mullo_epi16(r_16, _mm_set1_epi16(RGB2YUVParam::r_factor)),                                                                 \
			_mm_mullo_epi16(g_16, _mm_set1_epi16(RGB2YUVParam::g_factor)));                                                                              \
	y2_16 = _mm_add_epi16(y2_16, _mm_mullo_epi16(b_16, _mm_set1_epi16(RGB2YUVParam::b_factor)));                                                         \
	y2_16 = _mm_srli_epi16(y2_16, 8);                                                                                                                    \
	cb2_16 = _mm_add_epi16(cb2_16, _mm_sub_epi16(b_16, y2_16));                                                                                          \
	cr2_16 = _mm_add_epi16(cr2_16, _mm_sub_epi16(r_16, y2_16));                                                                                          \
	/* Rescale Y' to Y, pack it to 8bit values and save it */                                                                                            \
	y1_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y1_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	y2_16 = _mm_add_epi16(_mm_srli_epi16(_mm_mullo_epi16(y2_16, _mm_set1_epi16(RGB2YUVParam::y_factor)), 7), _mm_set1_epi16(RGB2YUVParam::y_offset));    \
	Y = _mm_packus_epi16(y1_16, y2_16);                                                                                                                  \
	Y = _mm_unpackhi_epi8(_mm_slli_si128(Y, 8), Y);                                                                                                      \
	SAVE_SI128((__m128i *)(y_ptr2 + 16), Y);                                                                                                             \
	/* Rescale Cb and Cr to their final range */                                                                                                         \
	cb2_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cb2_16, 2), _mm_set1_epi16(RGB2YUVParam::cb_factor)), 8), _mm_set1_epi16(128)); \
	cr2_16 = _mm_add_epi16(_mm_srai_epi16(_mm_mullo_epi16(_mm_srai_epi16(cr2_16, 2), _mm_set1_epi16(RGB2YUVParam::cr_factor)), 8), _mm_set1_epi16(128)); \
	/* Pack and save Cb Cr */                                                                                                                            \
	cb = _mm_packus_epi16(cb1_16, cb2_16);                                                                                                               \
	cr = _mm_packus_epi16(cr1_16, cr2_16);                                                                                                               \
	SAVE_SI128((__m128i *)(u_ptr), cb);                                                                                                                  \
	SAVE_SI128((__m128i *)(v_ptr), cr);

void rgb32_yuv420(uint32_t width, uint32_t height,
		const uint8_t *RGBA, uint32_t RGBA_stride,
		uint8_t *Y, uint8_t *U, uint8_t *V, uint32_t Y_stride, uint32_t UV_stride) {
#define LOAD_SI128 _mm_load_si128
#define SAVE_SI128 _mm_stream_si128

	uint32_t x, y;
	for (y = 0; y < (height - 1); y += 2) {
		const uint8_t *rgb_ptr1 = RGBA + y * RGBA_stride,
					  *rgb_ptr2 = RGBA + (y + 1) * RGBA_stride;

		uint8_t *y_ptr1 = Y + y * Y_stride,
				*y_ptr2 = Y + (y + 1) * Y_stride,
				*u_ptr = U + (y / 2) * UV_stride,
				*v_ptr = V + (y / 2) * UV_stride;

		for (x = 0; x < (width - 31); x += 32) {
			RGBA2YUV_32

			rgb_ptr1 += 128;
			rgb_ptr2 += 128;
			y_ptr1 += 32;
			y_ptr2 += 32;
			u_ptr += 16;
			v_ptr += 16;
		}
	}
#undef LOAD_SI128
#undef SAVE_SI128
}

#define UV2RGB_16(U, V, R1, G1, B1, R2, G2, B2)                                                    \
	r_tmp = _mm_srai_epi16(_mm_mullo_epi16(V, _mm_set1_epi16(YUV2RGBParam::cr_factor)), 6);        \
	g_tmp = _mm_srai_epi16(_mm_add_epi16(                                                          \
								   _mm_mullo_epi16(U, _mm_set1_epi16(YUV2RGBParam::g_cb_factor)),  \
								   _mm_mullo_epi16(V, _mm_set1_epi16(YUV2RGBParam::g_cr_factor))), \
			7);                                                                                    \
	b_tmp = _mm_srai_epi16(_mm_mullo_epi16(U, _mm_set1_epi16(YUV2RGBParam::cb_factor)), 6);        \
	R1 = _mm_unpacklo_epi16(r_tmp, r_tmp);                                                         \
	G1 = _mm_unpacklo_epi16(g_tmp, g_tmp);                                                         \
	B1 = _mm_unpacklo_epi16(b_tmp, b_tmp);                                                         \
	R2 = _mm_unpackhi_epi16(r_tmp, r_tmp);                                                         \
	G2 = _mm_unpackhi_epi16(g_tmp, g_tmp);                                                         \
	B2 = _mm_unpackhi_epi16(b_tmp, b_tmp);

#define ADD_Y2RGB_16(Y1, Y2, R1, G1, B1, R2, G2, B2)                                     \
	Y1 = _mm_srai_epi16(_mm_mullo_epi16(Y1, _mm_set1_epi16(YUV2RGBParam::y_factor)), 7); \
	Y2 = _mm_srai_epi16(_mm_mullo_epi16(Y2, _mm_set1_epi16(YUV2RGBParam::y_factor)), 7); \
                                                                                         \
	R1 = _mm_add_epi16(Y1, R1);                                                          \
	G1 = _mm_sub_epi16(Y1, G1);                                                          \
	B1 = _mm_add_epi16(Y1, B1);                                                          \
	R2 = _mm_add_epi16(Y2, R2);                                                          \
	G2 = _mm_sub_epi16(Y2, G2);                                                          \
	B2 = _mm_add_epi16(Y2, B2);

#define PACK_RGB24_32_STEP(RS1, RS2, RS3, RS4, RS5, RS6, RD1, RD2, RD3, RD4, RD5, RD6)                          \
	RD1 = _mm_packus_epi16(_mm_and_si128(RS1, _mm_set1_epi16(0xFF)), _mm_and_si128(RS2, _mm_set1_epi16(0xFF))); \
	RD2 = _mm_packus_epi16(_mm_and_si128(RS3, _mm_set1_epi16(0xFF)), _mm_and_si128(RS4, _mm_set1_epi16(0xFF))); \
	RD3 = _mm_packus_epi16(_mm_and_si128(RS5, _mm_set1_epi16(0xFF)), _mm_and_si128(RS6, _mm_set1_epi16(0xFF))); \
	RD4 = _mm_packus_epi16(_mm_srli_epi16(RS1, 8), _mm_srli_epi16(RS2, 8));                                     \
	RD5 = _mm_packus_epi16(_mm_srli_epi16(RS3, 8), _mm_srli_epi16(RS4, 8));                                     \
	RD6 = _mm_packus_epi16(_mm_srli_epi16(RS5, 8), _mm_srli_epi16(RS6, 8));

#define PACK_RGB24_32(R1, R2, G1, G2, B1, B2, RGB1, RGB2, RGB3, RGB4, RGB5, RGB6)  \
	PACK_RGB24_32_STEP(R1, R2, G1, G2, B1, B2, RGB1, RGB2, RGB3, RGB4, RGB5, RGB6) \
	PACK_RGB24_32_STEP(RGB1, RGB2, RGB3, RGB4, RGB5, RGB6, R1, R2, G1, G2, B1, B2) \
	PACK_RGB24_32_STEP(R1, R2, G1, G2, B1, B2, RGB1, RGB2, RGB3, RGB4, RGB5, RGB6) \
	PACK_RGB24_32_STEP(RGB1, RGB2, RGB3, RGB4, RGB5, RGB6, R1, R2, G1, G2, B1, B2) \
	PACK_RGB24_32_STEP(R1, R2, G1, G2, B1, B2, RGB1, RGB2, RGB3, RGB4, RGB5, RGB6)

#define LOAD_UV_PLANAR                                \
	__m128i u = LOAD_SI128((const __m128i *)(u_ptr)); \
	__m128i v = LOAD_SI128((const __m128i *)(v_ptr));

#define LOAD_UV_NV12                                                                                                \
	__m128i uv1 = LOAD_SI128((const __m128i *)(uv_ptr));                                                            \
	__m128i uv2 = LOAD_SI128((const __m128i *)(uv_ptr + 16));                                                       \
	__m128i u = _mm_packus_epi16(_mm_and_si128(uv1, _mm_set1_epi16(255)), _mm_and_si128(uv2, _mm_set1_epi16(255))); \
	uv1 = _mm_srli_epi16(uv1, 8);                                                                                   \
	uv2 = _mm_srli_epi16(uv2, 8);                                                                                   \
	__m128i v = _mm_packus_epi16(_mm_and_si128(uv1, _mm_set1_epi16(255)), _mm_and_si128(uv2, _mm_set1_epi16(255)));

#define LOAD_UV_NV21                                                                                                \
	__m128i uv1 = LOAD_SI128((const __m128i *)(uv_ptr));                                                            \
	__m128i uv2 = LOAD_SI128((const __m128i *)(uv_ptr + 16));                                                       \
	__m128i v = _mm_packus_epi16(_mm_and_si128(uv1, _mm_set1_epi16(255)), _mm_and_si128(uv2, _mm_set1_epi16(255))); \
	uv1 = _mm_srli_epi16(uv1, 8);                                                                                   \
	uv2 = _mm_srli_epi16(uv2, 8);                                                                                   \
	__m128i u = _mm_packus_epi16(_mm_and_si128(uv1, _mm_set1_epi16(255)), _mm_and_si128(uv2, _mm_set1_epi16(255)));

#define YUV2RGB_32                                                                                          \
	__m128i r_tmp, g_tmp, b_tmp;                                                                            \
	__m128i r_16_1, g_16_1, b_16_1, r_16_2, g_16_2, b_16_2;                                                 \
	__m128i r_uv_16_1, g_uv_16_1, b_uv_16_1, r_uv_16_2, g_uv_16_2, b_uv_16_2;                               \
	__m128i y_16_1, y_16_2;                                                                                 \
                                                                                                            \
	u = _mm_add_epi8(u, _mm_set1_epi8(-128));                                                               \
	v = _mm_add_epi8(v, _mm_set1_epi8(-128));                                                               \
                                                                                                            \
	/* process first 16 pixels of first line */                                                             \
	__m128i u_16 = _mm_srai_epi16(_mm_unpacklo_epi8(u, u), 8);                                              \
	__m128i v_16 = _mm_srai_epi16(_mm_unpacklo_epi8(v, v), 8);                                              \
                                                                                                            \
	UV2RGB_16(u_16, v_16, r_uv_16_1, g_uv_16_1, b_uv_16_1, r_uv_16_2, g_uv_16_2, b_uv_16_2)                 \
	r_16_1 = r_uv_16_1;                                                                                     \
	g_16_1 = g_uv_16_1;                                                                                     \
	b_16_1 = b_uv_16_1;                                                                                     \
	r_16_2 = r_uv_16_2;                                                                                     \
	g_16_2 = g_uv_16_2;                                                                                     \
	b_16_2 = b_uv_16_2;                                                                                     \
                                                                                                            \
	__m128i y = LOAD_SI128((const __m128i *)(y_ptr1));                                                      \
	y = _mm_sub_epi8(y, _mm_set1_epi8(YUV2RGBParam::y_offset));                                             \
	y_16_1 = _mm_unpacklo_epi8(y, _mm_setzero_si128());                                                     \
	y_16_2 = _mm_unpackhi_epi8(y, _mm_setzero_si128());                                                     \
                                                                                                            \
	ADD_Y2RGB_16(y_16_1, y_16_2, r_16_1, g_16_1, b_16_1, r_16_2, g_16_2, b_16_2)                            \
                                                                                                            \
	__m128i r_8_11 = _mm_packus_epi16(r_16_1, r_16_2);                                                      \
	__m128i g_8_11 = _mm_packus_epi16(g_16_1, g_16_2);                                                      \
	__m128i b_8_11 = _mm_packus_epi16(b_16_1, b_16_2);                                                      \
                                                                                                            \
	/* process first 16 pixels of second line */                                                            \
	r_16_1 = r_uv_16_1;                                                                                     \
	g_16_1 = g_uv_16_1;                                                                                     \
	b_16_1 = b_uv_16_1;                                                                                     \
	r_16_2 = r_uv_16_2;                                                                                     \
	g_16_2 = g_uv_16_2;                                                                                     \
	b_16_2 = b_uv_16_2;                                                                                     \
                                                                                                            \
	y = LOAD_SI128((const __m128i *)(y_ptr2));                                                              \
	y = _mm_sub_epi8(y, _mm_set1_epi8(YUV2RGBParam::y_offset));                                             \
	y_16_1 = _mm_unpacklo_epi8(y, _mm_setzero_si128());                                                     \
	y_16_2 = _mm_unpackhi_epi8(y, _mm_setzero_si128());                                                     \
                                                                                                            \
	ADD_Y2RGB_16(y_16_1, y_16_2, r_16_1, g_16_1, b_16_1, r_16_2, g_16_2, b_16_2)                            \
                                                                                                            \
	__m128i r_8_21 = _mm_packus_epi16(r_16_1, r_16_2);                                                      \
	__m128i g_8_21 = _mm_packus_epi16(g_16_1, g_16_2);                                                      \
	__m128i b_8_21 = _mm_packus_epi16(b_16_1, b_16_2);                                                      \
                                                                                                            \
	/* process last 16 pixels of first line */                                                              \
	u_16 = _mm_srai_epi16(_mm_unpackhi_epi8(u, u), 8);                                                      \
	v_16 = _mm_srai_epi16(_mm_unpackhi_epi8(v, v), 8);                                                      \
                                                                                                            \
	UV2RGB_16(u_16, v_16, r_uv_16_1, g_uv_16_1, b_uv_16_1, r_uv_16_2, g_uv_16_2, b_uv_16_2)                 \
	r_16_1 = r_uv_16_1;                                                                                     \
	g_16_1 = g_uv_16_1;                                                                                     \
	b_16_1 = b_uv_16_1;                                                                                     \
	r_16_2 = r_uv_16_2;                                                                                     \
	g_16_2 = g_uv_16_2;                                                                                     \
	b_16_2 = b_uv_16_2;                                                                                     \
                                                                                                            \
	y = LOAD_SI128((const __m128i *)(y_ptr1 + 16));                                                         \
	y = _mm_sub_epi8(y, _mm_set1_epi8(YUV2RGBParam::y_offset));                                             \
	y_16_1 = _mm_unpacklo_epi8(y, _mm_setzero_si128());                                                     \
	y_16_2 = _mm_unpackhi_epi8(y, _mm_setzero_si128());                                                     \
                                                                                                            \
	ADD_Y2RGB_16(y_16_1, y_16_2, r_16_1, g_16_1, b_16_1, r_16_2, g_16_2, b_16_2)                            \
                                                                                                            \
	__m128i r_8_12 = _mm_packus_epi16(r_16_1, r_16_2);                                                      \
	__m128i g_8_12 = _mm_packus_epi16(g_16_1, g_16_2);                                                      \
	__m128i b_8_12 = _mm_packus_epi16(b_16_1, b_16_2);                                                      \
                                                                                                            \
	/* process last 16 pixels of second line */                                                             \
	r_16_1 = r_uv_16_1;                                                                                     \
	g_16_1 = g_uv_16_1;                                                                                     \
	b_16_1 = b_uv_16_1;                                                                                     \
	r_16_2 = r_uv_16_2;                                                                                     \
	g_16_2 = g_uv_16_2;                                                                                     \
	b_16_2 = b_uv_16_2;                                                                                     \
                                                                                                            \
	y = LOAD_SI128((const __m128i *)(y_ptr2 + 16));                                                         \
	y = _mm_sub_epi8(y, _mm_set1_epi8(YUV2RGBParam::y_offset));                                             \
	y_16_1 = _mm_unpacklo_epi8(y, _mm_setzero_si128());                                                     \
	y_16_2 = _mm_unpackhi_epi8(y, _mm_setzero_si128());                                                     \
                                                                                                            \
	ADD_Y2RGB_16(y_16_1, y_16_2, r_16_1, g_16_1, b_16_1, r_16_2, g_16_2, b_16_2)                            \
                                                                                                            \
	__m128i r_8_22 = _mm_packus_epi16(r_16_1, r_16_2);                                                      \
	__m128i g_8_22 = _mm_packus_epi16(g_16_1, g_16_2);                                                      \
	__m128i b_8_22 = _mm_packus_epi16(b_16_1, b_16_2);                                                      \
                                                                                                            \
	__m128i rgb_1, rgb_2, rgb_3, rgb_4, rgb_5, rgb_6;                                                       \
                                                                                                            \
	PACK_RGB24_32(r_8_11, r_8_12, g_8_11, g_8_12, b_8_11, b_8_12, rgb_1, rgb_2, rgb_3, rgb_4, rgb_5, rgb_6) \
	SAVE_SI128((__m128i *)(rgb_ptr1), rgb_1);                                                               \
	SAVE_SI128((__m128i *)(rgb_ptr1 + 16), rgb_2);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr1 + 32), rgb_3);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr1 + 48), rgb_4);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr1 + 64), rgb_5);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr1 + 80), rgb_6);                                                          \
                                                                                                            \
	PACK_RGB24_32(r_8_21, r_8_22, g_8_21, g_8_22, b_8_21, b_8_22, rgb_1, rgb_2, rgb_3, rgb_4, rgb_5, rgb_6) \
	SAVE_SI128((__m128i *)(rgb_ptr2), rgb_1);                                                               \
	SAVE_SI128((__m128i *)(rgb_ptr2 + 16), rgb_2);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr2 + 32), rgb_3);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr2 + 48), rgb_4);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr2 + 64), rgb_5);                                                          \
	SAVE_SI128((__m128i *)(rgb_ptr2 + 80), rgb_6);

#define YUV2RGB_32_PLANAR \
	LOAD_UV_PLANAR        \
	YUV2RGB_32

void yuv420_rgb24(
		uint32_t width, uint32_t height,
		const uint8_t *Y, const uint8_t *U, const uint8_t *V, uint32_t Y_stride, uint32_t UV_stride,
		uint8_t *RGB, uint32_t RGB_stride) {
#define LOAD_SI128 _mm_load_si128
#define SAVE_SI128 _mm_stream_si128

	uint32_t x, y;
	for (y = 0; y < (height - 1); y += 2) {
		const uint8_t *y_ptr1 = Y + y * Y_stride,
					  *y_ptr2 = Y + (y + 1) * Y_stride,
					  *u_ptr = U + (y / 2) * UV_stride,
					  *v_ptr = V + (y / 2) * UV_stride;

		uint8_t *rgb_ptr1 = RGB + y * RGB_stride,
				*rgb_ptr2 = RGB + (y + 1) * RGB_stride;

		for (x = 0; x < (width - 31); x += 32) {
			YUV2RGB_32_PLANAR

			y_ptr1 += 32;
			y_ptr2 += 32;
			u_ptr += 16;
			v_ptr += 16;
			rgb_ptr1 += 96;
			rgb_ptr2 += 96;
		}
	}
#undef LOAD_SI128
#undef SAVE_SI128
}

#endif //__SSE2__

#endif // GODOTREMOTE_LIBJPG_TURBO_ENABLED
