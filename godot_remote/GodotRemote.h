/* GodotRemote.h */
#pragma once

#include "GRUtils.h"
#include "core/image.h"
#include "core/pool_vector.h"
#include "core/reference.h"

class GodotRemote : public Reference {
	GDCLASS(GodotRemote, Reference);

	static GodotRemote *singleton;

public:
	enum ConnectionType {
		WiFi = 0,
		ADB = 1,
	};

	enum DeviceType {
		DEVICE_Auto = 0,
		DEVICE_Development = 1,
		DEVICE_Standalone = 2,
	};

private:
	const String ps_autoload_name = "debug/godot_remote/general/autostart";
	const String ps_port_name = "debug/godot_remote/general/port";
	const String ps_con_type_name = "debug/godot_remote/general/connection_type";
	const String ps_jpg_mb_size_name = "debug/godot_remote/server/jpg_compress_buffer_size_mbytes";

	bool is_autostart = false;
	uint16_t port = 52341;
	ConnectionType con_type = ConnectionType::WiFi;

	class GRDevice *device = nullptr;
	void register_and_load_settings();
#ifdef TOOLS_ENABLED
	void _native_run_emitted();
#endif

protected:
	static void _bind_methods();
	void _notification(int p_notification);

public:
	// GRUtils functions binds for GDScript
	void set_log_level(GRUtils::LogLevel lvl) {
		GRUtils::set_log_level(lvl);
	}

	PoolByteArray compress_jpg(Ref<Image> orig_img, int quality = 75, int subsampling = GRUtils::Subsampling::SUBSAMPLING_H2V2) {
		return GRUtils::compress_jpg(orig_img, quality, subsampling);
	}

	void set_gravity(const Vector3 &p_gravity) const {
		GRUtils::set_gravity(p_gravity);
	}

	void set_accelerometer(const Vector3 &p_accel) const {
		GRUtils::set_accelerometer(p_accel);
	}

	void set_magnetometer(const Vector3 &p_magnetometer) const {
		GRUtils::set_magnetometer(p_magnetometer);
	}

	void set_gyroscope(const Vector3 &p_gyroscope) const {
		GRUtils::set_gyroscope(p_gyroscope);
	}
	// GRUtils end

	class GRDevice *get_device() const;
	// must be call_deffered
	bool start_remote_device(DeviceType type = DeviceType::DEVICE_Auto);
	bool stop_remote_device();

#ifdef TOOLS_ENABLED
	void _adb_port_forwarding();
#endif

	static GodotRemote *get_singleton();
	GodotRemote();
	~GodotRemote();
};

VARIANT_ENUM_CAST(GodotRemote::DeviceType)
VARIANT_ENUM_CAST(GodotRemote::ConnectionType)
