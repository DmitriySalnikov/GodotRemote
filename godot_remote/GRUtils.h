/* GRUtils.h */
#ifndef GRUTILS_H
#define GRUTILS_H

#include "core/image.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "core/pool_vector.h"
#include "core/project_settings.h"
#include "jpge.h"
#include "main/input_default.h"

#ifdef DEBUG_ENABLED

#define TimeCountInit() int simple_time_counter = OS::get_singleton()->get_ticks_usec()
#define TimeCountReset() simple_time_counter = OS::get_singleton()->get_ticks_usec()
#define TimeCount(str)                                                                                                                                        \
	GRUtils::log(str + String(": ") + String::num_real((OS::get_singleton()->get_ticks_usec() - simple_time_counter) / 1000.0), GRUtils::LogLevel::LL_Debug); \
	simple_time_counter = OS::get_singleton()->get_ticks_usec()

#else

#define TimeCountInit()
#define TimeCountReset()
#define TimeCount(str)

#endif // DEBUG_ENABLED

// Get Project Setting
#define GET_PS(setting_name) \
	ProjectSettings::get_singleton()->get_setting(setting_name)
// Get Project Setting and set it to variable
#define GET_PS_SET(variable_to_store, setting_name) \
	variable_to_store = ProjectSettings::get_singleton()->get_setting(setting_name)

namespace GRUtils {

enum LogLevel {
	LL_None = 255,
	LL_Debug = 0,
	LL_Normal = 1,
	LL_Warning = 2,
	LL_Error = 3,
};

enum class AuthErrorCode {
	OK = 0,
	VersionMismatch = 1,
};

enum Subsampling {
	SUBSAMPLING_Y_ONLY = 0,
	SUBSAMPLING_H1V1 = 1,
	SUBSAMPLING_H2V1 = 2,
	SUBSAMPLING_H2V2 = 3
};

extern int current_loglevel;
extern int compress_buffer_size_mb;
extern PoolByteArray internal_PACKET_HEADER;
extern PoolByteArray internal_VERSION;
extern PoolByteArray compress_buffer;

// defines

static void log(const Variant &val, LogLevel lvl = LogLevel::LL_Normal);
static String str(const Variant &val);

// literals

// conversion from usec to msec. most useful to OS::delay_usec()
constexpr uint32_t operator"" _ms(unsigned long long val) {
	return val * 1000;
}

// implementations

static void set_gravity(const Vector3 &p_gravity) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_gravity(p_gravity);
}

static void set_accelerometer(const Vector3 &p_accel) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_accelerometer(p_accel);
}

static void set_magnetometer(const Vector3 &p_magnetometer) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_magnetometer(p_magnetometer);
}

static void set_gyroscope(const Vector3 &p_gyroscope) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_gyroscope(p_gyroscope);
}

static PoolByteArray compress_jpg(Ref<Image> orig_img, int quality, int subsampling) {
	PoolByteArray res;
	ERR_FAIL_COND_V(!orig_img.ptr(), res);
	TimeCountInit();

	Image img;
	img.copy_internals_from(orig_img);
	TimeCount("Copy img");
	img.convert(Image::FORMAT_RGBA8);
	TimeCount("Convert img");

	jpge::params params;
	params.m_quality = quality;
	params.m_subsampling = (jpge::subsampling_t)subsampling;

	ERR_FAIL_COND_V(!params.check(), res);
	auto rb = compress_buffer.read();
	auto ri = img.get_data().read();

	int size = compress_buffer.size();
	ERR_FAIL_COND_V_MSG(!jpge::compress_image_to_jpeg_file_in_memory(
								(void *)rb.ptr(),
								size,
								img.get_width(),
								img.get_height(),
								4,
								(const unsigned char *)ri.ptr(),
								params),
			res, "Can't compress image.");

	rb.release();
	ri.release();
	TimeCount("Compress img");

	res.append_array(compress_buffer.subarray(0, size - 1));
	TimeCount("Combine arrays");

	log("JPG size: " + str(res.size()), LogLevel::LL_Debug);
	return res;
}

static void log(const Variant &val, LogLevel lvl) {
	if (lvl >= current_loglevel && lvl < LogLevel::LL_None) {
		if (lvl == LogLevel::LL_Error) {
			print_error("[GodotRemote Error] " + str(val));
		} else if (lvl == LogLevel::LL_Warning) {
			print_error("[GodotRemote Warning] " + str(val));
		} else {
			print_line("[GodotRemote] " + str(val));
		}
	}
}

