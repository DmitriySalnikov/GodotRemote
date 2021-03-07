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
	ClassDB::register_class<GRUtilsData>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", memnew(GodotRemote)));
	//GRUtils::init();

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

	// Packets
	ClassDB::register_virtual_class<GRPacket>();
	ClassDB::register_class<GRPacketClientStreamAspect>();
	ClassDB::register_class<GRPacketClientStreamOrientation>();
	ClassDB::register_class<GRPacketCustomInputScene>();
	ClassDB::register_class<GRPacketImageData>();
	ClassDB::register_class<GRPacketInputData>();
	ClassDB::register_class<GRPacketMouseModeSync>();
	ClassDB::register_class<GRPacketServerSettings>();
	ClassDB::register_class<GRPacketSyncTime>();
	ClassDB::register_class<GRPacketCustomUserData>();

	ClassDB::register_class<GRPacketPing>();
	ClassDB::register_class<GRPacketPong>();

	// Input Data
	ClassDB::register_virtual_class<GRInputData>();
	ClassDB::register_class<GRInputDeviceSensorsData>();
	ClassDB::register_class<GRInputDataEvent>();

	ClassDB::register_class<GRIEDataWithModifiers>();
	ClassDB::register_class<GRIEDataMouse>();
	ClassDB::register_class<GRIEDataGesture>();

	ClassDB::register_class<GRIEDataAction>();
	ClassDB::register_class<GRIEDataJoypadButton>();
	ClassDB::register_class<GRIEDataJoypadMotion>();
	ClassDB::register_class<GRIEDataKey>();
	ClassDB::register_class<GRIEDataMagnifyGesture>();
	ClassDB::register_class<GRIEDataMIDI>();
	ClassDB::register_class<GRIEDataMouseButton>();
	ClassDB::register_class<GRIEDataMouseMotion>();
	ClassDB::register_class<GRIEDataPanGesture>();
	ClassDB::register_class<GRIEDataScreenDrag>();
	ClassDB::register_class<GRIEDataScreenTouch>();
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
	register_class<GRUtilsData>();
	//Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", memnew(GodotRemote)));
	//GRUtils::init(); // moved to GodotRemote::init()

	register_class<GRNotifications>();
	register_class<GRNotificationPanel>();
	register_class<GRNotificationPanelUpdatable>();
	register_class<GRNotificationStyle>();

	register_class<GRDevice>();

	///////////////////////////////////////
	///////////////////////////////////////
	// must be registered to instantiate

	// Packets
	register_class<GRPacket>();
	register_class<GRPacketClientStreamAspect>();
	register_class<GRPacketClientStreamOrientation>();
	register_class<GRPacketCustomInputScene>();
	register_class<GRPacketImageData>();
	register_class<GRPacketInputData>();
	register_class<GRPacketMouseModeSync>();
	register_class<GRPacketServerSettings>();
	register_class<GRPacketSyncTime>();
	register_class<GRPacketCustomUserData>();

	register_class<GRPacketPing>();
	register_class<GRPacketPong>();

	// Input Data
	register_class<GRInputData>();
	register_class<GRInputDeviceSensorsData>();
	register_class<GRInputDataEvent>();

	register_class<GRIEDataWithModifiers>();
	register_class<GRIEDataMouse>();
	register_class<GRIEDataGesture>();

	register_class<GRIEDataAction>();
	register_class<GRIEDataJoypadButton>();
	register_class<GRIEDataJoypadMotion>();
	register_class<GRIEDataKey>();
	register_class<GRIEDataMagnifyGesture>();
	register_class<GRIEDataMIDI>();
	register_class<GRIEDataMouseButton>();
	register_class<GRIEDataMouseMotion>();
	register_class<GRIEDataPanGesture>();
	register_class<GRIEDataScreenDrag>();
	register_class<GRIEDataScreenTouch>();

	//////////////////////////////////////
	//////////////////////////////////////

#ifndef NO_GODOTREMOTE_SERVER
	register_class<GRServer>();
	register_class<GRServer::ListenerThreadParamsServer>();
	register_class<GRServer::ConnectionThreadParamsServer>();
	register_class<GRSViewport::ImgProcessingViewportStorage>();
	register_class<GRSViewport>();
	register_class<GRSViewportRenderer>();
#endif

#ifndef NO_GODOTREMOTE_CLIENT
	register_class<GRClient>();
	register_class<GRClient::ConnectionThreadParamsClient>();
	register_class<GRClient::ImgProcessingStorageClient>();
	register_class<GRInputCollector>();
	register_class<GRTextureRect>();
#endif

	//register_class<GRPacket>();
}
#endif
