/* register_types.cpp */

#include "register_types.h"

#include "GRClient.h"
#include "GRNotifications.h"
#include "GRProfilerViewportMiniPreview.h"
#include "GRServer.h"
#include "GRStreamDecoderH264.h"
#include "GRStreamDecoderImageSequence.h"
#include "GRStreamDecoders.h"
#include "GRStreamEncoderH264.h"
#include "GRStreamEncoderImageSequence.h"
#include "GRStreamEncoders.h"
#include "GRToolMenuPlugin.h"
#include "GRViewportCaptureRect.h"
#include "GodotRemote.h"

#if !defined(GDNATIVE_LIBRARY) && defined(TOOLS_ENABLED)
#include "editor/editor_plugin.h"
#endif

// clumsy settings to test
// outbound and ip.DstAddr >= 127.0.0.1 and ip.DstAddr <= 127.255.255.255 and (tcp.DstPort == 22766 or tcp.SrcPort == 22766)

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
	// GRUtils::init();

	ClassDB::register_class<GRNotifications>();
	ClassDB::register_class<GRNotificationListWithSafeZone>();
	ClassDB::register_class<GRNotificationPanel>();
	ClassDB::register_class<GRNotificationPanelUpdatable>();
	ClassDB::register_class<GRNotificationStyle>();

	ClassDB::register_class<GRViewportCaptureRect>();
	ClassDB::register_class<GRProfilerViewportMiniPreview>();

	ClassDB::register_virtual_class<GRDevice>();

#ifndef NO_GODOTREMOTE_SERVER
	ClassDB::register_class<GRServer>();
	ClassDB::register_class<GRSViewport>();
#endif

#ifndef NO_GODOTREMOTE_CLIENT
	ClassDB::register_class<GRClient>();
	ClassDB::register_class<GRInputCollector>();
	ClassDB::register_class<GRTextureRect>();
#endif

#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<GRToolMenuPlugin>();
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
#include <AcceptDialog.hpp>
#include <ClassDB.hpp>
#include <EditorInterface.hpp>
#include <IP.hpp>
#include <PCKPacker.hpp>
#include <PackedScene.hpp>
#include <PopupMenu.hpp>
#include <RandomNumberGenerator.hpp>
#include <RegEx.hpp>
#include <RegExMatch.hpp>
#include <ResourceLoader.hpp>
#include <SceneTree.hpp>
#include <Shader.hpp>
#include <TCP_Server.hpp>

using namespace godot;

