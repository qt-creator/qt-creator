// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignertracing.h"

#include <nanotrace/tracefile.h>

#include <sqlitebasestatement.h>

namespace QmlDesigner {

using namespace NanotraceHR::Literals;

namespace Tracing {

#ifdef ENABLE_QMLDESIGNER_TRACING

EventQueueWithStringArguments &eventQueueWithStringArguments()
{
    thread_local NanotraceHR::EnabledEventQueueWithArguments eventQueue(NanotraceHR::traceFile());

    return eventQueue;
}

EventQueueWithoutArguments &eventQueueWithoutArguments()
{
    thread_local NanotraceHR::EnabledEventQueueWithoutArguments eventQueue(NanotraceHR::traceFile());

    return eventQueue;
}

#endif

} // namespace Tracing

namespace ModelTracing {

#ifdef ENABLE_MODEL_TRACING

namespace {

thread_local Category category_{"model",
                                Tracing::eventQueueWithStringArguments(),
                                Tracing::eventQueueWithoutArguments(),
                                category};

} // namespace

Category &category()
{
    return category_;
}

#endif

} // namespace ModelTracing

namespace ProjectManagingTracing {
#ifdef ENABLE_PROJECT_MANAGING_TRACING

Category &category()
{
    thread_local Category category_{"project manager",
                                    Tracing::eventQueueWithStringArguments(),
                                    Tracing::eventQueueWithoutArguments(),
                                    category};

    return category_;
}
#endif

} // namespace ProjectManagingTracing
} // namespace QmlDesigner
