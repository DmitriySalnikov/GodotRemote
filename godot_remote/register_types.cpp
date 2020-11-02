/* register_types.cpp */

#include "register_types.h"

#include "GRClient.h"
#include "GRDevice.h"
#include "GRNotifications.h"
#include "GRPacket.h"
#include "GRServer.h"
#include "GRUtils.h"
#include "GodotRemote.h"

// clumsy settings to test
// outbound and ip.DstAddr >= 127.0.0.1 and ip.DstAddr <= 127.255.255.255 and (tcp.DstPort == 52341 or tcp.SrcPort == 52341)

#ifndef GDNATIVE_LIBRARY
#include "core/class_db.h"
#include "core/engine.h"
#include "core/project_settings.h"

void register_godot_remote_types() {
	ClassDB::register_class<GodotRemote>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", memnew(GodotRemote)));
	GRUtils::init();

	ClassDB::register_class<GRNotifications>();
	ClassDB::register_class<GRNotificationPanel>();
	ClassDB::register_class<GRNotificationPanelUpdatable>();
	ClassDB::register_class<GRNotificationStyle>();

	ClassDB::register_virtual_class<GRDevice>();

#ifndef NO_GODOTREMOTE_SERVER
	ClassDB::register_class<GRServer>();
	ClassDB::register_class<GRSViewport>();
	ClassDB::register_class<GRSViewportRenderer>();
#endif

#ifndef NO_GODOTREMOTE_CLIENT
	ClassDB::register_class<GRClient>();
	ClassDB::register_class<GRInputCollector>();
	ClassDB::register_class<GRTextureRect>();
#endif


	ClassDB::register_virtual_class<GRPacket>();
}

void unregister_godot_remote_types() {
	memdelete(GodotRemote::get_singleton());
	GRUtils::deinit();
}

#else
#include <Engine.hpp>
#include <ProjectSettings.hpp>

using namespace godot;

/** GDNative Initialize **/
extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options * o) {
	Godot::gdnative_init(o);
}

/** GDNative Terminate **/
extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options * o) {
	Godot::gdnative_terminate(o);
}

/** NativeScript Initialize **/
extern "C" void GDN_EXPORT godot_nativescript_init(void* handle) {
	Godot::nativescript_init(handle);

	register_class<GodotRemote>();
	//Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", memnew(GodotRemote)));
	GRUtils::init();

	register_class<GRNotifications>();
	register_class<GRNotificationPanel>();
	register_class<GRNotificationPanelUpdatable>();
	register_class<GRNotificationStyle>();

	//register_class<GRDevice>();

#ifndef NO_GODOTREMOTE_SERVER
	register_class<GRServer>();
	register_class<GRSViewport>();
	register_class<GRSViewportRenderer>();
#endif

#ifndef NO_GODOTREMOTE_CLIENT
	register_class<GRClient>();
	register_class<GRInputCollector>();
	register_class<GRTextureRect>();
#endif

	//register_class<GRPacket>();
}
#endif