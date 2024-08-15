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
using StringEventQueue = NanotraceHR::StringEventQueue<tracingStatus()>;

[[gnu::pure]] QMLDESIGNERCORE_EXPORT EventQueue &eventQueue();
[[gnu::pure]] QMLDESIGNERCORE_EXPORT EventQueueWithStringArguments &eventQueueWithStringArguments();
[[gnu::pure]] QMLDESIGNERCORE_EXPORT StringEventQueue &stringEventQueue();

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

using Category = NanotraceHR::StringCategory<tracingStatus()>;
using AsynchronousToken = Category::AsynchronousTokenType;
[[gnu::pure]] QMLDESIGNERCORE_EXPORT Category &category();

} // namespace ModelTracing

namespace ProjectStorageTracing {
constexpr NanotraceHR::Tracing projectStorageTracingStatus()
{
#ifdef ENABLE_PROJECT_STORAGE_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using Category = NanotraceHR::StringViewWithStringArgumentsCategory<projectStorageTracingStatus()>;

[[gnu::pure]] Category &projectStorageCategory();

[[gnu::pure]] Category &projectStorageUpdaterCategory();

} // namespace ProjectStorageTracing

namespace SourcePathStorageTracing {
constexpr NanotraceHR::Tracing tracingStatus()
{
#ifdef ENABLE_SOURCE_PATH_STORAGE_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using Category = NanotraceHR::StringViewWithStringArgumentsCategory<tracingStatus()>;

[[gnu::pure]] Category &category();

} // namespace SourcePathStorageTracing

namespace MetaInfoTracing {
constexpr NanotraceHR::Tracing tracingStatus()
{
#ifdef ENABLE_METAINFO_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using Category = NanotraceHR::StringViewWithStringArgumentsCategory<tracingStatus()>;

[[gnu::pure]] Category &category();

} // namespace MetaInfoTracing
} // namespace QmlDesigner
