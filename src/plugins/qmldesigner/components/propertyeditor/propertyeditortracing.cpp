// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditortracing.h"

#include <tracing/qmldesignertracing.h>

#include <sqlitebasestatement.h>

namespace QmlDesigner::PropertyEditorTracing {
using namespace NanotraceHR::Literals;

#ifdef ENABLE_PROPERTY_EDITOR_TRACING

namespace {

thread_local Category category_{"property editor",
                                Tracing::eventQueueWithStringArguments(),
                                Tracing::eventQueueWithoutArguments(),
                                category};

} // namespace

Category &category()
{
    return category_;
}

#endif

} // namespace QmlDesigner::PropertyEditorTracing
