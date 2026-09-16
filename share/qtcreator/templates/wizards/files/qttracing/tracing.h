%{JS: Cpp.licenseTemplate()}\
#pragma once

#include "%{ProviderName}_tracepoints_p.h"

#include <QString>

// Where a tracepoint was reached, in the form the trace viewer reads to link
// an event back to the source. Pass it wherever a tracepoint of
// %{ProviderName}.tracepoints takes an argument named "location":
//
//     Q_TRACE_SCOPE(render, width, height, %{MacroPrefix}_TRACE_LOCATION);
//     Q_TRACE(message, "frame started", %{MacroPrefix}_TRACE_LOCATION);
#define %{MacroPrefix}_TRACE_LOCATION QString::fromLatin1(__FILE__ ":" QT_STRINGIFY(__LINE__))
