/* GRUtils.h */
#ifndef GRUTILS_H
#define GRUTILS_H

#include <vector>

#ifndef GDNATIVE_LIBRARY
#include "core/image.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "core/print_string.h"
#include "core/project_settings.h"
#include "core/variant.h"
#include "main/input_default.h"

#else

#include <Image.hpp>
#include <Marshalls.hpp>
#include <OS.hpp>
#include <Engine.hpp>
#include <String.hpp>
#include <Dictionary.hpp>
#include <Array.hpp>
#include <ProjectSettings.hpp>
#include <Variant.hpp>
#include <InputDefault.hpp>
#include <File.hpp>
using namespace godot;
#endif

#define vec_remove(vec, idx) vec.erase(vec.begin() + idx);

#ifndef GDNATIVE_LIBRARY
#define ST() SceneTree::get_singleton()
#define GD_CLASS(c, p) GDCLASS(c, p)
#define GD_S_CLASS(c, p) GDCLASS(c, p)

#define queue_del queue_delete
#define img_is_empty(img) img->empty()
#define is_valid_ip is_valid

#define dict_get_key_at_index(dict, i) dict.get_key_at_index(i)
#define dict_get_value_at_index(dict, i) dict.get_value_at_index(i)

#define vec_find(vec, e, start_idx) std::find(vec.begin() + start_idx, vec.end(), e)
#define vec_find_idx(vec, e, start_idx) (std::distance(vec.begin(), vec_find(vec, e, start_idx)))
#define vec_find_exists(vec, e, start_idx) (vec_find(vec, e, start_idx) != vec.end())

#define file_get_as_string(path, err) FileAccess::get_file_as_string(path, err);

#define THREAD_FUNC static
#define t_wait_to_finish(thread) t_wait_to_finish(thread)
#define _TS_CLASS_	_THREAD_SAFE_CLASS_
#define _TS_METHOD_	_THREAD_SAFE_METHOD_
#define _TS_LOCK_	_THREAD_SAFE_LOCK_
#define _TS_UNLOCK_	_THREAD_SAFE_UNLOCK_
#define Mutex_create() Mutex::create()
#define Thread_create(_class, function, data_to_send, inst) Thread::create(&_class::function, data_to_send)
#define Thread_set_name(_name) Thread::set_name()

#else

enum Margin {
	MARGIN_LEFT,
	MARGIN_TOP,
	MARGIN_RIGHT,
	MARGIN_BOTTOM
};

// THREAD SAFE classes
// ORIGINAL CODE FROM GODOT CORE
// because GDNative don't has it

#include <Mutex.hpp>
#include <Thread.hpp>

class ThreadSafe {

	Mutex* mutex;

public:
	inline void lock() const {
		if (mutex) mutex->lock();
	}
	inline void unlock() const {
		if (mutex) mutex->unlock();
	}

	ThreadSafe();
	~ThreadSafe();
};

class ThreadSafeMethod {

	const ThreadSafe* _ts;

public:
	ThreadSafeMethod(const ThreadSafe* p_ts) {

		_ts = p_ts;
		_ts->lock();
	}

	~ThreadSafeMethod() { _ts->unlock(); }
};

#define THREAD_FUNC
#define t_wait_to_finish(thread) thread->wait_to_finish()
#define _TS_CLASS_ ThreadSafe __thread__safe__
#define _TS_METHOD_ ThreadSafeMethod __thread_safe_method__(&__thread__safe__)
#define _TS_LOCK_ __thread__safe__.lock()
#define _TS_UNLOCK_ __thread__safe__.unlock()
#define Mutex_create() Mutex::_new()
#define Thread_create(_class, function, data_to_send, inst) _gdn_thread_create(inst, #function, data_to_send)
#define Thread_set_name(_name)

// THREAD SAFE END


#define ST() ((SceneTree*)Engine::get_singleton()->get_main_loop())
#define GD_CLASS(c, p) GODOT_CLASS(c, p)
#define GD_S_CLASS(c, p) GODOT_SUBCLASS(c, p)

#define queue_del queue_free
#define is_valid_ip is_valid_ip_address
#define img_is_empty(img) img->is_empty()

#define dict_get_key_at_index(dict, i) dict.keys()[i]
#define dict_get_value_at_index(dict, i) dict.values()[i]

#define vec_find(vec, e, start_idx) std::find(vec.begin() + start_idx, vec.end(), e)
#define vec_find_idx(vec, e, start_idx) (std::distance(vec.begin(), vec_find(vec, e, start_idx)))
#define vec_find_exists(vec, e, start_idx) (vec_find(vec, e, start_idx) != vec.end())

