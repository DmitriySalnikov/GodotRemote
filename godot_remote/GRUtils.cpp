/* GRUtils.cpp */

#include "GRUtils.h"
#include "GRProfiler.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY
#include "core/io/compression.h"

#else
#include <ClassDB.hpp>

using namespace godot;
#endif

GRUtilsData *GRUtils::_grutils_data = nullptr;

namespace GRUtils {
void init() {
	LEAVE_IF_EDITOR();
	_grutils_data = memnew(GRUtilsData);
	_grutils_data->current_loglevel = LogLevel::LL_NORMAL;

	GR_PACKET_HEADER('G', 'R', 'H', 'D');
#include "GRVersion.h"

	GET_PS_SET(_grutils_data->current_loglevel, GodotRemote::ps_general_loglevel_name);
}

void deinit() {
	LEAVE_IF_EDITOR();
	if (_grutils_data) {
		_grutils_data->internal_PACKET_HEADER.resize(0);
		_grutils_data->internal_VERSION.resize(0);
		memdelete(_grutils_data);
		_grutils_data = nullptr;
	}
}

#ifdef DEBUG_ENABLED
void __log(const Variant &val, int lvl, String file, int line) {
	if (lvl >= _grutils_data->current_loglevel && lvl < LogLevel::LL_NONE) {
		auto val_str = str(val);
#ifndef GDNATIVE_LIBRARY
		String file_line = "";
		if (lvl > LogLevel::LL_NORMAL && file != "") {
			int idx = file.find("godot_remote");
			if (idx != -1) {
				file = file.substr(file.find("godot_remote"), file.length());
			}

			file_line = "\n    At: " + file + ":" + str(line);
		}

		if (lvl == LogLevel::LL_ERROR) {
			print_error("[GodotRemote] " + val_str + file_line);
		} else if (lvl == LogLevel::LL_WARNING) {
			print_error("[GodotRemote] " + val_str + file_line);
		} else {
			print_line("[GodotRemote] " + val_str);
		}
#else

		if (lvl > LogLevel::LL_NORMAL && file != "") {
			int idx = file.find("godot_remote");
			if (idx != -1)
				file = file.substr(file.find("godot_remote"), file.length());
		}

		if (lvl == LogLevel::LL_ERROR) {
			Godot::print_error(val_str, "[GodotRemote Error]", file, line);
		} else if (lvl == LogLevel::LL_WARNING) {
			Godot::print_warning(val_str, "[GodotRemote Warning]", file, line);
		} else {
			Godot::print("[GodotRemote] " + val_str);
		}
#endif
	}

#ifdef GODOTREMOTE_TRACY_ENABLED
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
#endif // GODOTREMOTE_TRACY_ENABLED
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

#ifndef GDNATIVE_LIBRARY
Vector<Variant> vec_args(const std::vector<Variant> &args) {
	Vector<Variant> res;
	for (Variant v : args) {
		res.push_back(v);
	}

	return res;
}

Ref<_Thread> _utils_thread_create(Object *instance, String func_name, const Variant &user_data) {
	Ref<_Thread> t = newref(_Thread);
	t->start(instance, func_name, user_data);
	return t;
}

#else
Array vec_args(const std::vector<Variant> &args) {
	return vec_to_arr(args);
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

Ref<Thread> _utils_thread_create(Object *instance, String func_name, const Variant &user_data) {
	Ref<Thread> t = newref(Thread);
	t->start(instance, func_name, user_data);
	return t;
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

} // namespace GRUtils
