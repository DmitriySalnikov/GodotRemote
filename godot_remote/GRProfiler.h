/* GRProfiler.h */
#ifndef GRPROFILER_H
#define GRPROFILER_H

#ifdef GODOTREMOTE_TRACY_ENABLED

//#include "tracy/client/TracyCallstack.hpp"
#include "tracy/Tracy.hpp"

// CUSTOM MACROS

#define TracyPlotExecTimeStart(name) int64_t _tracy_plot_time_##name = (int64_t)OS::get_singleton()->get_ticks_usec()
#define TracyPlotExecTimeEnd(name)                                                              \
	TracyPlot(#name, (int64_t)OS::get_singleton()->get_ticks_usec() - _tracy_plot_time_##name); \
	TracyPlotConfig(#name, tracy::PlotFormatType::Number);

#else

// CUSTOM MACROS

#define PlotExecTimeStart(name)
#define PlotExecTimeEnd(name)

// TRACY MACRO

#define ZoneNamed(varname, active)
#define ZoneNamedN(varname, name, active)
#define ZoneNamedC(varname, color, active)
#define ZoneNamedNC(varname, name, color, active)

#define ZoneTransient(varname, active)
#define ZoneTransientN(varname, name, active)

#define ZoneScoped
#define ZoneScopedN(name)
#define ZoneScopedC(color)
#define ZoneScopedNC(name, color)

#define ZoneText(txt, size)
#define ZoneTextV(varname, txt, size)
#define ZoneName(txt, size)
#define ZoneNameV(varname, txt, size)
#define ZoneColor(color)
#define ZoneColorV(varname, color)
#define ZoneValue(value)
#define ZoneValueV(varname, value)
#define ZoneIsActive false
#define ZoneIsActiveV(varname) false

#define FrameMark
#define FrameMarkNamed(name)
#define FrameMarkStart(name)
#define FrameMarkEnd(name)

#define FrameImage(image, width, height, offset, flip)

#define TracyLockable(type, varname) type varname;
#define TracyLockableN(type, varname, desc) type varname;
#define TracySharedLockable(type, varname) type varname;
#define TracySharedLockableN(type, varname, desc) type varname;
#define LockableBase(type) type
#define SharedLockableBase(type) type
#define LockMark(varname) (void)x;
#define LockableName(varname, txt, size) ;

#define TracyPlot(name, val)
#define TracyPlotConfig(name, type)

#define TracyMessage(txt, size)
#define TracyMessageL(txt)
#define TracyMessageC(txt, size, color)
#define TracyMessageLC(txt, color)
#define TracyAppInfo(txt, size)

#define TracyAlloc(ptr, size)
#define TracyFree(ptr)
#define TracySecureAlloc(ptr, size)
#define TracySecureFree(ptr)

#define TracyAllocN(ptr, size, name)
#define TracyFreeN(ptr, name)
#define TracySecureAllocN(ptr, size, name)
#define TracySecureFreeN(ptr, name)

#define ZoneNamedS(varname, depth, active)
#define ZoneNamedNS(varname, name, depth, active)
#define ZoneNamedCS(varname, color, depth, active)
#define ZoneNamedNCS(varname, name, color, depth, active)

#define ZoneTransientS(varname, depth, active)
#define ZoneTransientNS(varname, name, depth, active)

#define ZoneScopedS(depth)
#define ZoneScopedNS(name, depth)
#define ZoneScopedCS(color, depth)
#define ZoneScopedNCS(name, color, depth)

#define TracyAllocS(ptr, size, depth)
#define TracyFreeS(ptr, depth)
#define TracySecureAllocS(ptr, size, depth)
#define TracySecureFreeS(ptr, depth)

#define TracyAllocNS(ptr, size, depth, name)
#define TracyFreeNS(ptr, depth, name)
#define TracySecureAllocNS(ptr, size, depth, name)
#define TracySecureFreeNS(ptr, depth, name)

#define TracyMessageS(txt, size, depth)
#define TracyMessageLS(txt, depth)
#define TracyMessageCS(txt, size, color, depth)
#define TracyMessageLCS(txt, color, depth)

#define TracyParameterRegister(cb)
#define TracyParameterSetup(idx, name, isBool, val)
#define TracyIsConnected false

#endif // !GODOTREMOTE_TRACY_ENABLED

#endif // !GRPROFILER_H