#define file_get_as_string(path, err) _gdn_get_file_as_string(path, err)

#define memnew(obj) new obj
#define memdelete(obj) delete obj

#define ERR_FAIL_V_MSG(m_retval, m_msg)                                              \
	{                                                                                \
		ERR_PRINT(String("Method failed. Returning: ") + #m_retval + ".\n" + m_msg); \
		return m_retval;                                                             \
	}
#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                            \
	{                                                                                                           \
		if(!((int)m_cond)){                                                                                          \
			ERR_PRINT(String("Condition \"") + #m_cond + "\" is true. Returned: " + #m_retval + ".\n" + m_msg); \
			return m_retval;                                                                                    \
		}                                                                                                       \
	}
#define ERR_FAIL_MSG(m_msg)                    \
	{                                          \
		ERR_PRINT(String("Method failed.\n") + m_msg); \
		return;                                \
	}

#endif

#ifdef DEBUG_ENABLED

#define TimeCountInit() int simple_time_counter = OS::get_singleton()->get_ticks_usec()
#define TimeCountReset() simple_time_counter = OS::get_singleton()->get_ticks_usec()
// Shows delta between this and previous counter. Need to call TimeCountInit before
#define TimeCount(str)                                                                                                                                      \
	GRUtils::_log(str + String(": ") + String::num((OS::get_singleton()->get_ticks_usec() - simple_time_counter) / 1000.0, 3) + " ms", LogLevel::LL_Debug); \
	simple_time_counter = OS::get_singleton()->get_ticks_usec()

// Bind constant with custom name
#define BIND_ENUM_CONSTANT_CUSTOM(m_constant, m_name) \
	ClassDB::bind_integer_constant(get_class_static(), __constant_get_enum_name(m_constant, m_name), m_name, ((int)(m_constant)));

#else

#define TimeCountInit()
#define TimeCountReset()
#define TimeCount(str)

// Bind constant with custom name
#define BIND_ENUM_CONSTANT_CUSTOM(m_constant, m_name) \
	ClassDB::bind_integer_constant(get_class_static(), StringName(), m_name, ((int)(m_constant)));

#endif // DEBUG_ENABLED

#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)

#define GR_VERSION(x, y, z)             \
	if (internal_VERSION.size() == 0) { \
		internal_VERSION.append(x);     \
		internal_VERSION.append(y);     \
		internal_VERSION.append(z);     \
	}

#define GR_PACKET_HEADER(a, b, c, d)          \
	if (internal_PACKET_HEADER.size() == 0) { \
		internal_PACKET_HEADER.append(a);     \
		internal_PACKET_HEADER.append(b);     \
		internal_PACKET_HEADER.append(c);     \
		internal_PACKET_HEADER.append(d);     \
	}

#define CON_ADDRESS(con) str(con->get_connected_host()) + ":" + str(con->get_connected_port())

// Get Project Setting
#define GET_PS(setting_name) \
	ProjectSettings::get_singleton()->get_setting(setting_name)
// Get Project Setting and set it to variable
#define GET_PS_SET(variable_to_store, setting_name) \
	variable_to_store = ProjectSettings::get_singleton()->get_setting(setting_name)

enum LogLevel {
	LL_Debug = 0,
	LL_Normal = 1,
	LL_Warning = 2,
	LL_Error = 3,
	LL_None,
};

enum Subsampling {
	SUBSAMPLING_Y_ONLY = 0,
	SUBSAMPLING_H1V1 = 1,
	SUBSAMPLING_H2V1 = 2,
	SUBSAMPLING_H2V2 = 3
};

enum ImageCompressionType {
	Uncompressed = 0,
	JPG = 1,
	PNG = 2,
};

enum TypesOfServerSettings {
	USE_INTERNAL_SERVER_SETTINGS = 0,
	VIDEO_STREAM_ENABLED = 1,
	COMPRESSION_TYPE = 2,
	JPG_QUALITY = 3,
	SKIP_FRAMES = 4,
	RENDER_SCALE = 5,
};

namespace GRUtils {
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
	extern Error compress_jpg(PoolByteArray& ret, const PoolByteArray& img_data, int width, int height, int bytes_for_color = 4, int quality = 75, int subsampling = Subsampling::SUBSAMPLING_H2V2);
#endif

	extern Error compress_bytes(const PoolByteArray& bytes, PoolByteArray& res, int type);
	extern Error decompress_bytes(const PoolByteArray& bytes, int output_size, PoolByteArray& res, int type);
	extern void _log(const Variant& val, LogLevel lvl = LogLevel::LL_Normal);

	extern String str(const Variant& val);
	extern String str_arr(const Array arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ");
	extern String str_arr(const Dictionary arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ");
	extern String str_arr(const uint8_t* data, const int size, const bool force_full = false, const int max_shown_items = 64, String separator = ", ");

	extern bool validate_packet(const uint8_t* data);
	extern bool validate_version(const PoolByteArray& data);
	extern bool validate_version(const uint8_t* data);

	extern bool compare_pool_byte_arrays(const PoolByteArray& a, const PoolByteArray& b);

	extern void set_gravity(const Vector3& p_gravity);
	extern void set_accelerometer(const Vector3& p_accel);
	extern void set_magnetometer(const Vector3& p_magnetometer);
	extern void set_gyroscope(const Vector3& p_gyroscope);

	// LITERALS

	// conversion from usec to msec. most useful to OS::delay_usec()
	constexpr uint32_t operator"" _ms(unsigned long long val) {
		return val * 1000;
	}

	// IMPLEMENTATINS

#ifndef GDNATIVE_LIBRARY
	template <class T>
	static String str_arr(PoolVector<T> arr, const bool force_full = false, const int max_shown_items = 64, String separator = ", ") {
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
				res += separator;
			}
		}
		r.release();

		if (is_long) {
			res += str(int64_t(arr.size()) - s) + " more items...";
		}

		return res + " ]";
	};
#else

#define POOLARRAYS_STR_ARR(TYPE)                                                                                              \
	static String str_arr(TYPE arr, const bool force_full, const int max_shown_items, String separator) {                            \
		String res = "[ ";                                                                                                    \
		int s = arr.size();                                                                                                   \
		bool is_long = false;                                                                                                 \
		if (s > max_shown_items && !force_full) {                                                                             \
			s = max_shown_items;                                                                                              \
			is_long = true;                                                                                                   \
		}                                                                                                                     \
                                                                                                                              \
		auto r = arr.read();                                                                                                  \
		for (int i = 0; i < s; i++) {                                                                                         \
			res += str(r[i]);                                                                                                 \
			if (i != s - 1 || is_long) {                                                                                      \
				res += separator;                                                                                             \
			}                                                                                                                 \
		}                                                                                                                     \
                                                                                                                              \
		if (is_long) {                                                                                                        \
			res += str(int64_t(arr.size()) - s) + " more items...";                                                           \
		}                                                                                                                     \
                                                                                                                              \
		return res + " ]";                                                                                                    \
	}

	POOLARRAYS_STR_ARR(PoolByteArray);
	POOLARRAYS_STR_ARR(PoolIntArray);
	POOLARRAYS_STR_ARR(PoolRealArray);
	POOLARRAYS_STR_ARR(PoolStringArray);
	POOLARRAYS_STR_ARR(PoolVector2Array);
	POOLARRAYS_STR_ARR(PoolVector3Array);
	POOLARRAYS_STR_ARR(PoolColorArray);

#undef POOLARRAYS_STR_ARR
#endif
	
	template <typename T>
	static String str_arr(std::vector<T> const &arr, const bool force_full = false, const int max_shown_items = 64, String separator = ", ") {
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
				res += separator;
			}
		}

		if (is_long) {
			res += String::num_int64(int64_t(arr.size()) - s) + " more items...";
		}

		return res + " ]";
	};

	static PoolByteArray get_packet_header() {
		return internal_PACKET_HEADER;
	}

	static PoolByteArray get_gr_version() {
		return internal_VERSION;
	}

	static void set_log_level(LogLevel lvl) {
		current_loglevel = lvl;
	}


#ifndef GDNATIVE_LIBRARY

#else
	static String _gdn_get_file_as_string(String path, Error *ret_err) {
		auto f = File::_new();
		Error r = f->open(path, File::ModeFlags::READ);
		*ret_err = r;
		if (r == Error::OK) {
			String txt = f->get_as_text();
			f->close();
			f->free();
			return txt;
		}
		f->free();
		return "";
	}

	static Thread* _gdn_thread_create(Object* instance, String func_name, Variant user_data) {
		Thread* t = Thread::_new();
		t->start(instance, "", user_data);
		return t;
	}

#endif

}; // namespace GRUtils

#ifndef GDNATIVE_LIBRARY
VARIANT_ENUM_CAST(Subsampling)
VARIANT_ENUM_CAST(LogLevel)
VARIANT_ENUM_CAST(ImageCompressionType)
VARIANT_ENUM_CAST(TypesOfServerSettings)
#endif

#endif // !GRUTILS_H
