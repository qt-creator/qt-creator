// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibrarytracing.h"

#include <tracing/qmldesignertracing.h>

#include <sqlitebasestatement.h>

namespace QmlDesigner::ItemLibraryTracing {
using namespace NanotraceHR::Literals;

#ifdef ENABLE_ITEM_LIBRARY_TRACING

namespace {

thread_local Category category_{"item library",
                                Tracing::eventQueueWithStringArguments(),
                                Tracing::eventQueueWithoutArguments(),
                                category};

} // namespace

Category &category()
{
    return category_;
}

#endif

} // namespace QmlDesigner::ItemLibraryTracing
