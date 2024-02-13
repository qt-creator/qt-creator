// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldesignercorelib_exports.h>

#include <nanotrace/nanotracehr.h>

#pragma once

namespace QmlDesigner {
namespace Tracing {

constexpr NanotraceHR::Tracing tracingStatus()
{
#ifdef ENABLE_QMLDESIGNER_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using EventQueue = NanotraceHR::StringViewEventQueue<tracingStatus()>;
using EventQueueWithStringArguments = NanotraceHR::StringViewWithStringArgumentsEventQueue<tracingStatus()>;

QMLDESIGNERCORE_EXPORT EventQueue &eventQueue();
QMLDESIGNERCORE_EXPORT EventQueueWithStringArguments &eventQueueWithStringArguments();

} // namespace Tracing

namespace ModelTracing {
constexpr NanotraceHR::Tracing tracingStatus()
{
#ifdef ENABLE_MODEL_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using Category = NanotraceHR::StringViewWithStringArgumentsCategory<tracingStatus()>;
using ObjectTraceToken = Category::ObjectTokenType;
QMLDESIGNERCORE_EXPORT Category &category();

} // namespace ModelTracing
} // namespace QmlDesigner
