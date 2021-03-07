/* GRUtils.h */
#ifndef GRUTILS_H
#define GRUTILS_H

#include <algorithm>
#include <deque>
#include <map>
#include <queue>
#include <vector>

#ifndef GDNATIVE_LIBRARY
#include "core/image.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "core/print_string.h"
#include "core/project_settings.h"
#include "core/variant.h"
#include "core/version_generated.gen.h"
#include "main/input_default.h"
//#define VERSION_MINOR 2
//#define VERSION_PATCH 3

#else

#include <Array.hpp>
#include <Dictionary.hpp>
#include <Engine.hpp>
#include <File.hpp>
#include <Godot.hpp>
#include <Image.hpp>
#include <InputDefault.hpp>
#include <Marshalls.hpp>
#include <Mutex.hpp>
#include <OS.hpp>
#include <Object.hpp>
#include <ProjectSettings.hpp>
#include <String.hpp>
#include <Thread.hpp>
#include <Variant.hpp>
using namespace godot;
#endif

// =================================================================
// CONDITIONAL DEFINES

#ifndef GDNATIVE_LIBRARY
#define ENUM_ARG(en) en
#define ENUM_CONV(en) (en)

#define DEF_ARG(a) a
#define DEF_ARGS(a)

#define ST() SceneTree::get_singleton()
#define GD_CLASS(c, p) GDCLASS(c, p)
#define GD_S_CLASS(c, p) GDCLASS(c, p)
#define V_CAST(var, type) ((type)var)

// Bind constant with custom name
//ClassDB::bind_integer_constant(get_class_static(), __constant_get_enum_name(m_constant, #m_constant), #m_constant, m_constant);
#define BIND_ENUM_CONSTANT_CUSTOM(m_enum, m_const, m_name) ClassDB::bind_integer_constant(get_class_static(), #m_enum, m_name, ((int)(m_enum::m_const)));
#define GDNATIVE_BASIC_REGISTER
#define GDNATIVE_BASIC_REGISTER_NO_INIT

#define queue_del queue_delete
#define img_is_empty(img) img->empty()
#define is_valid_ip is_valid
#define release_pva_read(pva) pva.release()
#define release_pva_write(pva) pva.release()

#define dict_get_key_at_index(dict, i) dict.get_key_at_index(i)
#define dict_get_value_at_index(dict, i) dict.get_value_at_index(i)

#define file_get_as_string(path, err) FileAccess::get_file_as_string(path, err);

#define THREAD_FUNC static
#define THREAD_DATA void *
#define _TS_CLASS_ _THREAD_SAFE_CLASS_
#define _TS_METHOD_ _THREAD_SAFE_METHOD_
#define _TS_LOCK_ _THREAD_SAFE_LOCK_
#define _TS_UNLOCK_ _THREAD_SAFE_UNLOCK_
#define Thread_set_name(_name) Thread::set_name(_name)

#if VERSION_MINOR >= 2 && VERSION_PATCH >= 4
#define Mutex_define(_var) Mutex _var
#define Mutex_create(_var)
#define Mutex_delete(_var)
#define Mutex_lock(_var) _var.lock()
#define Mutex_unlock(_var) _var.unlock()

#define t_wait_to_finish(thread) thread.wait_to_finish()
#define Thread_define(_var) class Thread _var
#define Thread_start(_var, _class, function, data_to_send, inst) _var.start(&_class::function, data_to_send)
#define Thread_close(_name) \
	if (_name.is_started()) \
		t_wait_to_finish(_name);
#else
#define Mutex_define(_var) Mutex *_var = nullptr
#define Mutex_create(_var) _var = Mutex::create()
#define Mutex_delete(_var) \
	if (_var) {            \
		memdelete(_var);   \
		_var = nullptr;    \
	}

#define Mutex_lock(_var) _var->lock()
#define Mutex_unlock(_var) _var->unlock()

#define t_wait_to_finish(thread) Thread::wait_to_finish(thread)
#define Thread_define(_var) class Thread *_var = nullptr
#define Thread_start(_var, _class, function, data_to_send, inst) _var = Thread::create(&_class::function, data_to_send)
#define Thread_close(_name)      \
	if (_name) {                 \
		t_wait_to_finish(_name); \
		memdelete(_name);        \
		_name = nullptr;         \
	}
#endif

#else

enum Margin : int {
	MARGIN_LEFT,
	MARGIN_TOP,
	MARGIN_RIGHT,
	MARGIN_BOTTOM
};

// THREAD SAFE classes
// ORIGINAL CODE FROM GODOT CORE
// because GDNative don't has it

class ThreadSafe {

	Mutex *mutex;

public:
	inline void lock() const {
		if (mutex) mutex->lock();
	}
	inline void unlock() const {
		if (mutex) mutex->unlock();
	}

