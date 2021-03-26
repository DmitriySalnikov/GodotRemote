/* register_types.cpp */

#include "register_types.h"

#include "GRClient.h"
#include "GRDevice.h"
#include "GRNotifications.h"
#include "GRServer.h"
#include "GRStreamDecoderH264.h"
#include "GRStreamDecoderImageSequence.h"
#include "GRStreamDecoders.h"
#include "GRStreamEncoderH264.h"
#include "GRStreamEncoderImageSequence.h"
#include "GRStreamEncoders.h"
#include "GodotRemote.h"
#include "GRProfilerViewportMiniPreview.h"

// clumsy settings to test
// outbound and ip.DstAddr >= 127.0.0.1 and ip.DstAddr <= 127.255.255.255 and (tcp.DstPort == 52341 or tcp.SrcPort == 52341)

/*
* // TODO livepp needs more work. Like sync points lpp::lppSyncPoint(moduleReturnedFromLoadLibraryFunction); where i can restart server.
#ifdef GODOTREMOTE_LIVEPP
#include "windows.h"

#include "LivePP/LPP_API.h"
HMODULE livePP = NULL;

#endif
*/

#ifndef GDNATIVE_LIBRARY
#include "core/class_db.h"
#include "core/engine.h"
#include "core/project_settings.h"

void register_godot_remote_types() {
	ClassDB::register_class<GodotRemote>();
	Engine::get_singleton()->add_singleton(Engine::Singleton(NAMEOF(GodotRemote), memnew(GodotRemote)));
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
	ClassDB::register_class<GRStreamEncodersManager>();
	ClassDB::register_class<GRStreamEncoder>();
	ClassDB::register_class<GRStreamEncoderImageSequence>();
	ClassDB::register_class<GRStreamEncoderH264>();
#endif

#ifndef NO_GODOTREMOTE_CLIENT
	ClassDB::register_class<GRClient>();
	ClassDB::register_class<GRInputCollector>();
	ClassDB::register_class<GRTextureRect>();
#endif

/*
* // TODO move to GodotRemote class because here it's breaks everything
* 
#ifdef GODOTREMOTE_LIVEPP
	auto args = OS::get_singleton()->get_cmdline_args();
	for (int i = 0; i < args.size(); i++) {
		if (args[i] == "--livepp") {
			livePP = lpp::lppLoadAndRegister(L"LivePP", "GodotRemote");
			if (livePP) {
				lpp::lppEnableAllCallingModulesSync(livePP);
			}
		}
	}
#endif
*/
}

void unregister_godot_remote_types() {
	memdelete(GodotRemote::get_singleton());
	GRUtils::deinit();

	/*
#ifdef GODOTREMOTE_LIVEPP
	if (livePP)
		lpp::lppShutdown(livePP);
#endif
*/
}

#else
#include <Engine.hpp>
#include <ProjectSettings.hpp>

using namespace godot;

/** GDNative Initialize **/
extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *o) {
	Godot::gdnative_init(o);
}

/** GDNative Terminate **/
extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
	Godot::gdnative_terminate(o);
}

/** NativeScript Initialize **/
extern "C" void GDN_EXPORT godot_nativescript_init(void *handle) {
	Godot::nativescript_init(handle);

	register_class<GodotRemote>();
	//Engine::get_singleton()->add_singleton(Engine::Singleton("GodotRemote", memnew(GodotRemote)));
	//GRUtils::init(); // moved to GodotRemote::init()

	register_class<GRNotifications>();
	register_class<GRNotificationPanel>();
	register_class<GRNotificationPanelUpdatable>();
	register_class<GRNotificationStyle>();
	register_class<GRProfilerViewportMiniPreview>();

	register_class<GRDevice>();

#ifndef NO_GODOTREMOTE_SERVER
	register_class<GRServer>();
	register_class<GRServer::ListenerThreadParamsServer>();
	register_class<GRServer::ConnectionThreadParamsServer>();
	register_class<GRSViewport>();
	register_class<GRSViewportRenderer>();
	register_class<GRStreamEncodersManager>();
	register_class<GRStreamEncoder>();
	register_class<GRStreamEncoderImageSequence>();
	register_class<GRStreamEncoderH264>();
#endif

#ifndef NO_GODOTREMOTE_CLIENT
	register_class<GRClient>();
	register_class<GRClient::ConnectionThreadParamsClient>();
	register_class<GRClient::ImgProcessingStorageClient>();
	register_class<GRInputCollector>();
	register_class<GRTextureRect>();
	register_class<GRStreamDecodersManager>();
	register_class<GRStreamDecoder>();
	register_class<GRStreamDecoderImageSequence>();
	register_class<GRStreamDecoderH264>();
#endif
	//register_class<GRPacket>();
}
#endif
