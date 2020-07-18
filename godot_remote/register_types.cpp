/* register_types.cpp */

#include "register_types.h"

#include "GRClient.h"
#include "GRDevice.h"
#include "GRNotifications.h"
#include "GRServer.h"
#include "GRUtils.h"
#include "GodotRemote.h"
#include "core/class_db.h"
#include "core/engine.h"
#include "core/project_settings.h"

// clumsy settings to test
// outbound and ip.DstAddr >= 127.0.0.1 and ip.DstAddr <= 127.255.255.255 and (tcp.DstPort == 52341 or tcp.SrcPort == 52341)

void register_godot_remote_types() {
	ClassDB::register_class<GodotRemote>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", memnew(GodotRemote)));
	GRUtils::init();

	ClassDB::register_class<GRNotifications>();
	ClassDB::register_class<GRNotificationPanel>();

	ClassDB::register_virtual_class<GRDevice>();
	ClassDB::register_class<GRServer>();
	ClassDB::register_class<GRClient>();

	ClassDB::register_class<GRSViewport>();
	ClassDB::register_class<GRSViewportRenderer>();
	ClassDB::register_class<GRInputCollector>();
	ClassDB::register_class<GRTextureRect>();
}

void unregister_godot_remote_types() {
	memdelete(GodotRemote::get_singleton());
	GRUtils::deinit();
}
