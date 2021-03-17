/* GRJPGCodec.cpp */

#include "GRJPGCodec.h"
using namespace GRUtils;

#ifdef GODOTREMOTE_LIBJPEG_TURBO_ENABLED

#include "libjpeg-turbo/include/turbojpeg.h"

Error GRJPGCodec::_compress_jpg_turbo(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color, int quality) {
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

Error GRJPGCodec::_decompress_jpg_turbo(PoolByteArray &img_data, PoolByteArray &jpg_buffer, Ref<Image> *out_img) {
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

			img->create(width, height, false, Image::Format::FORMAT_RGB8, fin_data);
			if (!img->empty()) {
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

Error GRJPGCodec::_compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, PoolByteArray &jpg_buffer, int width, int height, int bytes_for_color, int quality) {
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
		release_pva_read(wb);
		release_pva_write(wr);
	}

	_log("JPG size: " + str(res.size()), LogLevel::LL_DEBUG);

	ret = res;
	return Error::OK;
}

#endif // GODOTREMOTE_LIBJPG_TURBO_ENABLED
