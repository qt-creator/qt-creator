// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourcepathstoragetracing.h"

#include <sqlitebasestatement.h>

namespace QmlDesigner::SourcePathStorageTracing {
using namespace NanotraceHR::Literals;

#ifdef ENABLE_SOURCE_PATH_STORAGE_TRACING

namespace {

thread_local Category category_{"source path storage",
                                Tracing::eventQueueWithStringArguments(),
                                Tracing::eventQueueWithoutArguments(),
                                category};

} // namespace

Category &category()
{
    return category_;
}

#endif

} // namespace QmlDesigner::SourcePathStorageTracing