template <class T>
static String str_arr(PoolVector<T> arr, const bool force_full = false, const int max_shown_items = 64) {
	String res = "[ ";
	int s = arr.size();
	bool is_long = false;
	if (s > max_shown_items && !force_full) {
		s = max_shown_items;
		is_long = true;
	}

	auto r = arr.read();
	for (int i = 0; i < s; i++) {
		res += str(r[i]);
		if (i != s - 1 || is_long) {
			res += ", ";
		}
	}
	r.release();

	if (is_long) {
		res += String::num_int64(int64_t(arr.size()) - s) + " more items...";
	}

	return res + " ]";
};
static String str_arr(const Array arr, const bool force_full = false, const int max_shown_items = 64) {
	String res = "[ ";
	int s = arr.size();
	bool is_long = false;
	if (s > max_shown_items && !force_full) {
		s = max_shown_items;
		is_long = true;
	}

	for (int i = 0; i < s; i++) {
		res += str(arr[i]);
		if (i != s - 1 || is_long) {
			res += ", ";
		}
	}

	if (is_long) {
		res += String::num_int64(int64_t(arr.size()) - s) + " more items...";
	}

	return res + " ]";
};
static String str_arr(const Dictionary arr, const bool force_full = false, const int max_shown_items = 32) {
	String res = "{ ";
	int s = arr.size();
	bool is_long = false;
	if (s > max_shown_items && !force_full) {
		s = max_shown_items;
		is_long = true;
	}

	for (int i = 0; i < s; i++) {
		res += str(arr.get_key_at_index(i)) + " : " + str(arr.get_value_at_index(i));
		if (i != s - 1 || is_long) {
			res += ", ";
		}
	}

	if (is_long) {
		res += String::num_int64(int64_t(arr.size()) - s) + " more items...";
	}

	return res + " }";
};
static String str_arr(const uint8_t *data, const int size, const bool force_full = false, const int max_shown_items = 64) {
	String res = "[ ";
	int s = size;
	bool is_long = false;
	if (s > max_shown_items && !force_full) {
		s = max_shown_items;
		is_long = true;
	}

	for (int i = 0; i < s; i++) {
		res += String::num_uint64(data[i]);
		if (i != s - 1 || is_long) {
			res += ", ";
		}
	}

	if (is_long) {
		res += String::num_int64(int64_t(size) - s) + " more bytes...";
	}

	return res + " ]";
};

static String str(const Variant &val) {
	Variant::Type type = val.get_type();
	switch (type) {
		case Variant::NIL: {
			return "NULL";
		}
		case Variant::BOOL: {
			return val ? "True" : "False";
		}
		case Variant::INT: {
			return String::num_int64(val);
		}
		case Variant::REAL: {
			return String::num_real(val);
		}
		case Variant::STRING: {
			return val;
		}
		case Variant::VECTOR2: {
			Vector2 v2 = val;
			return String("V2(") + String::num_real(v2.x) + ", " + String::num_real(v2.y) + ")";
		}
		case Variant::RECT2: {
			Rect2 r = val;
			return String("R2((") + String::num_real(r.position.x) + ", " + String::num_real(r.position.y) + "), (" + String::num_real(r.size.x) + ", " + String::num_real(r.size.y) + "))";
		}
		case Variant::VECTOR3: {
			Vector3 v3 = val;
			return String("V3(") + String::num_real(v3.x) + ", " + String::num_real(v3.y) + ", " + String::num_real(v3.z) + ")";
		}
		case Variant::TRANSFORM2D: {
			break;
		}
		case Variant::PLANE: {
			break;
		}
		case Variant::QUAT: {
			break;
		}
		case Variant::AABB: {
			break;
		}
		case Variant::BASIS: {
			break;
		}
		case Variant::TRANSFORM: {
			break;
		}
		case Variant::COLOR: {
			Color c = val;
			return String("C(") + String::num_real(c.r) + ", " + String::num_real(c.g) + ", " + String::num_real(c.b) + ", " + String::num_real(c.a) + ")";
		}
		case Variant::NODE_PATH: {
			NodePath np = val;
			return String("NP: ") + np + ")";
		}
		case Variant::_RID: {
			RID rid = val;
			return String("RID:") + String::num_int64(rid.get_id());
		}
		case Variant::OBJECT: {
			Object *obj = val;
			if (obj)
				return obj->to_string();
			else {
				return String("[NULL]");
			}
		}
		case Variant::DICTIONARY: {
			return str_arr((Dictionary)val);
		}
		case Variant::ARRAY: {
			return str_arr((Array)val);
		}
		case Variant::POOL_BYTE_ARRAY: {
			return str_arr((PoolByteArray)val);
		}
		case Variant::POOL_INT_ARRAY: {
			return str_arr((PoolIntArray)val);
		}
		case Variant::POOL_REAL_ARRAY: {
			return str_arr((PoolRealArray)val);
		}
		case Variant::POOL_STRING_ARRAY: {
			return str_arr((PoolStringArray)val);
		}
		case Variant::POOL_VECTOR2_ARRAY: {
			return str_arr((PoolVector2Array)val);
		}
		case Variant::POOL_VECTOR3_ARRAY: {
			return str_arr((PoolVector3Array)val);
		}
		case Variant::POOL_COLOR_ARRAY: {
			return str_arr((PoolColorArray)val);
		}
	}
	return String("|? ") + Variant::get_type_name(type) + " ?|";
}