	ThreadSafe() {
		mutex = Mutex::_new();
		if (!mutex) {

			WARN_PRINT("THREAD_SAFE defined, but no default mutex type");
		}
	}

	~ThreadSafe() {
		if (mutex)
			mutex->free();
	}
};

class ThreadSafeMethod {

	const ThreadSafe *_ts;

public:
	ThreadSafeMethod(const ThreadSafe *p_ts) {

		_ts = p_ts;
		_ts->lock();
	}

	~ThreadSafeMethod() { _ts->unlock(); }
};

#define THREAD_FUNC
#define THREAD_DATA Variant
#define _TS_CLASS_ ThreadSafe __thread__safe__
#define _TS_METHOD_ ThreadSafeMethod __thread_safe_method__(&__thread__safe__)
#define _TS_LOCK_ __thread__safe__.lock()
#define _TS_UNLOCK_ __thread__safe__.unlock()

#define Mutex_define(_var) Ref<Mutex> _var
#define Mutex_create(_var) _var = newref(Mutex)
#define Mutex_lock(_var) _var->lock()
#define Mutex_unlock(_var) _var->unlock()
#define Mutex_delete(_var)   \
	if (_var.is_valid()) {   \
		_var.unref();        \
		_var = Ref<Mutex>(); \
	}

#define Thread_define(_var) Ref<Thread> _var
#define t_wait_to_finish(thread) thread->wait_to_finish()
#define Thread_start(_var, _class, function, data_to_send, inst) _var = _gdn_thread_create(inst, #function, data_to_send)
#define Thread_set_name(_name)
#define Thread_close(_name)      \
	if (_name.is_valid()) {      \
		t_wait_to_finish(_name); \
		_name.unref();           \
		_name = Ref<Thread>();   \
	}

// THREAD SAFE END

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
#define GD_S_CLASS(c, p) GODOT_SUBCLASS(c, p)
#define V_CAST(var, type) (var.operator type())
#define GLOBAL_DEF(m_var, m_value) _GLOBAL_DEF(m_var, m_value)
#define GLOBAL_GET(m_var) ProjectSettings::get_singleton()->get(m_var)

#define queue_del queue_free
#define is_valid_ip is_valid_ip_address
#define img_is_empty(img) img->is_empty()
#define release_pva_read(pva)
#define release_pva_write(pva)

#define dict_get_key_at_index(dict, i) _gdn_dictionary_get_key_at_index(dict, i)
#define dict_get_value_at_index(dict, i) _gdn_dictionary_get_value_at_index(dict, i)

#define file_get_as_string(path, err) _gdn_get_file_as_string(path, err)

#define memnew(obj) obj::_new()
#define memdelete(obj) obj->free()

#define GDNATIVE_BASIC_REGISTER        \
public:                                \
	void _init(){};                    \
	static void _register_methods(){}; \
                                       \
protected:

#define GDNATIVE_BASIC_REGISTER_NO_INIT \
public:                                 \
	static void _register_methods(){};  \
                                        \
protected:

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

#define TimeCountInit() uint64_t simple_time_counter = OS::get_singleton()->get_ticks_usec()
#define TimeCountReset() simple_time_counter = OS::get_singleton()->get_ticks_usec()
// Shows delta between this and previous counter. Need to call TimeCountInit before
#define TimeCount(str)                                                                                                                                                   \
	GRUtils::_log(str + String(": ") + String::num((OS::get_singleton()->get_ticks_usec() - simple_time_counter) / 1000.0, 3) + " ms", GodotRemote::LogLevel::LL_DEBUG); \
	simple_time_counter = OS::get_singleton()->get_ticks_usec()
#else

#define TimeCountInit()
#define TimeCountReset()
#define TimeCount(str)

#endif // DEBUG_ENABLED

// =================================================================
// GLOBAL DEFINES

#define sleep_usec(usec) OS::get_singleton()->delay_usec(usec)

#define LEAVE_IF_EDITOR()                          \
	if (Engine::get_singleton()->is_editor_hint()) \
		return;

#define newref(_class) Ref<_class>(memnew(_class))
#define max(x, y) (x > y ? x : y)
#define min(x, y) (x < y ? x : y)
#define _log(val, ll) __log(val, ll, __FILE__, __LINE__)
#define is_vector_contains(vec, val) (std::find(vec.begin(), vec.end(), val) != vec.end())

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

// Get Project Setting
#define GET_PS(setting_name) \
	ProjectSettings::get_singleton()->get_setting(setting_name)
// Get Project Setting and set it to variable
#define GET_PS_SET(variable_to_store, setting_name) \
	variable_to_store = ProjectSettings::get_singleton()->get_setting(setting_name)

enum LogLevel : int {
	LL_DEBUG = 0,
	LL_NORMAL = 1,
	LL_WARNING = 2,
	LL_ERROR = 3,
	LL_NONE,
};

