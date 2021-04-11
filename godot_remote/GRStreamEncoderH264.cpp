/* GRStreamEncoderH264.cpp */

#if !defined(NO_GODOTREMOTE_SERVER) && defined(GODOTREMOTE_H264_ENABLED)

//#define DEBUG_H264
//#define DEBUG_H264_WRITE_RAW

#include "GRStreamEncoderH264.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRServer.h"
#include "GRStreamEncoders.h"
#include "GRUtilsH264Codec.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY

#ifdef DEBUG_H264
#include "core/io/file_access_pack.h"
#endif
#include "core/os/thread_safe.h"
#include "scene/main/node.h"
#else

#ifdef DEBUG_H264
#include <File.hpp>
#include <JSON.hpp>
#include <JSONParseResult.hpp>
typedef JSON _JSON;
#endif
#include <Node.hpp>
using namespace godot;
#endif

using namespace GRUtils;

///////////////////////////////////////////////////////////////////////////////
// H264 ENCODER

#ifndef GDNATIVE_LIBRARY

void GRStreamEncoderH264::_bind_methods() {
	ClassDB::bind_method(D_METHOD(NAMEOF(_processing_thread), "user_data"), &GRStreamEncoderH264::_processing_thread);
}

#else

void GRStreamEncoderH264::_register_methods() {
	METHOD_REG(GRStreamEncoderH264, _processing_thread);
}

#endif

void GRStreamEncoderH264::_notification(int p_notification) {
	switch (p_notification) {
		case NOTIFICATION_POSTINITIALIZE:
#ifndef GDNATIVE_LIBRARY
			_init();
#endif
			break;
		case NOTIFICATION_PREDELETE:
			_deinit();
			GRStreamEncoder::_deinit();
			break;
	}
}

void GRStreamEncoderH264::clear_buffers() {
	Scoped_lock(ts_lock);
	while (buffer.size())
		buffer.pop();
	while (images.size())
		images.pop();
}

void GRStreamEncoderH264::commit_stream_end() {
	Scoped_lock(ts_lock);
	clear_buffers();

	auto b = shared_new(BufferedImage);
	b->is_ready = true;
	b->pack = shared_cast(GRPacketStreamDataH264, create_stream_end_pack());
	buffer.push(b);
}

std::shared_ptr<GRPacketStreamData> GRStreamEncoderH264::create_stream_end_pack() {
	auto p = shared_new(GRPacketStreamDataH264);
	p->set_is_stream_end(true);
	return p;
}

bool GRStreamEncoderH264::has_data_to_send() {
	Scoped_lock(ts_lock);
	return buffer.size() && buffer.front()->is_ready;
}

std::shared_ptr<GRPacketStreamData> GRStreamEncoderH264::pop_data_to_send() {
	Scoped_lock(ts_lock);
	auto buf_img = buffer.front();
	buffer.pop();
	return buf_img->pack;
}

int GRStreamEncoderH264::get_max_queued_frames() {
	return 32;
}

void GRStreamEncoderH264::start_encoder_threads(int count) {
	ZoneScopedNC("Start Encoder Threads", tracy::Color::Firebrick);
	if (count >= 0) {
		Scoped_lock(ts_lock);
		int prop_val = count == 1 ? 0 : count;
		bool upd = encoder_props.threads_count != count;

		if (!is_thread_active) {
			is_thread_active = true;
			encoder_props.threads_count = prop_val;
			Thread_start(thread, this, _processing_thread, Variant());
		} else {
			if (upd) {
				encoder_props.threads_count = prop_val;
			}
		}
	} else {
		stop_encoder_threads();
	}
}

void GRStreamEncoderH264::stop_encoder_threads() {
	ZoneScopedNC("Stop Encoder Threads", tracy::Color::Firebrick3);
	is_thread_active = false;
	Thread_close(thread);
	clear_buffers();
}

void GRStreamEncoderH264::_init() {
	LEAVE_IF_EDITOR();

#ifndef GDNATIVE_LIBRARY
#else
	GRStreamEncoder::_init();
#endif
}