/** GDNative Initialize **/
extern "C" void GDN_EXPORT godot_gdnative_init(godot_gdnative_init_options *o) {
	Godot::gdnative_init(o);

#ifdef GODOT_REMOTE_CUSTOM_INIT_TRIMMED_GODOT_CPP
	// Custom register and init for only needed classes
	// Should be used with my patch, but might work without it (sort of)

	// Base class for others
	godot::_TagDB::register_global_type("Object", typeid(Object).hash_code(), 0);
	Object::___init_method_bindings();

#define CUSTOM_CLASS(_class, _base, _name)                                                             \
	godot::_TagDB::register_global_type(_name, typeid(_class).hash_code(), typeid(_base).hash_code()); \
	_class::___init_method_bindings();

#define TEXT(t) #t
#define REGULAR_CLASS(_class, _base) CUSTOM_CLASS(_class, _base, #_class)
#define REGULAR_CLASS_DASH(_class, _base) CUSTOM_CLASS(_class, _base, TEXT(_##_class))

	REGULAR_CLASS(AcceptDialog, WindowDialog);
	REGULAR_CLASS(BoxContainer, Container);
	REGULAR_CLASS(Button, BaseButton);
	REGULAR_CLASS(CanvasItem, Node);
	REGULAR_CLASS(CanvasLayer, Node);
	REGULAR_CLASS(ClassDB, Object);
	REGULAR_CLASS(Control, CanvasItem);
	REGULAR_CLASS(EditorInterface, Node);
	REGULAR_CLASS(EditorPlugin, Node);
	REGULAR_CLASS(Font, Resource);
	REGULAR_CLASS(HBoxContainer, BoxContainer);
	REGULAR_CLASS(Image, Resource);
	REGULAR_CLASS(ImageTexture, Texture);
	REGULAR_CLASS(Input, Object);
	REGULAR_CLASS(InputEvent, Resource);
	REGULAR_CLASS(InputEventAction, InputEvent);
	REGULAR_CLASS(InputEventGesture, InputEventWithModifiers);
	REGULAR_CLASS(InputEventJoypadButton, InputEvent);
	REGULAR_CLASS(InputEventJoypadMotion, InputEvent);
	REGULAR_CLASS(InputEventKey, InputEventWithModifiers);
	REGULAR_CLASS(InputEventMagnifyGesture, InputEventGesture);
	REGULAR_CLASS(InputEventMIDI, InputEvent);
	REGULAR_CLASS(InputEventMouse, InputEventWithModifiers);
	REGULAR_CLASS(InputEventMouseButton, InputEventMouse);
	REGULAR_CLASS(InputEventMouseMotion, InputEventMouse);
	REGULAR_CLASS(InputEventPanGesture, InputEventGesture);
	REGULAR_CLASS(InputEventScreenDrag, InputEvent);
	REGULAR_CLASS(InputEventScreenTouch, InputEvent);
	REGULAR_CLASS(InputEventWithModifiers, InputEvent);
	REGULAR_CLASS(IP, Object);
	REGULAR_CLASS(Label, Control);
	REGULAR_CLASS(Material, Resource);
	REGULAR_CLASS(Node, Object);
	REGULAR_CLASS(Node2D, CanvasItem);
	REGULAR_CLASS(PackedScene, Resource);
	REGULAR_CLASS(PacketPeer, Reference);
	REGULAR_CLASS(PacketPeerStream, PacketPeer);
	REGULAR_CLASS(PanelContainer, Container);
	REGULAR_CLASS(PCKPacker, Reference);
	REGULAR_CLASS(Popup, Control);
	REGULAR_CLASS(PopupMenu, Popup);
	REGULAR_CLASS(ProjectSettings, Object);
	REGULAR_CLASS(RandomNumberGenerator, Reference);
	REGULAR_CLASS(Reference, Object);
	REGULAR_CLASS(RegEx, Reference);
	REGULAR_CLASS(RegExMatch, Reference);
	REGULAR_CLASS(SceneTree, MainLoop);
	REGULAR_CLASS(Shader, Resource);
	REGULAR_CLASS(ShaderMaterial, Material);
	REGULAR_CLASS(StreamPeer, Reference);
	REGULAR_CLASS(StreamPeerBuffer, StreamPeer);
	REGULAR_CLASS(StreamPeerTCP, StreamPeer);
	REGULAR_CLASS(StyleBox, Resource);
	REGULAR_CLASS(StyleBoxEmpty, StyleBox);
	REGULAR_CLASS(StyleBoxFlat, StyleBox);
	REGULAR_CLASS(TCP_Server, Reference);
	REGULAR_CLASS(Texture, Resource);
	REGULAR_CLASS(TextureRect, Control);
	REGULAR_CLASS(Theme, Resource);
	REGULAR_CLASS(Tween, Node);
	REGULAR_CLASS(VBoxContainer, BoxContainer);
	REGULAR_CLASS(Viewport, Node);
	REGULAR_CLASS(WindowDialog, Popup);

	REGULAR_CLASS_DASH(Thread, Reference);
	REGULAR_CLASS_DASH(ResourceLoader, Object);
	REGULAR_CLASS_DASH(OS, Object);
	REGULAR_CLASS_DASH(File, Reference);
	REGULAR_CLASS_DASH(Engine, Object);
	REGULAR_CLASS_DASH(Directory, Reference);

#undef CUSTOM_CLASS
#undef TEXT
#undef REGULAR_CLASS
#undef REGULAR_CLASS_DASH
#endif
}

/** GDNative Terminate **/
extern "C" void GDN_EXPORT godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
	Godot::gdnative_terminate(o);
}

/** NativeScript Initialize **/
extern "C" void GDN_EXPORT godot_nativescript_init(void *handle) {
	Godot::nativescript_init(handle);

	register_tool_class<GodotRemote>();

	register_class<GRNotifications>();
	register_class<GRNotificationListWithSafeZone>();
	register_class<GRNotificationPanel>();
	register_class<GRNotificationPanelUpdatable>();
	register_class<GRNotificationStyle>();
	register_class<GRViewportCaptureRect>();
	register_class<GRProfilerViewportMiniPreview>();

	register_class<GRDevice>();

#ifndef NO_GODOTREMOTE_SERVER
	register_class<GRServer>();
	register_class<GRServer::ListenerThreadParamsServer>();
	register_class<GRServer::ConnectionThreadParamsServer>();
	register_class<GRSViewport>();
	register_class<GRStreamEncodersManager>();
	register_class<GRStreamEncoder>();
	register_class<GRStreamEncoderImageSequence>();
#ifdef GODOT_REMOTE_H264_ENABLED
	register_class<GRStreamEncoderH264>();
#endif
#endif

#ifndef NO_GODOTREMOTE_CLIENT
	register_class<GRClient>();
	register_class<GRClient::ConnectionThreadParamsClient>();
	register_class<GRInputCollector>();
	register_class<GRTextureRect>();
	register_class<GRStreamDecodersManager>();
	register_class<GRStreamDecoder>();
	register_class<GRStreamDecoderImageSequence>();
#ifdef GODOT_REMOTE_H264_ENABLED
	register_class<GRStreamDecoderH264>();
#endif
#endif

	register_tool_class<GRToolMenuPlugin>();
}
#endif