namespace GRUtils {
// DEFINES

template <typename T, typename Container = std::deque<T> >
class iterable_queue : public std::queue<T, Container> {
public:
	typedef typename Container::iterator iterator;
	typedef typename Container::const_iterator const_iterator;

	iterator begin() { return this->c.begin(); }
	iterator end() { return this->c.end(); }
	const_iterator begin() const { return this->c.begin(); }
	const_iterator end() const { return this->c.end(); }
	void add_value_limited(T value, uint32_t limit) {
		if (value > 100000)
			this->push(100000);
		else
			this->push(value);
		while (this->size() > limit) {
			this->pop();
		}
	}
};

class GRUtilsData : public Object {
	GD_CLASS(GRUtilsData, Object);
	GDNATIVE_BASIC_REGISTER;

public:
	int current_loglevel;
	PoolByteArray internal_PACKET_HEADER;
	PoolByteArray internal_VERSION;
};

#ifndef NO_GODOTREMOTE_SERVER
class GRUtilsDataServer {
public:
	PoolByteArray compress_buffer;
	int compress_buffer_size_mb;
};

extern GRUtilsDataServer *_grutils_data_server;
#endif
extern GRUtilsData *_grutils_data;

extern void init();
extern void deinit();

#ifndef NO_GODOTREMOTE_SERVER
extern void init_server_utils();
extern void deinit_server_utils();
extern Error compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, int width, int height, int bytes_for_color = 4, int quality = 75, int subsampling = 3 /*Subsampling ::SUBSAMPLING_H2V2*/);
#endif

extern Error compress_bytes(const PoolByteArray &bytes, PoolByteArray &res, int type);
extern Error decompress_bytes(const PoolByteArray &bytes, int output_size, PoolByteArray &res, int type);
extern void __log(const Variant &val, int lvl = 1 /*LogLevel::LL_NORMAL*/, String file = "", int line = 0);

extern String str(const Variant &val);
extern String str_arr(const Array arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ");
extern String str_arr(const Dictionary arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ");
extern String str_arr(const uint8_t *data, const int size, const bool force_full = false, const int max_shown_items = 64, String separator = ", ");

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
	return (int)val * 1000;
}

// IMPLEMENTATINS

template <class T>
extern String str_arr(const std::vector<T> arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ") {
	String res = "[ ";
	int s = (int)arr.size();
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
		res += str(int64_t(arr.size()) - s) + " more items...";
	}

	return res + " ]";
}

template <class K, class V>
extern String str_arr(const std::map<K, V> arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ") {
	String res = "{ ";
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

	return res + " }";
}

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
	release_pva_read(r);

	if (is_long) {
		res += str(int64_t(arr.size()) - s) + " more items...";
	}

	return res + " ]";
};
#else

#define POOLARRAYS_STR_ARR(TYPE)                                                                                              \
	static String str_arr(TYPE arr, const bool force_full = false, const int max_shown_items = 32, String separator = ", ") { \
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
		release_pva_read(r);                                                                                                  \
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

static PoolByteArray get_packet_header() {
	return _grutils_data->internal_PACKET_HEADER;
}

static PoolByteArray get_gr_version() {
	return _grutils_data->internal_VERSION;
}

static void set_log_level(int lvl) {
	_grutils_data->current_loglevel = lvl;
}

template <typename T>
inline void calculate_avg_min_max_values(const iterable_queue<T> &v, float *avg_val, float *min_val, float *max_val, float(modifier)(double)) {
	if (v.size() > 0) {
		double sum = 0;
		*min_val = (float)v.back();
		*max_val = (float)v.back();

		for (T i : v) {
			sum += i;
			if (i < *min_val)
				*min_val = (float)i;
			else if (i > *max_val)
				*max_val = (float)i;
		}
		*avg_val = modifier(sum / v.size());
		*min_val = modifier(*min_val);
		*max_val = modifier(*max_val);
	} else {
		*avg_val = 0;
		*min_val = 0;
		*max_val = 0;
	}
}

template <class T>
inline void vec_remove_idx(std::vector<T> &v, const T &item) {
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

#ifndef GDNATIVE_LIBRARY
extern Vector<Variant> vec_args(const std::vector<Variant> &args);

#else
extern Array vec_args(const std::vector<Variant> &args);

extern String _gdn_get_file_as_string(String path, Error *ret_err);
extern Variant _gdn_dictionary_get_key_at_index(Dictionary d, int idx);
extern Variant _gdn_dictionary_get_value_at_index(Dictionary d, int idx);
extern Ref<Thread> _gdn_thread_create(Object *instance, String func_name, const Object *user_data);
extern Variant _GLOBAL_DEF(const String &p_var, const Variant &p_default, bool p_restart_if_changed = false);
#endif

}; // namespace GRUtils

#endif // !GRUTILS_H