void GRStreamEncoderH264::_deinit() {
	LEAVE_IF_EDITOR();
	stop_encoder_threads();
	while (buffer.size()) {
		buffer.pop();
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// H264 Encoder
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void _get_encoder_params(ISVCEncoder *h264_encoder, SEncParamExt *p_param, GRStreamEncoderH264::EncoderProperties *props) {
#ifdef DEBUG_H264
#define GET_PARAM_FROM_CFG(var, def_val, _type_conv)                                                                           \
	{                                                                                                                          \
		if (dict.has(#var) && (def_val != _type_conv dict[#var])) {                                                            \
			var = _type_conv dict[#var];                                                                                       \
			_log("Loaded var from \"" + params_cfg + "\". Name: " + #var + " Value: " + str(dict[#var]), LogLevel::LL_NORMAL); \
		} else {                                                                                                               \
			var = def_val;                                                                                                     \
		}                                                                                                                      \
	}

	Error err = Error::OK;
	String params_cfg = "params.json";
	Dictionary dict;
	auto s = file_get_as_string(params_cfg, &err);
	if (err == Error::OK) {
		dict = (Dictionary)(_JSON::get_singleton()->parse(s))->get_result();
	}
#else
#define GET_PARAM_FROM_CFG(var, def_val, _type_conv) var = def_val
#endif

	memset(p_param, 0, sizeof(SEncParamExt));
	{
		if (h264_encoder) {
			h264_encoder->GetDefaultParams(p_param);
		}
	}

	GET_PARAM_FROM_CFG(p_param->iUsageType, EUsageType::SCREEN_CONTENT_REAL_TIME, (EUsageType)(int));

	p_param->iPicWidth = props->pic_width; ///< same as in TagEncParamBase
	p_param->iPicHeight = props->pic_height; ///< same as in TagEncParamBase
	p_param->iTargetBitrate = props->target_bitrate; ///< same as in TagEncParamBase
	GET_PARAM_FROM_CFG(p_param->iRCMode, RC_MODES::RC_QUALITY_MODE, (RC_MODES)(int)); ///< same as in TagEncParamBase
	p_param->fMaxFrameRate = float(props->max_frame_rate); ///< same as in TagEncParamBase
	p_param->uiIntraPeriod = props->max_frame_rate; ///< period of Intra frame
	GET_PARAM_FROM_CFG(p_param->iNumRefFrame, -1, (int)); ///< number of reference frame used

	GET_PARAM_FROM_CFG(p_param->iTemporalLayerNum, 3, (int)); ///< temporal layer number, max temporal layer = 4
	GET_PARAM_FROM_CFG(p_param->iSpatialLayerNum, 1, (int)); ///< spatial layer number,1<= iSpatialLayerNum <= MAX_SPATIAL_LAYER_NUM, MAX_SPATIAL_LAYER_NUM = 128

	SSpatialLayerConfig *cfg = &p_param->sSpatialLayers[0];
	cfg->iVideoWidth = props->pic_width; ///< width of picture in luminance samples of a layer
	cfg->iVideoHeight = props->pic_height; ///< height of picture in luminance samples of a layer
	cfg->fFrameRate = float(props->max_frame_rate); ///< frame rate specified for a layer
	cfg->iSpatialBitrate = 0; ///< target bitrate for a spatial layer, in unit of bps
	cfg->iMaxSpatialBitrate = props->target_bitrate; ///< maximum  bitrate for a spatial layer, in unit of bps
	GET_PARAM_FROM_CFG(cfg->uiProfileIdc, EProfileIdc::PRO_HIGH, (EProfileIdc)(int)); ///< value of profile IDC (PRO_UNKNOWN for auto-detection)
	GET_PARAM_FROM_CFG(cfg->uiLevelIdc, ELevelIdc::LEVEL_UNKNOWN, (ELevelIdc)(int)); ///< value of profile IDC (0 for auto-detection)
	GET_PARAM_FROM_CFG(cfg->iDLayerQp, 0, (int)); ///< value of level IDC (0 for auto-detection)

	cfg->sSliceArgument.uiSliceMode = SliceModeEnum::SM_SINGLE_SLICE;
	cfg->sSliceArgument.uiSliceNum = 0; // AUTO
	cfg->sSliceArgument.uiSliceSizeConstraint = 1500;

	// Note: members bVideoSignalTypePresent through uiColorMatrix below are also defined in SWelsSPS in parameter_sets.h.
	GET_PARAM_FROM_CFG(cfg->bVideoSignalTypePresent, true, (bool)); // false => do not write any of the following information to the header
	GET_PARAM_FROM_CFG(cfg->uiVideoFormat, EVideoFormatSPS::VF_UNDEF, (EVideoFormatSPS)(int)); // EVideoFormatSPS; 3 bits in header; 0-5 => component, kpal, ntsc, secam, mac, undef
	GET_PARAM_FROM_CFG(cfg->bFullRange, true, (bool)); // false => analog video data range [16, 235]; true => full data range [0,255]
	GET_PARAM_FROM_CFG(cfg->bColorDescriptionPresent, true, (bool)); // false => do not write any of the following three items to the header
	GET_PARAM_FROM_CFG(cfg->uiColorPrimaries, EColorPrimaries::CP_UNDEF, (EColorPrimaries)(int)); // EColorPrimaries; 8 bits in header; 0 - 9 => ???, bt709, undef, ???, bt470m, bt470bg,	//    smpte170m, smpte240m, film, bt2020
	GET_PARAM_FROM_CFG(cfg->uiTransferCharacteristics, ETransferCharacteristics::TRC_UNDEF, (ETransferCharacteristics)(int)); // ETransferCharacteristics; 8 bits in header; 0 - 15 => ???, bt709, undef, ???, bt470m, bt470bg, smpte170m,	//   smpte240m, linear, log100, log316, iec61966-2-4, bt1361e, iec61966-2-1, bt2020-10, bt2020-12
	GET_PARAM_FROM_CFG(cfg->uiColorMatrix, EColorMatrix::CM_YCGCO, (EColorMatrix)(int)); // EColorMatrix; 8 bits in header (corresponds to FFmpeg "colorspace"); 0 - 10 => GBR, bt709,	//   undef, ???, fcc, bt470bg, smpte170m, smpte240m, YCgCo, bt2020nc, bt2020c

	GET_PARAM_FROM_CFG(cfg->bAspectRatioPresent, false, (bool)); ///< aspect ratio present in VUI
	//cfg.eAspectRatio = ESampleAspectRatio::ASP_UNSPECIFIED; ///< aspect ratio idc
	//cfg.sAspectRatioExtWidth ; ///< use if aspect ratio idc == 255
	//cfg.sAspectRatioExtHeight; ///< use if aspect ratio idc == 255

	GET_PARAM_FROM_CFG(p_param->iComplexityMode, ECOMPLEXITY_MODE::MEDIUM_COMPLEXITY, (ECOMPLEXITY_MODE)(int));
	GET_PARAM_FROM_CFG(p_param->eSpsPpsIdStrategy, EParameterSetStrategy::INCREASING_ID, (EParameterSetStrategy)(int)); ///< different strategy in adjust ID in SPS/PPS: 0- constant ID, 1-additional ID, 6-mapping and additional
	GET_PARAM_FROM_CFG(p_param->bPrefixNalAddingCtrl, false, (bool)); ///< false:not use Prefix NAL; true: use Prefix NAL
	GET_PARAM_FROM_CFG(p_param->bEnableSSEI, true, (bool)); ///< false:not use SSEI; true: use SSEI -- todo: planning to remove the interface of SSEI
	GET_PARAM_FROM_CFG(p_param->bSimulcastAVC, false, (bool)); ///< (when encoding more than 1 spatial layer) false: use SVC syntax for higher layers; true: use Simulcast AVC
	GET_PARAM_FROM_CFG(p_param->iPaddingFlag, 0, (int)); ///< 0:disable padding;1:padding
	GET_PARAM_FROM_CFG(p_param->iEntropyCodingModeFlag, 0, (int)); ///< 0:CAVLC  1:CABAC.

	// rc control
	GET_PARAM_FROM_CFG(p_param->bEnableFrameSkip, true, (bool)); ///< False: don't skip frame even if VBV buffer overflow.True: allow skipping frames to keep the bitrate within limits
	p_param->iMaxBitrate = props->target_bitrate; ///< the maximum bitrate, in unit of bps, set it to UNSPECIFIED_BIT_RATE if not needed
	GET_PARAM_FROM_CFG(p_param->uiMaxNalSize, 0, (int)); ///< the maximum NAL size.  This value should be not 0 for dynamic slice mode
	GET_PARAM_FROM_CFG(p_param->iMaxQp, 35, (int));
	GET_PARAM_FROM_CFG(p_param->iMinQp, 26, (int));

	// LTR settings
	GET_PARAM_FROM_CFG(p_param->bEnableLongTermReference, 0, (int)); ///< 1: on, 0: off
	GET_PARAM_FROM_CFG(p_param->iLTRRefNum, 0, (int)); ///< the number of LTR(long term reference),todo: not supported to set it arbitrary yet
	GET_PARAM_FROM_CFG(p_param->iLtrMarkPeriod, props->max_frame_rate / 2, (int)); ///< the LTR marked period that is used in feedback.

	// multi-thread settings
	p_param->iMultipleThreadIdc = props->threads_count; ///< 1 # 0: auto(dynamic imp. internal encoder); 1: multiple threads imp. disabled; lager than 1: count number of threads;
	GET_PARAM_FROM_CFG(p_param->bUseLoadBalancing, false, (bool)); ///< only used when uiSliceMode=1 or 3, will change slicing of a picture during the run-time of multi-thread encoding, so the result of each run may be different

	// Deblocking loop filter
	GET_PARAM_FROM_CFG(p_param->iLoopFilterDisableIdc, 0, (int)); ///< 0: on, 1: off, 2: on except for slice boundaries
	GET_PARAM_FROM_CFG(p_param->iLoopFilterAlphaC0Offset, 0, (int)); ///< AlphaOffset: valid range [-6, 6], default 0
	GET_PARAM_FROM_CFG(p_param->iLoopFilterBetaOffset, 0, (int)); ///< BetaOffset: valid range [-6, 6], default 0

	// pre-processing feature
	GET_PARAM_FROM_CFG(p_param->bEnableDenoise, false, (bool)); ///< denoise control
	GET_PARAM_FROM_CFG(p_param->bEnableBackgroundDetection, false, (bool)); ///< background detection control //VAA_BACKGROUND_DETECTION //BGD cmd
	GET_PARAM_FROM_CFG(p_param->bEnableAdaptiveQuant, false, (bool)); ///< adaptive quantization control
	GET_PARAM_FROM_CFG(p_param->bEnableFrameCroppingFlag, true, (bool)); ///< enable frame cropping flag: TRUE always in application
	GET_PARAM_FROM_CFG(p_param->bEnableSceneChangeDetect, true, (bool));
	GET_PARAM_FROM_CFG(p_param->bIsLosslessLink, false, (bool)); ///<  LTR advanced setting
}

void GRStreamEncoderH264::_processing_thread(Variant p_userdata) {
	Thread_set_name("Stream Encoder: H264");
	OS *os = OS::get_singleton();

	uint64_t frames_encoded = 0;

	ISVCEncoder *h264_encoder = nullptr;
	SSourcePicture h264_encoder_pic;
	SFrameBSInfo h264_stream_info;
	GRUtilsH264Codec::yuv_buffer_data_to_encoder yuv_buffer;
	EncoderProperties current_props;
	GRUtilsH264Codec::_encoder_create(&h264_encoder);

	bool is_encoder_inited = false;
	bool error_shown = false;
	bool first_send_quality_hint = false;

#ifdef DEBUG_H264
	auto d = memnew(_Directory);
	d->remove("stream.264");
	auto file_open(f, "stream.264", _File::ModeFlags::WRITE);
#ifdef DEBUG_H264_WRITE_RAW
	d->remove("stream.yuv");
	auto file_open(f_yuv, "stream.yuv", _File::ModeFlags::WRITE);
#endif
#endif

	while (is_thread_active) {
		ts_lock.lock();

		if (images.size() == 0) {
			ts_lock.unlock();
			sleep_usec(1_ms);
			continue;
		}

		CommitedImage com_image = images.front();
		images.pop();

		while (buffer.size() > get_max_queued_frames()) {
			buffer.pop();
		}

		if (buffer.size()) {
			shared_cast_def(GRPacketStreamDataH264, im, buffer.front()->pack);
			if (im && im->get_is_stream_end()) {
				ts_lock.unlock();
				continue;
			}
		}

		auto buf_img = std::make_shared<BufferedImage>();
		buffer.push(buf_img);

		auto pack = shared_new(GRPacketStreamDataH264);
		int bytes_in_color = com_image.img_format == Image::FORMAT_RGB8 ? 3 : 4;

		GRServer *srv = cast_to<GRServer>(GodotRemote::get_singleton()->get_device());

		encoder_props.max_frame_rate = srv ? int(srv->get_target_fps() * (1.f / (srv->get_skip_frames() + 1))) : 60;
		encoder_props.pic_width = com_image.img_width;
		encoder_props.pic_height = com_image.img_height;

		int new_bitrate = int((com_image.img_width * com_image.img_height * encoder_props.max_frame_rate) * (viewport->get_stream_quality() / 100.f));
		// Send hint to client
		if (encoder_props.target_bitrate != new_bitrate || !first_send_quality_hint) {
			auto quality_hint = shared_new(GRPacketServerStreamQualityHint);
			quality_hint->set_hint(str(int(new_bitrate / 1024)) + " KBps");
			if (srv)
				srv->send_packet(quality_hint);
			first_send_quality_hint = true;
		}

		encoder_props.target_bitrate = new_bitrate;
		ts_lock.unlock();

		int width = encoder_props.pic_width;
		int height = encoder_props.pic_height;

		pack->set_is_stream_end(false);
		pack->set_start_time(com_image.time_added);
		pack->set_frametime(com_image.frametime);
		pack->set_compression_type(GRDevice::ImageCompressionType::COMPRESSION_H264);

		if (com_image.img_data.size() == 0)
			goto end;

		if (h264_encoder) {
			ZoneScopedNC("Working with encoder", tracy::Color::VioletRed2);
			ts_lock.lock();
			// Update params
			if (encoder_props != current_props) {
				commit_stream_end(); // clear all buffers and send first packet with reset data
				buffer.push(buf_img); // then add current editing buffer to queue again

#ifdef DEBUG_H264
				f->close();
				memdelete(f);
				d->remove("stream.264");
				file_open(f, "stream.264", _File::ModeFlags::WRITE);
#ifdef DEBUG_H264_WRITE_RAW
				f_yuv->close();
				memdelete(f_yuv);
				d->remove("stream.yuv");
				file_open(f_yuv, "stream.yuv", _File::ModeFlags::WRITE);
#endif // DEBUG_H264_WRITE_RAW
#endif

				frames_encoded = 0;
				current_props = encoder_props;
				error_shown = false;
				ts_lock.unlock();

				SEncParamExt param;
				_get_encoder_params(h264_encoder, &param, &current_props);
				int err = 0;
				if (is_encoder_inited) {
					err = h264_encoder->SetOption(ENCODER_OPTION::ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &param);
					if (err) {
						is_encoder_inited = false;
						_log("Can't change OpenH264 encoder parameters. Code: " + str(err), LogLevel::LL_ERROR);
						goto reinit_end;
					}
				} else {
					err = h264_encoder->InitializeExt(&param);
					if (err) {
						is_encoder_inited = false;
						_log("Can't initialize OpenH264 encoder. Code: " + str(err), LogLevel::LL_ERROR);
						goto reinit_end;
					}
				}

				is_encoder_inited = true;
				yuv_buffer.update_size(current_props.pic_width, current_props.pic_height);
				memset(&h264_stream_info, 0, sizeof(SFrameBSInfo));

				memset(&h264_encoder_pic, 0, sizeof(SSourcePicture));
				h264_encoder_pic.iPicWidth = current_props.pic_width;
				h264_encoder_pic.iPicHeight = current_props.pic_height;
				h264_encoder_pic.iColorFormat = videoFormatI420;
				h264_encoder_pic.iStride[0] = h264_encoder_pic.iPicWidth;
				h264_encoder_pic.iStride[1] = h264_encoder_pic.iStride[2] = h264_encoder_pic.iPicWidth >> 1;
				h264_encoder_pic.pData[0] = yuv_buffer.buf[0];
				h264_encoder_pic.pData[1] = yuv_buffer.buf[1];
				h264_encoder_pic.pData[2] = yuv_buffer.buf[2];

			reinit_end:
				(void *)0;
			} else {
				ts_lock.unlock();
			}

			if (is_encoder_inited) {
				if (GRUtilsH264Codec::_encode_image_to_yuv(com_image.img_data, width, height, bytes_in_color, yuv_buffer.buf) == Error::OK) {
#if defined(DEBUG_H264) && defined(DEBUG_H264_WRITE_RAW)
					f_yuv->store_buffer(store_data_to_file(yuv_buffer.buf[0], yuv_buffer.y_size));
					f_yuv->store_buffer(store_data_to_file(yuv_buffer.buf[1], yuv_buffer.uv_size));
					f_yuv->store_buffer(store_data_to_file(yuv_buffer.buf[2], yuv_buffer.uv_size));
#endif

					h264_encoder_pic.uiTimeStamp = (int32_t)(0.5 + (frames_encoded * (1000 / current_props.max_frame_rate)));
					//h264_encoder->ForceIntraFrame((frames_encoded++ % en_max_frame_rate) == 0);

					int rv = h264_encoder->EncodeFrame(&h264_encoder_pic, &h264_stream_info);
					if (rv != cmResultSuccess) {
						_log("Can't encode frame. Code: " + str(rv), LogLevel::LL_ERROR);
						continue;
					}

					if (h264_stream_info.eFrameType != EVideoFrameType::videoFrameTypeSkip) {
						pack->set_frame_type(h264_stream_info.eFrameType);
						int iFrameSize = 0;
						int iLayer = 0;
						while (iLayer < h264_stream_info.iLayerNum) {
							SLayerBSInfo *pLayerBsInfo = &h264_stream_info.sLayerInfo[iLayer];
							if (pLayerBsInfo != NULL) {
								int iLayerSize = 0;
								int iNalIdx = pLayerBsInfo->iNalCount - 1;
								do {
									iLayerSize += pLayerBsInfo->pNalLengthInByte[iNalIdx];
									--iNalIdx;
								} while (iNalIdx >= 0);
								iFrameSize += iLayerSize;

								pack->add_image_data(pLayerBsInfo->pBsBuf, iLayerSize);
#ifdef DEBUG_H264
								f->store_buffer(put_data_from_array_pointer(pack->get_image_data()[pack->get_image_data().size() - 1]));
#endif
							}
							++iLayer;
						}

						_log("H264 Encoder buffered image size: " + str(iFrameSize) + "\tType: " + str(h264_stream_info.eFrameType), LogLevel::LL_DEBUG);
					}
				} else {
					_log("Can't encode RGB Image to YUV buffer", LogLevel::LL_ERROR);
				}
			} else {
				sleep_usec(1_ms);
				if (!error_shown) {
					_log("The H264 encoder is null or not initialized.", LogLevel::LL_ERROR);
					error_shown = true;
				}
			}
		} else {
			sleep_usec(25_ms);
			// just wait thread closing...
		}
	end:
		if (pack->get_image_data().size()) {
			ZoneScopedNC("Push Image to Buffer", tracy::Color::Violet);
			buf_img->pack = pack;
			buf_img->is_ready = true;
		}
	}

	is_encoder_inited = false;
	if (h264_encoder) {
		h264_encoder->Uninitialize();
		GRUtilsH264Codec::_encoder_free(h264_encoder);
	}
	h264_encoder = nullptr;

#ifdef DEBUG_H264
	f->close();
	memdelete(f);
	memdelete(d);
#ifdef DEBUG_H264_WRITE_RAW
	f_yuv->close();
	memdelete(f_yuv);
#endif
#endif
}

#endif // !NO_GODOTREMOTE_SERVER
