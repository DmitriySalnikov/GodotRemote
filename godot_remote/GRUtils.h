/* GRUtils.h */
#ifndef GRUTILS_H
#define GRUTILS_H

#include "core/image.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "core/print_string.h"
#include "core/project_settings.h"
#include "core/variant.h"
#include "main/input_default.h"

#ifdef DEBUG_ENABLED

#define TimeCountInit() int simple_time_counter = OS::get_singleton()->get_ticks_usec()
#define TimeCountReset() simple_time_counter = OS::get_singleton()->get_ticks_usec()
// Shows delta between this and previous counter. Need to call TimeCountInit before
#define TimeCount(str)                                                                                                                                                 \
	GRUtils::_log(str + String(": ") + String::num_real((OS::get_singleton()->get_ticks_usec() - simple_time_counter) / 1000.0) + " ms", GRUtils::LogLevel::LL_Debug); \
	simple_time_counter = OS::get_singleton()->get_ticks_usec()

// Bind constant with custom name
#define BIND_ENUM_CONSTANT_CUSTOM(m_constant, m_name) \
	ClassDB::bind_integer_constant(get_class_static(), __constant_get_enum_name(((int)(m_constant)), m_name), m_name, ((int)(m_constant)));

#else

#define TimeCountInit()
#define TimeCountReset()
#define TimeCount(str)

// Bind constant with custom name
#define BIND_ENUM_CONSTANT_CUSTOM(m_constant, m_name) \
	ClassDB::bind_integer_constant(get_class_static(), StringName(), m_name, ((int)(m_constant)));

#endif // DEBUG_ENABLED

#define GR_VERSION(x, y, z)         \
	if (internal_VERSION.empty()) { \
		internal_VERSION.append(x); \
		internal_VERSION.append(y); \
		internal_VERSION.append(z); \
	}

#define GR_PACKET_HEADER(a, b, c, d)      \
	if (internal_PACKET_HEADER.empty()) { \
		internal_PACKET_HEADER.append(a); \
		internal_PACKET_HEADER.append(b); \
		internal_PACKET_HEADER.append(c); \
		internal_PACKET_HEADER.append(d); \
	}

#define CON_ADDRESS(con) str(con->get_connected_host()) + ":" + str(con->get_connected_port())

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

// DEFINES

extern int current_loglevel;
extern PoolByteArray internal_PACKET_HEADER;
extern PoolByteArray internal_VERSION;

extern void init();
extern void deinit();

#ifndef NO_GODOTREMOTE_SERVER
extern void init_server_utils();
extern void deinit_server_utils();
extern PoolByteArray compress_buffer;
extern int compress_buffer_size_mb;
extern PoolByteArray compress_jpg(PoolByteArray &img_data, int width, int height, int bytes_for_color = 4, int quality = 75, int subsampling = Subsampling::SUBSAMPLING_H2V2);
#endif

extern void _log(const Variant &val, LogLevel lvl = LogLevel::LL_Normal);
extern String str(const Variant &val);

extern String str_arr(const Array arr, const bool force_full = false, const int max_shown_items = 32);
extern String str_arr(const Dictionary arr, const bool force_full = false, const int max_shown_items = 32);
extern String str_arr(const uint8_t *data, const int size, const bool force_full = false, const int max_shown_items = 64);

extern PoolByteArray var2bytes(const Variant &data, bool full_objects = false);

extern PoolByteArray int162bytes(const int16_t &data);
extern PoolByteArray int322bytes(const int32_t &data);
extern PoolByteArray int642bytes(const int64_t &data);
extern PoolByteArray uint162bytes(const uint16_t &data);
extern PoolByteArray uint322bytes(const uint32_t &data);
extern PoolByteArray uint642bytes(const uint64_t &data);
extern PoolByteArray float2bytes(const float &data);
extern PoolByteArray double2bytes(const double &data);

extern int16_t bytes2int16(const PoolByteArray &data, int idx = 0);
extern int32_t bytes2int32(const PoolByteArray &data, int idx = 0);
extern int64_t bytes2int64(const PoolByteArray &data, int idx = 0);
extern uint16_t bytes2uint16(const PoolByteArray &data, int idx = 0);
extern uint32_t bytes2uint32(const PoolByteArray &data, int idx = 0);
extern uint64_t bytes2uint64(const PoolByteArray &data, int idx);
extern float bytes2float(const PoolByteArray &data, int idx = 0);
extern double bytes2double(const PoolByteArray &data, int idx = 0);

extern int16_t bytes2int16(const uint8_t *data);
extern int32_t bytes2int32(const uint8_t *data);
extern int64_t bytes2int64(const uint8_t *data);
extern uint16_t bytes2uint16(const uint8_t *data);
extern uint32_t bytes2uint32(const uint8_t *data);
extern uint64_t bytes2uint64(const uint8_t *data);
extern float bytes2float(const uint8_t *data);
extern double bytes2double(const uint8_t *data);

extern bool validate_packet(const uint8_t *data);
extern bool validate_version(const PoolByteArray &data);
extern bool validate_version(const uint8_t *data);

extern bool compare_pool_byte_arrays(const PoolByteArray &a, const PoolByteArray &b);

extern void set_gravity(const Vector3 &p_gravity);
extern void set_accelerometer(const Vector3 &p_accel);
extern void set_magnetometer(const Vector3 &p_magnetometer);
extern void set_gyroscope(const Vector3 &p_gyroscope);

// LITERALS

// conversion from usec to msec. most useful to OS::delay_usec()
constexpr uint32_t operator"" _ms(unsigned long long val) {
	return val * 1000;
}

// IMPLEMENTATINS

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

static void log_array(const uint8_t *data, const int size, const bool force_full = false, const int max_shown_items = 64, LogLevel lvl = LogLevel::LL_Normal) {
	_log(str_arr(data, size, force_full, max_shown_items), lvl);
}
static void log_array(const Array data, const bool force_full = false, const int max_shown_items = 64, LogLevel lvl = LogLevel::LL_Normal) {
	_log(str_arr(data, force_full, max_shown_items), lvl);
}
static void log_array(const Dictionary data, const bool force_full = false, const int max_shown_items = 32, LogLevel lvl = LogLevel::LL_Normal) {
	_log(str_arr(data, force_full, max_shown_items), lvl);
}
template <class T>
static void log_array(const PoolVector<T> data, const bool force_full = false, const int max_shown_items = 64, LogLevel lvl = LogLevel::LL_Normal) {
	_log(str_arr(data, force_full, max_shown_items), lvl);
}

template <class T = Variant>
static T bytes2var(const PoolByteArray &data, bool allow_objects = false) {
	Variant ret;
	PoolByteArray::Read r = data.read();
	Error err = decode_variant(ret, r.ptr(), data.size(), NULL, allow_objects);
	if (err)
		return Variant();
	return ret;
}

template <class T = Variant>
static T bytes2var(const uint8_t *data, int size, bool allow_objects = false) {
	Variant ret;
	Error err = decode_variant(ret, data, size, NULL, allow_objects);
	if (err)
		return Variant();
	return ret;
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

}; // namespace GRUtils

VARIANT_ENUM_CAST(GRUtils::Subsampling)
VARIANT_ENUM_CAST(GRUtils::LogLevel)
#endif // !GRUTILS_H
