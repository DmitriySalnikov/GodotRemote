/* register_types.cpp */

#include "register_types.h"

#include "GRDevice.h"
#include "GRDeviceDevelopment.h"
#include "GRDeviceStandalone.h"
#include "GRUtils.h"
#include "GodotRemote.h"
#include "core/class_db.h"
#include "core/engine.h"
#include "core/project_settings.h"

void register_godot_remote_types() {
	GLOBAL_DEF(GodotRemote::ps_autoload_name, true);

	GRUtils::init();
	ClassDB::register_class<GodotRemote>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", new GodotRemote()));

	ClassDB::register_virtual_class<GRDevice>();
	ClassDB::register_class<GRDeviceDevelopment>();
	ClassDB::register_class<GRDeviceStandalone>();

	ClassDB::register_class<GRDDViewport>();
	ClassDB::register_class<GRDDViewportRenderer>();
	ClassDB::register_class<GRInputCollector>();
}

void unregister_godot_remote_types() {
	delete GodotRemote::get_singleton();
	GRUtils::deinit();
}
