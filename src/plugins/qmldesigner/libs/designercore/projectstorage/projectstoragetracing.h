// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldesignercorelib_exports.h>

#include <nanotrace/nanotracehr.h>

#pragma once

namespace QmlDesigner::ProjectStorageTracing {

#ifdef ENABLE_PROJECT_STORAGE_TRACING

using Category = NanotraceHR::EnabledCategory;
using SourceLocation = Category::SourceLocation;

[[gnu::pure]] QMLDESIGNERCORE_EXPORT Category &category();

#else

using Category = NanotraceHR::DisabledCategory;
using SourceLocation = Category::SourceLocation;

inline Category category()
{
    return {};
}

#endif
} // namespace QmlDesigner::ProjectStorageTracing
