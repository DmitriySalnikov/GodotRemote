/* register_types.cpp */

#include "register_types.h"

#include "GRDevice.h"
#include "GRServer.h"
#include "GRClient.h"
#include "GRUtils.h"
#include "GodotRemote.h"
#include "core/class_db.h"
#include "core/engine.h"
#include "core/project_settings.h"

void register_godot_remote_types() {
	ClassDB::register_class<GodotRemote>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", memnew(GodotRemote)));
	GRUtils::init();

	ClassDB::register_virtual_class<GRDevice>();
	ClassDB::register_class<GRServer>();
	ClassDB::register_class<GRClient>();

	ClassDB::register_class<GRSViewport>();
	ClassDB::register_class<GRSViewportRenderer>();
	ClassDB::register_class<GRInputCollector>();
}

void unregister_godot_remote_types() {
	memdelete(GodotRemote::get_singleton());
	GRUtils::deinit();
}
