/* GRUtils.cpp */

#include "GRUtils.h"
#include "GodotRemote.h"
#include <chrono>
#include <ctime>

#ifndef GDNATIVE_LIBRARY
#include "core/io/compression.h"
#else
#include <ClassDB.hpp>
using namespace godot;
#endif

std::shared_ptr<GRUtilsData> GRUtils::_grutils_data = nullptr;

namespace GRUtils {
void init() {
	_grutils_data = shared_new(GRUtilsData);
	_grutils_data->current_loglevel = LogLevel::LL_NORMAL;
	GET_PS_SET(_grutils_data->current_loglevel, GodotRemote::ps_general_loglevel_name);
	_grutils_data->streamPeerBufferPool = std::make_shared<GRObjectPool<StreamPeerBuffer> >(
			[]() { return newref_std(StreamPeerBuffer); },
			//[]() { return std::shared_ptr<StreamPeerBuffer>(memnew(StreamPeerBuffer), [](StreamPeerBuffer *o) { memdelete(o); _log("StreamPeerBuffer", LogLevel::LL_NORMAL); }); },
			[](RefStd(StreamPeerBuffer) buf) { buf->set_data_array(PoolByteArray()); },
			true, 0);

	LEAVE_IF_EDITOR();

	GR_PACKET_HEADER('G', 'R', 'H', 'D');
#include "GRVersion.h"
	srand((unsigned int)time(0));
}

void deinit() {
	if (_grutils_data) {
		_grutils_data->internal_PACKET_HEADER.resize(0);
		_grutils_data->internal_VERSION.resize(0);
		if (_grutils_data->streamPeerBufferPool) {
			_grutils_data->streamPeerBufferPool->clear();
		}
	}
	LEAVE_IF_EDITOR();
}

#ifdef DEBUG_ENABLED
void __log(const Variant &val, int lvl, String func, String file, int line) {
	if (lvl >= (_grutils_data ? _grutils_data->current_loglevel : LogLevel::LL_DEBUG) && lvl < LogLevel::LL_NONE) {
		if (lvl == LogLevel::LL_ERROR) {
			auto val_str = "[GodotRemote Error] " + str(val);
			//GodotRemote::get_singleton()->call_deferred("print_error_str", val_str, func, file, line);
			GodotRemote::get_singleton()->print_error_str(val_str, func, file, line);
		} else if (lvl == LogLevel::LL_WARNING) {
			auto val_str = "[GodotRemote Warning] " + str(val);
			//GodotRemote::get_singleton()->call_deferred("print_warning_str", val_str, func, file, line);
			GodotRemote::get_singleton()->print_warning_str(val_str, func, file, line);
		} else {
			auto val_str = "[GodotRemote] " + str(val);
			//GodotRemote::get_singleton()->call_deferred("print_str", val_str);
			GodotRemote::get_singleton()->print_str(val_str);
		}
	}

#ifdef GODOT_REMOTE_TRACY_ENABLED
	auto val_str = str(val);
	if (lvl == LogLevel::LL_ERROR) {
		TracyMessageC(val_str.ascii().get_data(), val_str.length(), tracy::Color::Red2);
	} else if (lvl == LogLevel::LL_WARNING) {
		TracyMessageC(val_str.ascii().get_data(), val_str.length(), tracy::Color::Yellow2);
	} else if (lvl == LogLevel::LL_DEBUG) {
		TracyMessageC(val_str.ascii().get_data(), val_str.length(), tracy::Color::Gray55);
	} else {
		TracyMessageC(val_str.ascii().get_data(), val_str.length(), tracy::Color::WhiteSmoke);
	}
#endif // GODOT_REMOTE_TRACY_ENABLED
}
#endif

String str_arr(const Array arr, const bool force_full, const int max_shown_items, String separator) {
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

String str_arr(const Dictionary arr, const bool force_full, const int max_shown_items, String separator) {
	String res = "{ ";
	int s = arr.size();
	bool is_long = false;
	if (s > max_shown_items && !force_full) {
		s = max_shown_items;
		is_long = true;
	}

	Array keys = arr.keys();
	Array values = arr.values();

	for (int i = 0; i < s; i++) {
		res += str(keys[i]) + " : " + str(values[i]);
		if (i != s - 1 || is_long) {
			res += separator;
		}
	}

	keys.clear();
	values.clear();

	if (is_long) {
		res += String::num_int64(int64_t(arr.size()) - s) + " more items...";
	}

	return res + " }";
};

String str_arr(const uint8_t *data, const int size, const bool force_full, const int max_shown_items, String separator) {
	String res = "[ ";
	int s = size;
	bool is_long = false;
	if (s > max_shown_items && !force_full) {
		s = max_shown_items;
		is_long = true;
	}

	for (int i = 0; i < s; i++) {
		res += str(data[i]);
		if (i != s - 1 || is_long) {
			res += separator;
		}
	}

	if (is_long) {
		res += String::num_int64(int64_t(size) - s) + " more bytes...";
	}

	return res + " ]";
};

uint64_t get_time_usec() {
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

Error compress_bytes(const PoolByteArray &bytes, PoolByteArray &res, int type) {
#ifndef GDNATIVE_LIBRARY
	Error err = res.resize(bytes.size());

	ERR_FAIL_COND_V_MSG(err, err, "Can't resize output array");

	int size = 0;
	{
		auto r = bytes.read();
		auto w = res.write();
		size = Compression::compress(w.ptr(), r.ptr(), bytes.size(), (Compression::Mode)type);
	}

	if (size) {
		res.resize(size);
	} else {
		ERR_PRINT("Can't resize output array after compression");
		err = Error::FAILED;
		res = PoolByteArray();
	}

	return err;
#else
	// TODO I don't found any ways to implement compression in GDNative
	_log("Compression not supported in GDNative library", LogLevel::LL_ERROR);
	res = bytes;
	return Error::OK;
#endif
}

Error decompress_bytes(const PoolByteArray &bytes, int output_size, PoolByteArray &res, int type) {
#ifndef GDNATIVE_LIBRARY
	Error err = res.resize(output_size);
	ERR_FAIL_COND_V_MSG(err, err, "Can't resize output array");

	int size = 0;
	{
		auto r = bytes.read();
		auto w = res.write();
		size = Compression::decompress(w.ptr(), output_size, r.ptr(), bytes.size(), (Compression::Mode)type);
	}
	if (output_size == -1) {
		ERR_PRINT("Can't decompress bytes");
		err = Error::FAILED;
		res = PoolByteArray();
	} else if (output_size != size) {
		ERR_PRINT("Desired size not equal to real size");
		err = Error::FAILED;
		res = PoolByteArray();
	}
	return err;
#else
	// TODO I don't found any ways to implement compression in GDNative
	_log("Compression not supported in GDNative library", LogLevel::LL_ERROR);
	res = bytes;
	return Error::OK;
#endif
}

String str(const Variant &val) {
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
			return String("V2(") + v2 + ")";
		}
		case Variant::RECT2: {
			Rect2 r = val;
			return String("R2((") + String::num_real(r.position.x) + ", " + String::num_real(r.position.y) + "), (" + String::num_real(r.size.x) + ", " + String::num_real(r.size.y) + "))";
		}
		case Variant::VECTOR3: {
			Vector3 v3 = val;
			return String("V3(") + v3 + ")";
		}
		case Variant::TRANSFORM: {
			Transform t3d = val;
			return String("T3D(") + t3d + ")";
		}
		case Variant::TRANSFORM2D: {
			Transform2D t2d = val;
			return String("T2D(") + t2d + ")";
		}
		case Variant::PLANE: {
			Plane pln = val;
			return String("P(") + pln + ")";
		}
		case Variant::QUAT: {
			Quat q = val;
			return String("Q(") + q + ")";
		}
#ifndef GDNATIVE_LIBRARY
		case Variant::AABB: {
#else
		case Variant::RECT3: {
#endif
			AABB ab = val;
			return String("AABB(") + ab + ")";
		}
		case Variant::BASIS: {
			Basis bs = val;
			return String("B(") + bs + ")";
		}
		case Variant::COLOR: {
			Color c = val;
			return String("C(") + c + ")";
		}
		case Variant::NODE_PATH: {
			NodePath np = val;
			return String("NP: ") + np + ")";
		}
		case Variant::_RID: {
			RID rid = val;
#ifndef GDNATIVE_LIBRARY
			return String("RID:") + String::num_int64(rid.get_id());
#else
			return String("RID:") + String::num_int64(rid.get_id());
#endif
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
			return str_arr(V_CAST(val, Dictionary));
		}
		case Variant::ARRAY: {
			return str_arr(V_CAST(val, Array));
		}
		case Variant::POOL_BYTE_ARRAY: {
			return str_arr(V_CAST(val, PoolByteArray));
		}
		case Variant::POOL_INT_ARRAY: {
			return str_arr(V_CAST(val, PoolIntArray));
		}
		case Variant::POOL_REAL_ARRAY: {
			return str_arr(V_CAST(val, PoolRealArray));
		}
		case Variant::POOL_STRING_ARRAY: {
			return str_arr(V_CAST(val, PoolStringArray));
		}
		case Variant::POOL_VECTOR2_ARRAY: {
			return str_arr(V_CAST(val, PoolVector2Array));
		}
		case Variant::POOL_VECTOR3_ARRAY: {
			return str_arr(V_CAST(val, PoolVector3Array));
		}
		case Variant::POOL_COLOR_ARRAY: {
			return str_arr(V_CAST(val, PoolColorArray));
		}
	}
#ifndef GDNATIVE_LIBRARY
	return String("|? ") + Variant::get_type_name(type) + " ?|";
#else
	return String("|? ") + type + " ?|";
#endif
}

bool validate_packet(const uint8_t *data) {
	if (data[0] == _grutils_data->internal_PACKET_HEADER[0] && data[1] == _grutils_data->internal_PACKET_HEADER[1] && data[2] == _grutils_data->internal_PACKET_HEADER[2] && data[3] == _grutils_data->internal_PACKET_HEADER[3])
		return true;
	return false;
}

bool validate_version(const PoolByteArray &data) {
	if (data.size() < 2)
		return false;
	if (((PoolByteArray)data)[0] == _grutils_data->internal_VERSION[0] && ((PoolByteArray)data)[1] == _grutils_data->internal_VERSION[1])
		return true;
	return false;
}

bool validate_version(const uint8_t *data) {
	if (data[0] == _grutils_data->internal_VERSION[0] && data[1] == _grutils_data->internal_VERSION[1])
		return true;
	return false;
}

std::shared_ptr<GRObjectPool<StreamPeerBuffer> > get_stream_peer_buffer_pool() {
	return _grutils_data->streamPeerBufferPool;
}

bool compare_pool_byte_arrays(const PoolByteArray &a, const PoolByteArray &b) {
	if (a.size() != b.size())
		return false;
	auto r_a = a.read();
	auto r_b = b.read();
	for (int i = 0; i < a.size(); i++) {
		if (r_a[i] != r_b[i])
			return false;
	}

	return true;
}

bool compare_pool_string_arrays(const PoolStringArray &a, const PoolStringArray &b) {
	if (a.size() == b.size()) {
		auto ra = a.read();
		auto rb = b.read();
		for (int i = 0; i < (int)a.size(); i++) {
			if (ra[i] != rb[i]) {
				return false;
			}
		}
	} else {
		return false;
	}
	return true;
}

bool compare_pool_string_arrays(const std::vector<String> &a, const PoolStringArray &b) {
	if (a.size() == b.size()) {
		auto rb = b.read();
		for (int i = 0; i < (int)a.size(); i++) {
			if (a[i] != rb[i]) {
				return false;
			}
		}
	} else {
		return false;
	}
	return true;
}

void set_gravity(const Vector3 &p_gravity) {
#ifndef GDNATIVE_LIBRARY
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_gravity(p_gravity);
#else
	Input *id = Input::get_singleton();
	if (ClassDB::get_singleton()->class_has_method(id->get_class(), "set_gravity")) {
		id->call("set_gravity", p_gravity);
	}
#endif
}

void set_accelerometer(const Vector3 &p_accel) {
#ifndef GDNATIVE_LIBRARY
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_accelerometer(p_accel);
#else
	Input *id = Input::get_singleton();
	if (ClassDB::get_singleton()->class_has_method(id->get_class(), "set_accelerometer")) {
		id->call("set_accelerometer", p_accel);
	}
#endif
}

void set_magnetometer(const Vector3 &p_magnetometer) {
#ifndef GDNATIVE_LIBRARY
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_magnetometer(p_magnetometer);
#else
	Input *id = Input::get_singleton();
	if (ClassDB::get_singleton()->class_has_method(id->get_class(), "set_magnetometer")) {
		id->call("set_magnetometer", p_magnetometer);
	}
#endif
}

void set_gyroscope(const Vector3 &p_gyroscope) {
#ifndef GDNATIVE_LIBRARY
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_gyroscope(p_gyroscope);
#else
	Input *id = Input::get_singleton();
	if (ClassDB::get_singleton()->class_has_method(id->get_class(), "set_gyroscope")) {
		id->call("set_gyroscope", p_gyroscope);
	}
#endif
}

Ref<_Thread> utils_thread_create(Object *instance, String func_name, const Variant &user_data) {
	Ref<_Thread> t = newref(_Thread);
	t->start(instance, func_name, user_data);
	return t;
}

#ifndef GDNATIVE_LIBRARY
Vector<Variant> vec_args(const std::vector<Variant> &args) {
	Vector<Variant> res;
	for (Variant v : args) {
		res.push_back(v);
	}

	return res;
}

#else
Array vec_args(const std::vector<Variant> &args) {
	return vec_to_arr(args);
}

PoolByteArray _gdn_convert_native_pointer_array_to_pba(uint8_t *p, int64_t size) {
	PoolByteArray a;
	a.resize((int)size);
	auto w = a.write();
	memcpy(w.ptr(), p, size);
	return a;
}

void _gdn_convert_array_to_native_pointer_array(uint8_t *pDst, const Array &pSrc, int64_t size) {
	PoolByteArray a = PoolByteArray(pSrc);
	auto r = a.read();
	memcpy(pDst, r.ptr(), size);
}

String _gdn_get_file_as_string(String path, Error *ret_err) {
	auto f = memnew(File);
	Error r = f->open(path, File::ModeFlags::READ);
	*ret_err = r;
	if (r == Error::OK) {
		String txt = f->get_as_text();
		f->close();
		memdelete(f);
		return txt;
	} else {
		memdelete(f);
	}
	return "";
}

Variant _gdn_dictionary_get_key_at_index(Dictionary d, int idx) {
	Array k = d.keys();
	Variant r = k[idx];
	k.clear();
	return r;
}

Variant _gdn_dictionary_get_value_at_index(Dictionary d, int idx) {
	Array v = d.values();
	Variant r = v[idx];
	v.clear();
	return r;
}

Variant _GLOBAL_DEF(const String &p_var, const Variant &p_default, bool p_restart_if_changed) {
	Variant ret;
	if (!ProjectSettings::get_singleton()->has_setting(p_var)) {
		ProjectSettings::get_singleton()->set(p_var, p_default);
	}
	ret = ProjectSettings::get_singleton()->get(p_var);

	ProjectSettings::get_singleton()->set_initial_value(p_var, p_default);
	//ProjectSettings::get_singleton()->set_builtin_order(p_var);
	//ProjectSettings::get_singleton()->set_restart_if_changed(p_var, p_restart_if_changed);
	return ret;
}
#endif

// https://stackoverflow.com/a/33021408/8980874

#define IMAX_BITS(m) ((m) / ((m) % 255 + 1) / 255 % 255 * 8 + 7 - 86 / ((m) % 255 + 12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)
static_assert((RAND_MAX & (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");

int64_t rand64() {
	int64_t r = 0;
	for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
		r <<= RAND_MAX_WIDTH;
		r ^= rand();
	}
	return r * get_time_usec();
}

} // namespace GRUtils
