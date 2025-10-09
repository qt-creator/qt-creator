// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nodeinstancetracing.h"

#include <tracing/qmldesignertracing.h>

#include <sqlitebasestatement.h>

namespace QmlDesigner::NodeInstanceTracing {

#ifdef ENABLE_NODE_INSTANCE_TRACING

Category &category()
{
    thread_local Category category_{"node instances",
                                    Tracing::eventQueueWithStringArguments(),
                                    Tracing::eventQueueWithoutArguments(),
                                    category};

    return category_;
}


#endif

} // namespace QmlDesigner::NodeInstanceTracing
