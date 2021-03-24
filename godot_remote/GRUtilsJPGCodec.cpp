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

	buf = new uint8_t *[3];
	buf[0] = new uint8_t[y];
	buf[1] = new uint8_t[u];
	buf[2] = new uint8_t[v];

	y_size = y;
	u_size = u;
	v_size = v;
}

void GRUtilsJPGCodec::yuv_buffer_data_to_encoder::free_mem() {
	if (buf) {
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
		delete[] buf;
	}
}

void GRUtilsJPGCodec::yuv_buffer_data_to_decoder::free_mem() {
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

Error GRUtilsJPGCodec::_encode_image_to_yuv(Ref<Image> img, int width, int height, int bytes_in_color, uint8_t **buf) {
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

Error GRUtilsJPGCodec::_decode_yuv_to_image(Ref<Image> &img, int width, int height, uint8_t *buf_y, uint8_t *buf_u, uint8_t *buf_v) {
	tjhandle _jpegDecompressor = tjInitDecompress();
	uint8_t *yuv[] = {
		buf_y,
		buf_u,
		buf_v,
	};
	const uint8_t **yuv_const = (const uint8_t **)yuv;
	int strides[] = {
		_get_yuv_comp_size(0, width, height),
		_get_yuv_comp_size(1, width, height),
		_get_yuv_comp_size(2, width, height)
	};

	PoolByteArray rgb;
	rgb.resize(width * height * 3);
	auto rw = rgb.write();

	auto img_data = img->get_data().read();
	int err = tjDecodeYUVPlanes(_jpegDecompressor, yuv_const, strides, TJSAMP_420, rw.ptr(), width, 0, height, TJPF_RGB, TJFLAG_FASTDCT | TJFLAG_NOREALLOC);
	if (err) {
		tjDestroy(_jpegDecompressor);
		return Error::FAILED;
	}
	tjDestroy(_jpegDecompressor);

	release_pva_write(rw);
	img.instance();
#ifndef GDNATIVE_LIBRARY
	img->create(width, height, false, Image::Format::FORMAT_RGB8, rgb);
#else
	img->create_from_data(width, height, false, Image::Format::FORMAT_RGB8, rgb);
#endif

	return Error::OK;
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

// richgel999/jpeg-compressor: https://github.com/richgel999/jpeg-compressor
#include "jpge.h"

#define CLIP_VAL(v) (v > 255 ? 255 : (v < 0 ? 0 : v))

// https://www.programmersought.com/article/97924644549/
bool GRUtilsJPGCodec::rgb2yuv(const uint8_t *RgbBuf, int w, int h, int pixel_size, uint8_t **yuvBuf) {
	uint8_t *ptrY, *ptrU, *ptrV;
	const uint8_t *ptrRGB;

	ptrY = yuvBuf[0];
	ptrU = yuvBuf[1];
	ptrV = yuvBuf[2];
	unsigned char y, u, v, r, g, b;
	for (int j = 0; j < h; j++) {
		ptrRGB = RgbBuf + w * j * 3;
		for (int i = 0; i < w; i++) {

			r = *(ptrRGB++);
			g = *(ptrRGB++);
			b = *(ptrRGB++);
			if (pixel_size == 4)
				ptrRGB++;

			y = (unsigned char)((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
			u = (unsigned char)((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
			v = (unsigned char)((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
			*(ptrY++) = CLIP_VAL(y);
			if (j % 2 == 0 && i % 2 == 0) {
				*(ptrU++) = CLIP_VAL(u);
			} else {
				if (i % 2 == 0) {
					*(ptrV++) = CLIP_VAL(v);
				}
			}
		}
	}
	return true;
}

#undef CLIP_VAL

bool GRUtilsJPGCodec::yuv2rgb(const uint8_t **yuv, int w, int h, uint8_t *rgb) {
	const uint8_t *ptrY = yuv[0],
				  *ptrU = yuv[1],
				  *ptrV = yuv[2];

	for (int i = 0; i < w * h / 4; i++) {
		rgb[i * 3 + 2] = ptrY[i] + 1.772 * (ptrU[i] - 128); //B = Y +1.779*(U-128)
		rgb[i * 3 + 1] = ptrY[i] - 0.34413 * (ptrU[i] - 128) - 0.71414 * (ptrV[i] - 128); //G = Y-0.3455*(U-128)-0.7169*(V-128)
		rgb[i * 3 + 0] = ptrY[i] + 1.402 * (ptrV[i] - 128); //R = Y+1.4075*(V-128)
	}
	return true;
}

Error GRUtilsJPGCodec::_compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color, int quality) {
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

	ERR_FAIL_COND_V_MSG(!jpge::compress_image_to_jpeg_file_in_memory((void *)wb.ptr(), size, width, height, bytes_for_color,
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

Error GRUtilsJPGCodec::_encode_image_to_yuv(Ref<Image> img, int width, int height, int bytes_in_color, uint8_t **buf) {
	auto img_data = img->get_data().read();
	if (!rgb2yuv(img_data.ptr(), width, height, bytes_in_color, buf)) {
		return Error::FAILED;
	}

	return Error::OK;
}

Error GRUtilsJPGCodec::_decode_yuv_to_image(Ref<Image> &img, int width, int height, uint8_t *buf_y, uint8_t *buf_u, uint8_t *buf_v) {
	uint8_t *yuv[] = {
		buf_y,
		buf_u,
		buf_v,
	};
	const uint8_t **yuv_const = (const uint8_t **)yuv;
	int strides[] = {
		_get_yuv_comp_size(0, width, height),
		_get_yuv_comp_size(1, width, height),
		_get_yuv_comp_size(2, width, height)
	};

	PoolByteArray rgb;
	rgb.resize(width * height * 3);
	auto rw = rgb.write();

	auto img_data = img->get_data().read();
	if (!yuv2rgb(yuv_const, width, height, rw.ptr())) {
		return Error::FAILED;
	}

	release_pva_write(rw);
	img.instance();
	img->create_from_data(width, height, false, Image::Format::FORMAT_RGB8, rgb);

	return Error::OK;
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

#endif // GODOTREMOTE_LIBJPG_TURBO_ENABLED