static void log_array(const uint8_t *data, const int size, const bool force_full = false, const int max_shown_items = 64, LogLevel lvl = LogLevel::LL_Normal) {
	log(str_arr(data, size, force_full, max_shown_items), lvl);
}
static void log_array(const Array data, const bool force_full = false, const int max_shown_items = 64, LogLevel lvl = LogLevel::LL_Normal) {
	log(str_arr(data, force_full, max_shown_items), lvl);
}
static void log_array(const Dictionary data, const bool force_full = false, const int max_shown_items = 32, LogLevel lvl = LogLevel::LL_Normal) {
	log(str_arr(data, force_full, max_shown_items), lvl);
}
template <class T>
static void log_array(const PoolVector<T> data, const bool force_full = false, const int max_shown_items = 64, LogLevel lvl = LogLevel::LL_Normal) {
	log(str_arr(data, force_full, max_shown_items), lvl);
}

static PoolByteArray var2bytes(const Variant &data, bool full_objects = false) {
	PoolByteArray barr;
	int len;
	Error err = encode_variant(data, NULL, len, full_objects);
	if (err) {
		//log(String("Unexpected error in var2bytes: ") + String::num(err), LogLevel::LL_Error);
		return barr;
	}

	barr.resize(len);
	{
		PoolByteArray::Write w = barr.write();
		encode_variant(data, w.ptr(), len, full_objects);
	}
	return barr;
}

template <class T = Variant>
static T bytes2var(const PoolByteArray &data, bool allow_objects = false) {
	Variant ret;
	{
		PoolByteArray::Read r = data.read();
		Error err = decode_variant(ret, r.ptr(), data.size(), NULL, allow_objects);
		if (err) {
			//log("Not enough bytes for decoding bytes, or invalid format.", LogLevel::LL_Error);
			return Variant();
		}
	}

	return ret;
}

template <class T = Variant>
static T bytes2var(const uint8_t *data, int size, bool allow_objects = false) {
	Variant ret;
	{
		Error err = decode_variant(ret, data, size, NULL, allow_objects);
		if (err) {
			//log("Not enough bytes for decoding bytes, or invalid format.", LogLevel::LL_Error);
			return Variant();
		}
	}

	return ret;
}

static PoolByteArray int162bytes(const int16_t &data) {
	PoolByteArray res;
	res.resize(2);
	auto w = res.write();
	encode_uint16(data, w.ptr());
	return res;
}

static PoolByteArray int322bytes(const int32_t &data) {
	PoolByteArray res;
	res.resize(4);
	auto w = res.write();
	encode_uint32(data, w.ptr());
	return res;
}

static PoolByteArray int642bytes(const int64_t &data) {
	PoolByteArray res;
	res.resize(8);
	auto w = res.write();
	encode_uint64(data, w.ptr());
	return res;
}

static PoolByteArray uint162bytes(const uint16_t &data) {
	PoolByteArray res;
	res.resize(2);
	auto w = res.write();
	encode_uint16(data, w.ptr());
	return res;
}

static PoolByteArray uint322bytes(const uint32_t &data) {
	PoolByteArray res;
	res.resize(4);
	auto w = res.write();
	encode_uint32(data, w.ptr());
	return res;
}

