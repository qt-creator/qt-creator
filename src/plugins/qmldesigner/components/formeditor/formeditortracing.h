// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldesignercomponents_global.h>

#include <nanotrace/nanotracehr.h>

#pragma once

namespace QmlDesigner::FormEditorTracing {

constexpr NanotraceHR::Tracing tracingStatus()
{
#ifdef ENABLE_FORM_EDITOR_TRACING
    return NanotraceHR::Tracing::IsEnabled;
#else
    return NanotraceHR::Tracing::IsDisabled;
#endif
}

using Category = NanotraceHR::StringViewWithStringArgumentsCategory<tracingStatus()>;
using SourceLocation = Category::SourceLocation;

[[gnu::pure]] QMLDESIGNERCOMPONENTS_EXPORT Category &category();

} // namespace QmlDesigner::FormEditorTracing
