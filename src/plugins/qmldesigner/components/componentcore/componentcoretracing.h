// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <nanotrace/nanotracehr.h>

#pragma once

namespace QmlDesigner::ComponentCoreTracing {

#ifdef ENABLE_COMPONENT_CORE_TRACING

using Category = NanotraceHR::EnabledCategory;
using SourceLocation = Category::SourceLocation;

[[gnu::pure]] Category &category();

#else

using Category = NanotraceHR::DisabledCategory;
using SourceLocation = Category::SourceLocation;

inline Category category()
{
    return {};
}

#endif
} // namespace QmlDesigner::ComponentCoreTracing
