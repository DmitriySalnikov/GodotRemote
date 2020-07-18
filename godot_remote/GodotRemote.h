/* GodotRemote.h */
#pragma once

#include "core/image.h"
#include "core/pool_vector.h"
#include "core/reference.h"

class GodotRemote : public Reference {
	GDCLASS(GodotRemote, Reference);

	friend class GRDevice;
	static GodotRemote *singleton;

public:
	enum DeviceType {
		DEVICE_Auto = 0,
		DEVICE_Development = 1,
		DEVICE_Standalone = 2,
	};

	enum class TypesOfServerSettings {
		USE_INTERNAL_SERVER_SETTINGS = 0,
		JPG_QUALITY = 1,
		SEND_FPS = 2,
		RENDER_SCALE = 3,
	};

	static String ps_autoload_name;
	static String ps_notifications_enabled_name;
	static String ps_noticications_position_name;
	static String ps_notifications_duration_name;
	static String ps_port_name;
	static String ps_config_adb_name;
	static String ps_jpg_buffer_mb_size_name;
	static String ps_jpg_quality_name;
	static String ps_scale_of_sending_stream_name;
	static String ps_auto_adjust_scale_name;
	static String ps_password_name;

private:
	bool is_autostart = false;
	bool is_notifications_enabled = true;

	class GRDevice *device = nullptr;

	void register_and_load_settings();

	void _create_notification_manager();
	void _remove_notifications_manager();
#ifdef TOOLS_ENABLED
	void _run_emitted();
	void _adb_port_forwarding();
#endif

protected:
	static void _bind_methods();
	void _notification(int p_notification);

public:
	// GRNotifications
	Array get_all_notifications() const;

	void set_notifications_layer(int layer) const;
	int get_notifications_layer() const;

	void set_notifications_position(int positon) const;
	int get_notifications_position() const;

	void set_notifications_enabled(bool _enabled) const;
	bool get_notifications_enabled() const;

	void set_notifications_duration(float _duration) const;
	float get_notifications_duration() const;

	void add_notification(String title, String text, bool update_existing = true) const;
	void remove_notification(String title, bool all_entries = true) const;
	void remove_notification_exact(Node *_notif) const;
	void clear_notifications() const;
	// GRNotifications end

	// GRUtils functions binds for GDScript
	void set_log_level(int lvl) const;
	void set_gravity(const Vector3 &p_gravity) const;
	void set_accelerometer(const Vector3 &p_accel) const;
	void set_magnetometer(const Vector3 &p_magnetometer) const;
	void set_gyroscope(const Vector3 &p_gyroscope) const;
	// GRUtils end

	class GRDevice *get_device() const;

	// must be call_deffered
	void create_and_start_device(DeviceType type = DeviceType::DEVICE_Auto);
	bool create_remote_device(DeviceType type = DeviceType::DEVICE_Auto);
	bool start_remote_device();
	bool remove_remote_device();

	static GodotRemote *get_singleton();
	GodotRemote();
	~GodotRemote();
};

VARIANT_ENUM_CAST(GodotRemote::DeviceType)
VARIANT_ENUM_CAST(GodotRemote::TypesOfServerSettings)
