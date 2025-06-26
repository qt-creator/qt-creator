// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldesignercorelib_exports.h>

#include <nanotrace/nanotracehr.h>

#pragma once

namespace QmlDesigner {
namespace Tracing {

#ifdef ENABLE_QMLDESIGNER_TRACING

using EventQueueWithStringArguments = NanotraceHR::EnabledEventQueueWithArguments;
using EventQueueWithoutArguments = NanotraceHR::EnabledEventQueueWithoutArguments;

[[gnu::pure]] QMLDESIGNERCORE_EXPORT EventQueueWithStringArguments &eventQueueWithStringArguments();
[[gnu::pure]] QMLDESIGNERCORE_EXPORT EventQueueWithoutArguments &eventQueueWithoutArguments();

#endif

} // namespace Tracing

namespace ModelTracing {

#ifdef ENABLE_MODEL_TRACING

using Category = NanotraceHR::EnabledCategory;
using SourceLocation = Category::SourceLocation;
using AsynchronousToken = NanotraceHR::EnabledAsynchronousToken;
using FlowToken = NanotraceHR::EnabledFlowToken;
[[gnu::pure]] QMLDESIGNERCORE_EXPORT Category &category();

#else

using Category = NanotraceHR::DisabledCategory;
using SourceLocation = Category::SourceLocation;
using AsynchronousToken = NanotraceHR::DisabledAsynchronousToken;
using FlowToken = NanotraceHR::DisabledFlowToken;

inline Category category()
{
    return {};
};

#endif

} // namespace ModelTracing

namespace ProjectManagingTracing {

#ifdef ENABLE_PROJECT_MANAGING_TRACING
using Category = NanotraceHR::EnabledCategory;

[[gnu::pure]] QMLDESIGNERCORE_EXPORT Category &category();
#else

using Category = NanotraceHR::DisabledCategory;

inline Category category()
{
    return {};
}

#endif
} // namespace ProjectManagingTracing
} // namespace QmlDesigner
