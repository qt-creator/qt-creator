// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formeditortracing.h"

#include <tracing/qmldesignertracing.h>

#include <sqlitebasestatement.h>

namespace QmlDesigner::FormEditorTracing {
using namespace NanotraceHR::Literals;
namespace {

thread_local Category category_{"model", Tracing::eventQueueWithStringArguments(), category};

} // namespace

Category &category()
{
    return category_;
}

} // namespace QmlDesigner::FormEditorTracing