static PoolByteArray uint642bytes(const uint64_t &data) {
	PoolByteArray res;
	res.resize(8);
	auto w = res.write();
	encode_uint64(data, w.ptr());
	return res;
}

static PoolByteArray float2bytes(const float &data) {
	PoolByteArray res;
	res.resize(4);
	auto w = res.write();
	encode_float(data, w.ptr());
	return res;
}

static PoolByteArray double2bytes(const double &data) {
	PoolByteArray res;
	res.resize(8);
	auto w = res.write();
	encode_double(data, w.ptr());
	return res;
}

static int16_t bytes2int16(const PoolByteArray &data, int idx = 0) {
	auto r = data.read();
	return (int16_t)decode_uint16(r.ptr() + idx);
}

static int32_t bytes2int32(const PoolByteArray &data, int idx = 0) {
	auto r = data.read();
	return (int32_t)decode_uint32(r.ptr() + idx);
}

static int64_t bytes2int64(const PoolByteArray &data, int idx = 0) {
	auto r = data.read();
	return (int64_t)decode_uint64(r.ptr() + idx);
}

static uint16_t bytes2uint16(const PoolByteArray &data, int idx = 0) {
	auto r = data.read();
	return (int16_t)decode_uint16(r.ptr() + idx);
}

static uint32_t bytes2uint32(const PoolByteArray &data, int idx = 0) {
	auto r = data.read();
	return (int32_t)decode_uint32(r.ptr() + idx);
}

static uint64_t bytes2uint64(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return (int64_t)decode_uint64(r.ptr() + idx);
}

static float bytes2float(const PoolByteArray &data, int idx = 0) {
	auto r = data.read();
	return decode_float(r.ptr() + idx);
}

static double bytes2double(const PoolByteArray &data, int idx = 0) {
	auto r = data.read();
	return decode_double(r.ptr() + idx);
}

static int16_t bytes2int16(const uint8_t *data) {
	return (int16_t)decode_uint16(data);
}

static int32_t bytes2int32(const uint8_t *data) {
	return (int32_t)decode_uint32(data);
}

static int64_t bytes2int64(const uint8_t *data) {
	return (int64_t)decode_uint64(data);
}

static uint16_t bytes2uint16(const uint8_t *data) {
	return (int16_t)decode_uint16(data);
}

static uint32_t bytes2uint32(const uint8_t *data) {
	return (int32_t)decode_uint32(data);
}

static uint64_t bytes2uint64(const uint8_t *data) {
	return (int64_t)decode_uint64(data);
}

static float bytes2float(const uint8_t *data) {
	return decode_float(data);
}

static double bytes2double(const uint8_t *data) {
	return decode_double(data);
}

static bool validate_packet(const uint8_t *data) {
	if (data[0] == internal_PACKET_HEADER[0] && data[1] == internal_PACKET_HEADER[1] && data[2] == internal_PACKET_HEADER[2] && data[3] == internal_PACKET_HEADER[3])
		return true;
	return false;
}

static bool validate_version(const uint8_t *data) {
	if (data[0] == internal_VERSION[0] && data[1] == internal_VERSION[1])
		return true;
	return false;
}

static PoolByteArray get_packet_header() {
	return internal_PACKET_HEADER;
}

static PoolByteArray get_version() {
	return internal_VERSION;
}

static void set_log_level(LogLevel lvl) {
	current_loglevel = lvl;
}

static void init() {
	if (internal_PACKET_HEADER.empty()) {
		internal_PACKET_HEADER.append(47);
		internal_PACKET_HEADER.append(52);
		internal_PACKET_HEADER.append(48);
		internal_PACKET_HEADER.append(44);
	}
	if (internal_VERSION.empty()) {
		internal_VERSION.append(1);
		internal_VERSION.append(0);
		internal_VERSION.append(0);
	}

	GET_PS_SET(compress_buffer_size_mb, "debug/godot_remote/server/jpg_compress_buffer_size_mbytes");
	compress_buffer.resize((1024 * 1024) * compress_buffer_size_mb);
}

static void deinit() {
	internal_PACKET_HEADER.resize(0);
	internal_VERSION.resize(0);
	compress_buffer.resize(0);
}

}; // namespace GRUtils

VARIANT_ENUM_CAST(GRUtils::Subsampling)
VARIANT_ENUM_CAST(GRUtils::LogLevel)
#endif // !GRUTILS_H
