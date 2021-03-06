/* GRUtils.h */
#pragma once

#include "GRObjectPool.h"
#include "GRProfiler.h"
#include <algorithm>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#ifndef GDNATIVE_LIBRARY
#include "core/bind/core_bind.h"
#include "core/image.h"
#include "core/io/marshalls.h"
#include "core/io/stream_peer.h"
#include "core/os/os.h"
#include "core/print_string.h"
#include "core/project_settings.h"
#include "core/variant.h"
#include "core/version_generated.gen.h"
#include "main/input_default.h"
#include "scene/2d/canvas_item.h"
//#define VERSION_MINOR 3

#else

#include <Array.hpp>
#include <CanvasItem.hpp>
#include <Dictionary.hpp>
#include <Directory.hpp>
#include <Engine.hpp>
#include <File.hpp>
#include <Godot.hpp>
#include <Image.hpp>
#include <InputDefault.hpp>
#include <Marshalls.hpp>
#include <Mutex.hpp>
#include <OS.hpp>
#include <Object.hpp>
#include <PoolArrays.hpp>
#include <ProjectSettings.hpp>
#include <StreamPeerBuffer.hpp>
#include <String.hpp>
#include <Thread.hpp>
#include <Variant.hpp>
using namespace godot;
#endif

// =================================================================
// CONDITIONAL DEFINES

#ifndef GDNATIVE_LIBRARY
#define VARIANT_OBJ_CAST_TO(var, to) ((to *)(Object *)var)

#define ENUM_ARG(en) en
#define ENUM_CONV(en) (en)

#define DEF_ARG(a) a
#define DEF_ARGS(a)

#define ST() SceneTree::get_singleton()
#define GD_CLASS(c, p) GDCLASS(c, p)
#define V_CAST(var, type) ((type)var)

// Bind constant with custom name
//ClassDB::bind_integer_constant(get_class_static(), __constant_get_enum_name(m_constant, #m_constant), #m_constant, m_constant);
#define BIND_ENUM_CONSTANT_CUSTOM(m_enum, m_const, m_name) ClassDB::bind_integer_constant(get_class_static(), #m_enum, m_name, ((int)(m_enum::m_const)));

#define queue_del queue_delete
#define img_is_empty(img) img->empty()
#define img_create_from_data(img, __width, __height, __mipmaps, __format, __data) img->create(__width, __height, __mipmaps, __format, __data);
#define is_valid_ip is_valid
#define release_pva_read(pva) pva.release()
#define release_pva_write(pva) pva.release()
#define store_data_to_file(p, size) p, size
#define put_data_from_array_pointer(buf) buf.read().ptr(), buf.size()
#define get_data_from_stream(_stream, pDst, size) _stream->get_data(pDst, size)

#define dict_get_key_at_index(dict, i) dict.get_key_at_index(i)
#define dict_get_value_at_index(dict, i) dict.get_value_at_index(i)

#define file_get_as_string(path, err) FileAccess::get_file_as_string(path, err);
#define file_open(var, path, flags) \
	var = FileAccess::open(path, flags);

#else

enum Margin : int {
	MARGIN_LEFT,
	MARGIN_TOP,
	MARGIN_RIGHT,
	MARGIN_BOTTOM
};

typedef Thread _Thread;
typedef File _File;
typedef Directory _Directory;
// THREAD SAFE END

#define VARIANT_OBJ_CAST_TO(var, to) ((to *)var)

#define ENUM_ARG(en) int
#define ENUM_CONV(en)

#define DEF_ARG(a)
//#define DEF_ARGS(...) ,__VA_ARGS__

#define CONST_FAKE_SET() \
	void set_constant(int val) {}
#define CONST_FAKE_REG(cl) register_method("set_constant", &cl::set_constant)
#define METHOD_REG(cl, fn) register_method(#fn, &cl::fn)

