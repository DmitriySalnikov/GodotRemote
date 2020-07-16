/* GRUtils.cpp */

#include "GRUtils.h"
#include "GodotRemote.h"

#ifndef NO_GODOTREMOTE_SERVER
// richgel999/jpeg-compressor: https://github.com/richgel999/jpeg-compressor
#include "jpge.h"
#endif

namespace GRUtils {
int current_loglevel =
#ifdef DEBUG_ENABLED
		LogLevel::LL_Normal;
#else
		LogLevel::LL_Warning;
#endif

PoolByteArray internal_PACKET_HEADER = PoolByteArray();
PoolByteArray internal_VERSION = PoolByteArray();

#ifndef NO_GODOTREMOTE_SERVER
int compress_buffer_size_mb = 4;
PoolByteArray compress_buffer = PoolByteArray();
#endif

void init() {
	GR_PACKET_HEADER('G', 'R', 'H', 'D');
	GR_VERSION(1, 0, 0);
}

void deinit() {
	internal_PACKET_HEADER.resize(0);
	internal_VERSION.resize(0);
}

#ifndef NO_GODOTREMOTE_SERVER
void init_server_utils() {
	GET_PS_SET(compress_buffer_size_mb, GodotRemote::ps_jpg_buffer_mb_size_name);
	compress_buffer.resize((1024 * 1024) * compress_buffer_size_mb);
}

void deinit_server_utils() {
	compress_buffer.resize(0);
}
#endif

void _log(const Variant &val, LogLevel lvl) {
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

String str_arr(const Array arr, const bool force_full, const int max_shown_items) {
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

String str_arr(const Dictionary arr, const bool force_full, const int max_shown_items) {
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

String str_arr(const uint8_t *data, const int size, const bool force_full, const int max_shown_items) {
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

#ifndef NO_GODOTREMOTE_SERVER
PoolByteArray compress_jpg(PoolByteArray &img_data, int width, int height, int bytes_for_color, int quality, int subsampling) {
	PoolByteArray res;
	ERR_FAIL_COND_V(img_data.empty(), res);
	ERR_FAIL_COND_V(quality < 1 || quality > 100, res);

	jpge::params params;
	params.m_quality = quality;
	params.m_subsampling = (jpge::subsampling_t)subsampling;

	ERR_FAIL_COND_V(!params.check(), res);
	auto rb = compress_buffer.read();
	auto ri = img_data.read();
	int size = compress_buffer.size();

	TimeCountInit();

	ERR_FAIL_COND_V_MSG(!jpge::compress_image_to_jpeg_file_in_memory(
								(void *)rb.ptr(),
								size,
								width,
								height,
								bytes_for_color,
								(const unsigned char *)ri.ptr(),
								params),
			res, "Can't compress image.");

	TimeCount("Compress img");

	rb.release();
	ri.release();

	res.append_array(compress_buffer.subarray(0, size - 1));
	TimeCount("Combine arrays");

	_log("JPG size: " + str(res.size()), LogLevel::LL_Debug);
	return res;
}
#endif

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
		case Variant::AABB: {
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

PoolByteArray var2bytes(const Variant &data, bool full_objects) {
	PoolByteArray barr;
	int len;
	Error err = encode_variant(data, NULL, len, full_objects);
	if (err) {
		//_log(String("Unexpected error in var2bytes: ") + String::num(err), LogLevel::LL_Error);
		return barr;
	}

	barr.resize(len);
	{
		PoolByteArray::Write w = barr.write();
		encode_variant(data, w.ptr(), len, full_objects);
	}
	return barr;
}

PoolByteArray int162bytes(const int16_t &data) {
	PoolByteArray res;
	res.resize(2);
	auto w = res.write();
	encode_uint16(data, w.ptr());
	return res;
}

PoolByteArray int322bytes(const int32_t &data) {
	PoolByteArray res;
	res.resize(4);
	auto w = res.write();
	encode_uint32(data, w.ptr());
	return res;
}

PoolByteArray int642bytes(const int64_t &data) {
	PoolByteArray res;
	res.resize(8);
	auto w = res.write();
	encode_uint64(data, w.ptr());
	return res;
}

PoolByteArray uint162bytes(const uint16_t &data) {
	PoolByteArray res;
	res.resize(2);
	auto w = res.write();
	encode_uint16(data, w.ptr());
	return res;
}

PoolByteArray uint322bytes(const uint32_t &data) {
	PoolByteArray res;
	res.resize(4);
	auto w = res.write();
	encode_uint32(data, w.ptr());
	return res;
}

PoolByteArray uint642bytes(const uint64_t &data) {
	PoolByteArray res;
	res.resize(8);
	auto w = res.write();
	encode_uint64(data, w.ptr());
	return res;
}

PoolByteArray float2bytes(const float &data) {
	PoolByteArray res;
	res.resize(4);
	auto w = res.write();
	encode_float(data, w.ptr());
	return res;
}

PoolByteArray double2bytes(const double &data) {
	PoolByteArray res;
	res.resize(8);
	auto w = res.write();
	encode_double(data, w.ptr());
	return res;
}

int16_t bytes2int16(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return (int16_t)decode_uint16(r.ptr() + idx);
}

int32_t bytes2int32(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return (int32_t)decode_uint32(r.ptr() + idx);
}

int64_t bytes2int64(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return (int64_t)decode_uint64(r.ptr() + idx);
}

uint16_t bytes2uint16(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return (int16_t)decode_uint16(r.ptr() + idx);
}

uint32_t bytes2uint32(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return (int32_t)decode_uint32(r.ptr() + idx);
}

uint64_t bytes2uint64(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return (int64_t)decode_uint64(r.ptr() + idx);
}

float bytes2float(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return decode_float(r.ptr() + idx);
}

double bytes2double(const PoolByteArray &data, int idx) {
	auto r = data.read();
	return decode_double(r.ptr() + idx);
}

int16_t bytes2int16(const uint8_t *data) {
	return (int16_t)decode_uint16(data);
}

int32_t bytes2int32(const uint8_t *data) {
	return (int32_t)decode_uint32(data);
}

int64_t bytes2int64(const uint8_t *data) {
	return (int64_t)decode_uint64(data);
}

uint16_t bytes2uint16(const uint8_t *data) {
	return (int16_t)decode_uint16(data);
}

uint32_t bytes2uint32(const uint8_t *data) {
	return (int32_t)decode_uint32(data);
}

uint64_t bytes2uint64(const uint8_t *data) {
	return (int64_t)decode_uint64(data);
}

float bytes2float(const uint8_t *data) {
	return decode_float(data);
}

double bytes2double(const uint8_t *data) {
	return decode_double(data);
}

bool validate_packet(const uint8_t *data) {
	if (data[0] == internal_PACKET_HEADER[0] && data[1] == internal_PACKET_HEADER[1] && data[2] == internal_PACKET_HEADER[2] && data[3] == internal_PACKET_HEADER[3])
		return true;
	return false;
}

bool validate_version(const PoolByteArray &data) {
	if (data.size() < 2)
		return false;
	if (data[0] == internal_VERSION[0] && data[1] == internal_VERSION[1])
		return true;
	return false;
}

bool validate_version(const uint8_t *data) {
	if (data[0] == internal_VERSION[0] && data[1] == internal_VERSION[1])
		return true;
	return false;
}

bool compare_pool_byte_arrays(const PoolByteArray &a, const PoolByteArray &b) {
	if (a.size() != b.size())
		return false;
	auto r_a = a.read();
	auto r_b = b.read();
	for (int i = 0; i<a.size(); i++)
	{
		if (r_a[i] != r_b[i])
			return false;
	}

	return true;
}

void set_gravity(const Vector3 &p_gravity) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_gravity(p_gravity);
}

void set_accelerometer(const Vector3 &p_accel) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_accelerometer(p_accel);
}

void set_magnetometer(const Vector3 &p_magnetometer) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_magnetometer(p_magnetometer);
}

void set_gyroscope(const Vector3 &p_gyroscope) {
	auto *id = (InputDefault *)Input::get_singleton();
	if (id)
		id->set_gyroscope(p_gyroscope);
}

} // namespace GRUtils
