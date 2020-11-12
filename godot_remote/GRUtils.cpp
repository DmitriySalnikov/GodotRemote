/* GRUtils.cpp */

#include "GRUtils.h"
#include "GodotRemote.h"

#ifndef GDNATIVE_LIBRARY
#include "core/io/compression.h"

#else

using namespace godot;
#endif

#ifndef NO_GODOTREMOTE_SERVER
// richgel999/jpeg-compressor: https://github.com/richgel999/jpeg-compressor
#include "jpge.h"
#endif

GRUtilsData* GRUtils::_grutils_data = nullptr;

namespace GRUtils {
void init() {
	_grutils_data = new GRUtilsData();
	_grutils_data->current_loglevel = LogLevel::LL_Normal;

	GR_PACKET_HEADER('G', 'R', 'H', 'D');
	GR_VERSION(1, 0, 0);

	GET_PS_SET(_grutils_data->current_loglevel, GodotRemote::ps_general_loglevel_name);
}

void deinit() {
	if (_grutils_data) {
		_grutils_data->internal_PACKET_HEADER.resize(0);
		_grutils_data->internal_VERSION.resize(0);
		delete _grutils_data;
	}
}

#ifndef NO_GODOTREMOTE_SERVER
GRUtilsDataServer* GRUtils::_grutils_data_server = nullptr;

void init_server_utils() {
	_grutils_data_server = new GRUtilsDataServer();

	GET_PS_SET(_grutils_data_server->compress_buffer_size_mb, GodotRemote::ps_server_jpg_buffer_mb_size_name);
	_grutils_data_server->compress_buffer.resize((1024 * 1024) * _grutils_data_server->compress_buffer_size_mb);
}

void deinit_server_utils() {
	_grutils_data_server->compress_buffer.resize(0);
	delete _grutils_data_server;
}
#endif

void _log(const Variant &val, LogLevel lvl) {
#ifdef DEBUG_ENABLED
#ifndef GDNATIVE_LIBRARY
	if (lvl >= current_loglevel && lvl < LogLevel::LL_None) {
		if (lvl == LogLevel::LL_Error) {
			print_error("[GodotRemote Error] " + str(val));
		} else if (lvl == LogLevel::LL_Warning) {
			print_error("[GodotRemote Warning] " + str(val));
		} else {
			print_line("[GodotRemote] " + str(val));
		}
	}

#else

#define print_error_ext(msg) Godot::print_error("[GodotRemote Error] " + str(val), __FUNCTION__, __FILE__, __LINE__ )

	if (lvl >= _grutils_data->current_loglevel && lvl < LogLevel::LL_None) {
		if (lvl == LogLevel::LL_Error) {
			print_error_ext("[GodotRemote Error] " + str(val));
		}
		else if (lvl == LogLevel::LL_Warning) {
			print_error_ext("[GodotRemote Warning] " + str(val));
		}
		else {
			Godot::print("[GodotRemote] " + str(val));
		}
	}
#undef print_error_ext
#endif
#endif
}

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

	for (int i = 0; i < s; i++) {
		res += str(dict_get_key_at_index(arr, i)) + " : " + str(dict_get_value_at_index(arr, i));
		if (i != s - 1 || is_long) {
			res += separator;
		}
	}

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

#ifndef NO_GODOTREMOTE_SERVER
Error compress_jpg(PoolByteArray &ret, const PoolByteArray &img_data, int width, int height, int bytes_for_color, int quality, int subsampling) {
	PoolByteArray res;
	ERR_FAIL_COND_V(img_data.size() == 0, Error::ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(quality < 1 || quality > 100, Error::ERR_INVALID_PARAMETER);

	jpge::params params;
	params.m_quality = quality;
	params.m_subsampling = (jpge::subsampling_t)subsampling;

	ERR_FAIL_COND_V(!params.check(), Error::ERR_INVALID_PARAMETER);
	auto rb = _grutils_data_server->compress_buffer.read();
	auto ri = img_data.read();
	int size = _grutils_data_server->compress_buffer.size();

	TimeCountInit();

	ERR_FAIL_COND_V_MSG(!jpge::compress_image_to_jpeg_file_in_memory(
								(void *)rb.ptr(),
								size,
								width,
								height,
								bytes_for_color,
								(const unsigned char *)ri.ptr(),
								params),
			Error::FAILED, "Can't compress image.");

	TimeCount("Compress img");

	res.resize(size);
	auto wr = res.write();
	memcpy(wr.ptr(), rb.ptr(), size);

	TimeCount("Combine arrays");

	_log("JPG size: " + str(res.size()), LogLevel::LL_Debug);

	ret = res;
	return Error::OK;
}
#endif

Error compress_bytes(const PoolByteArray &bytes, PoolByteArray &res, int type) {
#ifndef GDNATIVE_LIBRARY
	Error err =	res.resize(bytes.size());

	ERR_FAIL_COND_V_MSG(err, err, "Can't resize output array");

	auto r = bytes.read();
	auto w = res.write();
	int size = Compression::compress(w.ptr(), r.ptr(), bytes.size(), (Compression::Mode)type);

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
	res = bytes;
	return Error::OK;
#endif
}

Error decompress_bytes(const PoolByteArray &bytes, int output_size, PoolByteArray &res, int type) {
#ifndef GDNATIVE_LIBRARY
	Error err = res.resize(output_size);
	ERR_FAIL_COND_V_MSG(err, err, "Can't resize output array");

	auto r = bytes.read();
	auto w = res.write();
	int size = Compression::decompress(w.ptr(), output_size, r.ptr(), bytes.size(), (Compression::Mode)type);

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
		case Variant::RECT3: {
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
#else
	auto* id = Input::get_singleton();
#endif
	if (id)
		id->set_gravity(p_gravity);
}

void set_accelerometer(const Vector3 &p_accel) {
#ifndef GDNATIVE_LIBRARY
	auto* id = (InputDefault*)Input::get_singleton();
#else
	auto* id = Input::get_singleton();
#endif
	if (id)
		id->set_accelerometer(p_accel);
}

void set_magnetometer(const Vector3 &p_magnetometer) {
#ifndef GDNATIVE_LIBRARY
	auto* id = (InputDefault*)Input::get_singleton();
#else
	auto* id = Input::get_singleton();
#endif
	if (id)
		id->set_magnetometer(p_magnetometer);
}

void set_gyroscope(const Vector3 &p_gyroscope) {
#ifndef GDNATIVE_LIBRARY
	auto* id = (InputDefault*)Input::get_singleton();
#else
	auto* id = Input::get_singleton();
#endif
	if (id)
		id->set_gyroscope(p_gyroscope);
}

} // namespace GRUtils
