// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldesignercomponents_global.h>

#include <nanotrace/nanotracehr.h>

#pragma once

namespace QmlDesigner::FormEditorTracing {

#ifdef ENABLE_FORM_EDITOR_TRACING

using Category = NanotraceHR::EnabledCategory;
using SourceLocation = Category::SourceLocation;

[[gnu::pure]] QMLDESIGNERCOMPONENTS_EXPORT Category &category();

#else

using Category = NanotraceHR::DisabledCategory;
using SourceLocation = Category::SourceLocation;

inline Category category()
{
    return {};
}

#endif
} // namespace QmlDesigner::FormEditorTracing
