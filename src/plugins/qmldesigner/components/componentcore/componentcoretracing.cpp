// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "componentcoretracing.h"

#include <tracing/qmldesignertracing.h>

#include <sqlitebasestatement.h>

namespace QmlDesigner::ComponentCoreTracing {

#ifdef ENABLE_COMPONENT_CORE_TRACING

Category &category()
{
    thread_local Category category_{"component core",
                                    Tracing::eventQueueWithStringArguments(),
                                    Tracing::eventQueueWithoutArguments(),
                                    category};

    return category_;
}


#endif

} // namespace QmlDesigner::ComponentCoreTracing