#define CONST_FUNC_CAT(cl, en, val) _get_##cl##_##en##_##val
#define CONST_FUNC_CAT_S(cl, en, val) "_get_" #cl "_" #en "_" #val
#define CONST_GET(cl, en, val) \
	int CONST_FUNC_CAT(cl, en, val)() { return (int)cl::en::val; }

#define CONST_REG(cl, en, val)                                                                 \
	register_method(CONST_FUNC_CAT_S(cl, en, val), &GodotRemote::CONST_FUNC_CAT(cl, en, val)); \
	register_property<GodotRemote, int>(#cl "_" #val, &GodotRemote::set_constant, &GodotRemote::CONST_FUNC_CAT(cl, en, val), cl::en::val)

#define ST() ((SceneTree *)Engine::get_singleton()->get_main_loop())
#define GD_CLASS(c, p) GODOT_CLASS(c, p)
#define V_CAST(var, type) (var.operator type())
#define GLOBAL_DEF(m_var, m_value) _GLOBAL_DEF(m_var, m_value)
#define GLOBAL_GET(m_var) ProjectSettings::get_singleton()->get(m_var)

#define queue_del queue_free
#define is_valid_ip is_valid_ip_address
#define img_is_empty(img) img->is_empty()
#define img_create_from_data(img, __width, __height, __mipmaps, __format, __data) img->create_from_data(__width, __height, __mipmaps, __format, __data);
#define release_pva_read(pva)
#define release_pva_write(pva)
#define store_data_to_file(p, size) _gdn_convert_native_pointer_array_to_pba(p, size)
#define put_data_from_array_pointer(p) p
#define get_data_from_stream(_stream, pDst, size) _gdn_convert_array_to_native_pointer_array(pDst, _stream->get_data(size), size)

#define dict_get_key_at_index(dict, i) _gdn_dictionary_get_key_at_index(dict, i)
#define dict_get_value_at_index(dict, i) _gdn_dictionary_get_value_at_index(dict, i)

#define file_get_as_string(path, err) _gdn_get_file_as_string(path, err)
#define file_open(var, path, flags) \
	var = memnew(File);             \
	var->open(path, flags);

#define memnew(obj) obj::_new()
#define memdelete(obj) obj->free()

#define ERR_FAIL_V_MSG(m_retval, m_msg)                                              \
	{                                                                                \
		ERR_PRINT(String("Method failed. Returning: ") + #m_retval + ".\n" + m_msg); \
		return m_retval;                                                             \
	}
#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                                            \
	{                                                                                                           \
		if (((int)m_cond)) {                                                                                    \
			ERR_PRINT(String("Condition \"") + #m_cond + "\" is true. Returned: " + #m_retval + ".\n" + m_msg); \
			return m_retval;                                                                                    \
		}                                                                                                       \
	}
#define ERR_FAIL_MSG(m_msg)                            \
	{                                                  \
		ERR_PRINT(String("Method failed.\n") + m_msg); \
		return;                                        \
	}

#endif

// =================================================================
// DEBUG DEFINES

#ifdef DEBUG_ENABLED
#define _log(val, logLevel) __log(val, logLevel, __FUNCTION__, __FILE__, __LINE__)
#else
#define _log(val, logLevel)
#endif // DEBUG_ENABLED

#if defined(TRACY_ENABLE) && defined(GODOT_REMOTE_TRACY_ENABLED)
#define Thread_set_name(_name) tracy::SetThreadName(_name)

#if defined(TRACY_DELAYED_INIT) && defined(TRACY_MANUAL_LIFETIME)
#define MANUAL_TRACY
#define START_TRACY tracy::StartupProfiler();
#define STOP_TRACY tracy::ShutdownProfiler();
#else
#define START_TRACY
#define STOP_TRACY
#endif

#else
#define Thread_set_name(_name)

#define START_TRACY
#define STOP_TRACY
#endif

// =================================================================
// GLOBAL DEFINES

#ifndef NO_SAFE_CAST
#define shared_ptr_cast_type dynamic_pointer_cast
#else
#define shared_ptr_cast_type static_pointer_cast
#endif

#define sleep_usec(usec) OS::get_singleton()->delay_usec(usec)
#define rnd_rng(_min, _max) (_min + (abs(rand64()) % (_max - _min + 1)))
#define Mutex_define(_var, _description) TracyLockableN(std::recursive_mutex, _var, _description)
#define Scoped_lock(_mutex_name) std::lock_guard<LockableBase(std::recursive_mutex)> _scoped_lock_guard(_mutex_name)

#define LEAVE_IF_EDITOR()                          \
	if (Engine::get_singleton()->is_editor_hint()) \
		return;

#define t_wait_to_finish(thread) thread->wait_to_finish()
#define Thread_start(_var, inst, function, data_to_send) Thread_start_string(_var, inst, #function, data_to_send)
#define Thread_start_string(_var, inst, function_string, data_to_send) _var = utils_thread_create(inst, function_string, data_to_send)
#define Thread_close(_name)                       \
	if (_name.is_valid() && _name->is_active()) { \
		t_wait_to_finish(_name);                  \
		_name.unref();                            \
		_name = Ref<_Thread>();                   \
	}

#define shared_cast(type, from) std::shared_ptr_cast_type<type>(from)
#define shared_cast_var(type, var, from) var = std::shared_ptr_cast_type<type>(from)
#define shared_cast_def(type, var, from) std::shared_ptr<type> var = std::shared_ptr_cast_type<type>(from)
#define shared_new(type, ...) std::make_shared<type>(__VA_ARGS__)

#define newref(_class) Ref<_class>(memnew(_class))
#define newref_std(_class) std::shared_ptr<_class>(memnew(_class), [](_class *o) { memdelete(o); })

#define RefStd(_class) std::shared_ptr<_class>

#define GR_VERSION(x, y, z)                            \
	if (_grutils_data->internal_VERSION.size() == 0) { \
		_grutils_data->internal_VERSION.append(x);     \
		_grutils_data->internal_VERSION.append(y);     \
		_grutils_data->internal_VERSION.append(z);     \
	}

#define GR_PACKET_HEADER(a, b, c, d)                         \
	if (_grutils_data->internal_PACKET_HEADER.size() == 0) { \
		_grutils_data->internal_PACKET_HEADER.append(a);     \
		_grutils_data->internal_PACKET_HEADER.append(b);     \
		_grutils_data->internal_PACKET_HEADER.append(c);     \
		_grutils_data->internal_PACKET_HEADER.append(d);     \
	}

#define CONNECTION_ADDRESS(con) str(con->get_connected_host()) + ":" + str(con->get_connected_port())
#define NAMEOF(var) #var

// Get Project Setting
#define GET_PS(setting_name) \
	ProjectSettings::get_singleton()->get_setting(setting_name)
// Get Project Setting and set it to variable
#define GET_PS_SET(variable_to_store, setting_name) \
	variable_to_store = ProjectSettings::get_singleton()->get_setting(setting_name)

#define PORT_AUTO_CONNECTION 22765
#define PORT_STATIC_CONNECTION 22766
#define MB_SIZE 1000000
#define MiB_SIZE 1048576

enum LogLevel : int {
	LL_DEBUG = 0,
	LL_NORMAL = 1,
	LL_WARNING = 2,
	LL_ERROR = 3,
	LL_NONE,
};

namespace GRUtils {
// LITERALS

// conversion from usec to msec. most useful to OS::delay_usec()
constexpr uint32_t operator"" _ms(unsigned long long val) {
	return (int)val * 1000;
}

// CLASSES

class GRUtilsData {
public:
	int current_loglevel;
	PoolByteArray internal_PACKET_HEADER;
	PoolByteArray internal_VERSION;
	std::shared_ptr<GRObjectPool<StreamPeerBuffer> > streamPeerBufferPool;
};

extern std::shared_ptr<GRUtilsData> _grutils_data;

extern void init();
extern void deinit();

extern PoolByteArray get_packet_header();
extern PoolByteArray get_gr_version();
extern void set_log_level(int lvl);

extern uint64_t get_time_usec();

extern Error compress_bytes(const PoolByteArray &bytes, PoolByteArray &res, int type);
extern Error decompress_bytes(const PoolByteArray &bytes, int output_size, PoolByteArray &res, int type);
#ifdef DEBUG_ENABLED
extern void __log(const Variant &val, int lvl = 1 /*LogLevel::LL_NORMAL*/, String func = "", String file = "", int line = 0);
#endif

extern Rect2 get_2d_safe_area(class CanvasItem *ci);

extern String str(const Variant &val);
extern String str_arr(const Array arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ", bool add_braces = true);
extern String str_arr(const Dictionary arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ", bool add_braces = true);
extern String str_arr(const uint8_t *arr, const int size, const bool force_full = false, const int max_shown_items = 64, String separator = ", ", bool add_braces = true);

extern std::shared_ptr<GRObjectPool<StreamPeerBuffer> > get_stream_peer_buffer_pool();
extern bool validate_packet(const uint8_t *data);
extern bool validate_version(const PoolByteArray &data);
extern bool validate_version(const uint8_t *data);

extern bool compare_pool_byte_arrays(const PoolByteArray &a, const PoolByteArray &b);
extern bool compare_pool_string_arrays(const PoolStringArray &a, const PoolStringArray &b);
extern bool compare_pool_string_arrays(const std::vector<String> &a, const PoolStringArray &b);

extern void set_gravity(const Vector3 &p_gravity);
extern void set_accelerometer(const Vector3 &p_accel);
extern void set_magnetometer(const Vector3 &p_magnetometer);
extern void set_gyroscope(const Vector3 &p_gyroscope);

extern Ref<_Thread> utils_thread_create(Object *instance, String func_name, const Variant &user_data);

// IMPLEMENTATINS

#define DEFAULT_STR_ARR_BODY                            \
	String res = add_braces ? "[ " : "";                \
	bool is_long = false;                               \
	if (s > max_shown_items && !force_full) {           \
		s = max_shown_items;                            \
		is_long = true;                                 \
	}                                                   \
                                                        \
	for (int i = 0; i < s; i++) {                       \
		res += str(arr[i]);                             \
		if (i != s - 1 || is_long) {                    \
			res += separator;                           \
		}                                               \
	}                                                   \
                                                        \
	if (is_long) {                                      \
		res += str(int64_t(ss) - s) + " more items..."; \
	}                                                   \
                                                        \
	if (add_braces) {                                   \
		return res + " ]";                              \
	} else {                                            \
		return res;                                     \
	}

template <class T>
extern String str_arr(const std::vector<T> arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ", bool add_braces = true) {
	int s = (int)arr.size(), ss = (int)arr.size();
	DEFAULT_STR_ARR_BODY;
}

template <class K, class V>
extern String str_arr(const std::map<K, V> arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ", bool add_braces = true) {
	String res = add_braces ? "{ " : "";
	int s = (int)arr.size();
	bool is_long = false;
	if (s > max_shown_items && !force_full) {
		s = max_shown_items;
		is_long = true;
	}

	int i = 0;
	for (auto p : arr) {
		if (i++ >= s)
			break;
		res += str(p.first) + " : " + str(p.second);
		if (i != s - 1 || is_long) {
			res += separator;
		}
	}

	if (is_long) {
		res += String::num_int64(int64_t(arr.size()) - s) + " more items...";
	}

	if (add_braces) {
		return res + " }";
	} else {
		return res;
	}
}

#ifndef GDNATIVE_LIBRARY
template <class T>
extern String str_arr(const Vector<T> arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ", bool add_braces = true) {
	int s = (int)arr.size(), ss = (int)arr.size();
	DEFAULT_STR_ARR_BODY;
}

template <class T>
static String str_arr(PoolVector<T> arr, const bool force_full = false, const int max_shown_items = 64, String separator = ", ", bool add_braces = true) {
	String res = add_braces ? "[ " : "";
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
	release_pva_read(r);

	if (is_long) {
		res += str(int64_t(arr.size()) - s) + " more items...";
	}

	if (add_braces) {
		return res + " ]";
	} else {
		return res;
	}
};
#else

#define POOLARRAYS_STR_ARR(TYPE)                                                                                                                      \
	static String str_arr(TYPE arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ", bool add_braces = true) { \
		String res = add_braces ? "[ " : "";                                                                                                          \
		int s = arr.size();                                                                                                                           \
		bool is_long = false;                                                                                                                         \
		if (s > max_shown_items && !force_full) {                                                                                                     \
			s = max_shown_items;                                                                                                                      \
			is_long = true;                                                                                                                           \
		}                                                                                                                                             \
                                                                                                                                                      \
		auto r = arr.read();                                                                                                                          \
		for (int i = 0; i < s; i++) {                                                                                                                 \
			res += str(r[i]);                                                                                                                         \
			if (i != s - 1 || is_long) {                                                                                                              \
				res += separator;                                                                                                                     \
			}                                                                                                                                         \
		}                                                                                                                                             \
		release_pva_read(r);                                                                                                                          \
                                                                                                                                                      \
		if (is_long) {                                                                                                                                \
			res += str(int64_t(arr.size()) - s) + " more items...";                                                                                   \
		}                                                                                                                                             \
                                                                                                                                                      \
		if (add_braces) {                                                                                                                             \
			return res + " ]";                                                                                                                        \
		} else {                                                                                                                                      \
			return res;                                                                                                                               \
		}                                                                                                                                             \
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

template <class T>
inline void vec_remove_obj(std::vector<T> &v, const T &item) {
	v.erase(std::remove(v.begin(), v.end(), item), v.end());
}

template <class K, class V>
static Dictionary map_to_dict(std::map<K, V> m) {
	Dictionary res;
	for (auto p : m) {
		res[p.first] = p.second;
	}
	return res;
}

template <class K, class V>
static std::map<K, V> dict_to_map(Dictionary d) {
	std::map<K, V> res;
	Array keys = d.keys();
	Array values = d.values();
	for (int i = 0; i < keys.size(); i++) {
		res[keys[i]] = values[i];
	}
	keys.clear();
	values.clear();
	return res;
}

template <class V>
static Array vec_to_arr(std::vector<V> v) {
	Array res;
	res.resize((int)v.size());
	for (int i = 0; i < v.size(); i++) {
		res[i] = v[i];
	}
	return res;
}

template <class V>
static std::vector<V> arr_to_vec(Array a) {
	std::vector<V> res;
	res.resize(a.size());
	for (int i = 0; i < a.size(); i++) {
		res[i] = a[i];
	}
	return res;
}

template <class V, class VAL>
static bool is_vector_contains(V vec, VAL val) {
	return std::find(vec.begin(), vec.end(), val) != vec.end();
}

template <class V, typename PRED>
static bool is_vector_contains_if(std::vector<V> vec, PRED pred) {
	return std::find_if(vec.begin(), vec.end(), pred) != vec.end();
}

#ifndef GDNATIVE_LIBRARY
extern Vector<Variant> vec_args(const std::vector<Variant> &args);

#else
extern Array vec_args(const std::vector<Variant> &args);

extern PoolByteArray _gdn_convert_native_pointer_array_to_pba(uint8_t *p, int64_t size);
extern void _gdn_convert_array_to_native_pointer_array(uint8_t *pDst, const Array &pSrc, int64_t size);
extern String _gdn_get_file_as_string(String path, Error *ret_err);
extern Variant _gdn_dictionary_get_key_at_index(Dictionary d, int idx);
extern Variant _gdn_dictionary_get_value_at_index(Dictionary d, int idx);
extern Variant _GLOBAL_DEF(const String &p_var, const Variant &p_default, bool p_restart_if_changed = false);
#endif

// https://stackoverflow.com/a/33021408/8980874
extern int64_t rand64();

}; // namespace GRUtils
